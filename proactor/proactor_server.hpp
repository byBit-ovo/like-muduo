#pragma once
/*
 * Proactor 并发服务器
 *
 * 线程模型（对比主从 Reactor）：
 *
 *  主从 Reactor                     Proactor 并发服务器
 *  ─────────────────────            ──────────────────────────────
 *  主线程: epoll_wait(accept fd)    主线程: io_uring ACCEPT
 *    → 新连接分发给 sub-reactor       → 新连接分发给 Worker
 *  子线程: epoll_wait(conn fds)     Worker 线程: io_uring READ/WRITE
 *    → 可读 → 调 read()               → 内核完成 I/O → 调完成回调
 *    → 可写 → 调 write()
 *
 *  相同点：都是 one-loop-per-thread + round-robin 分发
 *  差异点：Reactor 自己做 I/O，Proactor 让内核做 I/O
 */
#include "async_conn.hpp"
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace byBit {

class ProactorServer {
public:
    using ConnectedCallback = AsyncConn::ConnectedCallback;
    using MessageCallback   = AsyncConn::MessageCallback;
    using ClosedCallback    = AsyncConn::ClosedCallback;

    explicit ProactorServer(int port)
        : _port(port), _server_fd(-1), _thread_count(4), _running(false), _next_rr(0)
    {}

    ~ProactorServer() {
        if (_server_fd >= 0) close(_server_fd);
    }

    void SetThreadsCount(int n)              { _thread_count = n; }
    void SetConnectedCallback(ConnectedCallback cb) { _conn_cb  = std::move(cb); }
    void SetMessageCallback  (MessageCallback   cb) { _msg_cb   = std::move(cb); }
    void SetClosedCallback   (ClosedCallback    cb) { _close_cb = std::move(cb); }

    // 启动服务器（阻塞，直到进程退出）
    void Start() {
        SetupListenSocket();
        StartWorkers();
        AcceptLoop();       // main thread 运行 accept 循环
    }

private:
    // ─── Worker：一个线程 + 一个 io_uring Proactor ──────────────
    struct Worker {
        std::unique_ptr<Proactor>                     proactor;
        std::thread                                   thread;
        std::mutex                                    pending_mutex;
        std::vector<int>                              pending_fds;
        std::unordered_map<int, PtrAsyncConn>         connections;
        std::atomic<bool>                             running{true};
        uint64_t                                      next_id = 0;

        Worker() : proactor(std::make_unique<Proactor>()) {}
    };

    void SetupListenSocket() {
        _server_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        assert(_server_fd >= 0);
        int opt = 1;
        setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(_server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(_port);
        assert(::bind(_server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        assert(::listen(_server_fd, 1024) == 0);
        LOG(logLevel::INFO) << "ProactorServer listening on port " << _port;
    }

    void StartWorkers() {
        for (int i = 0; i < _thread_count; ++i) {
            auto w = std::make_unique<Worker>();
            Worker* wp = w.get();
            w->thread = std::thread([this, wp]() { WorkerLoop(wp); });
            _workers.push_back(std::move(w));
        }
    }

    // ─── Worker 事件循环 ────────────────────────────────────────
    // 每次循环等待一个 io_uring 完成事件，按类型分发：
    //   WAKEUP → 处理主线程投递的新连接 fd
    //   READ   → 调 AsyncConn::OnReadComplete（数据已在 buf 里）
    //   WRITE  → 调 AsyncConn::OnWriteComplete
    void WorkerLoop(Worker* w) {
        // 注册首次 wakeup read，等待主线程通知新连接
        auto* wakeup_op  = new IoOp{};
        wakeup_op->type  = OpType::WAKEUP;
        w->proactor->SubmitWakeupRead(wakeup_op);

        while (w->running) {
            Proactor::Completion c;
            if (!w->proactor->WaitCompletion(c)) continue;

            auto* op = static_cast<IoOp*>(c.user_data);
            switch (op->type) {

            case OpType::WAKEUP: {
                // 排空主线程投递的新 fd
                std::vector<int> new_fds;
                {
                    std::lock_guard<std::mutex> lock(w->pending_mutex);
                    new_fds.swap(w->pending_fds);
                }
                for (int fd : new_fds) AddConnection(w, fd);
                // 重新注册 wakeup read（同一个 op 复用）
                w->proactor->SubmitWakeupRead(op);
                break;
            }

            case OpType::READ: {
                auto conn = op->conn;                   // 持有引用，防止 OnReadComplete 内析构
                conn->OnReadComplete(op, c.result);     // op 在此被 delete
                if (conn->IsClosed())
                    w->connections.erase(conn->Fd());
                break;
            }

            case OpType::WRITE: {
                auto conn = op->conn;
                conn->OnWriteComplete(op, c.result);    // op 在此被 delete
                if (conn->IsClosed())
                    w->connections.erase(conn->Fd());
                break;
            }

            default:
                delete op;
                break;
            }
        }
    }

    void AddConnection(Worker* w, int fd) {
        auto conn = std::make_shared<AsyncConn>(w->proactor.get(), ++w->next_id, fd);
        conn->SetConnectedCallback(_conn_cb);
        conn->SetMessageCallback(_msg_cb);
        conn->SetClosedCallback(_close_cb);
        // srv_close_cb 在这里不需要额外操作：worker 在 WorkerLoop 里检查 IsClosed() 后 erase
        w->connections[fd] = conn;
        conn->Established();   // 触发 connected 回调 + 提交首次异步 READ
        LOG(logLevel::DEBUG) << "Worker got new conn fd=" << fd << " total=" << w->connections.size();
    }

    // ─── 主线程 Accept 循环 ─────────────────────────────────────
    // 使用 io_uring 异步 accept：提交 ACCEPT → 等完成 → 得到 client_fd → 分发
    // 对比 Reactor 的 Acceptor：Reactor 是 epoll 通知"可接受" → 自己调 accept()
    // 这里是直接让内核 accept 完成后通知我们
    void AcceptLoop() {
        _running = true;
        Proactor acceptor;

        // accept_op 生命期覆盖整个循环，addr 字段被 io_uring 填充
        auto accept_op = std::make_unique<IoOp>();
        accept_op->type = OpType::ACCEPT;
        acceptor.SubmitAccept(_server_fd,
                              &accept_op->client_addr,
                              &accept_op->client_addrlen,
                              accept_op.get());

        while (_running) {
            Proactor::Completion c;
            if (!acceptor.WaitCompletion(c)) continue;

            auto* op      = static_cast<IoOp*>(c.user_data);
            int client_fd = c.result;

            if (client_fd < 0) {
                LOG(logLevel::ERROR) << "accept failed: " << strerror(-client_fd);
            } else {
                LOG(logLevel::INFO) << "Accepted fd=" << client_fd
                                    << " from " << inet_ntoa(op->client_addr.sin_addr);
                Dispatch(client_fd);
            }

            // 重置 addrlen 后立即提交下一次异步 accept
            op->client_addrlen = sizeof(sockaddr_in);
            acceptor.SubmitAccept(_server_fd,
                                  &op->client_addr,
                                  &op->client_addrlen,
                                  op);
        }
    }

    // Round-robin 选 worker，投递 fd 并唤醒 worker
    void Dispatch(int client_fd) {
        int idx    = _next_rr.fetch_add(1) % static_cast<int>(_workers.size());
        Worker* w  = _workers[idx].get();
        {
            std::lock_guard<std::mutex> lock(w->pending_mutex);
            w->pending_fds.push_back(client_fd);
        }
        w->proactor->Wakeup();  // 写 eventfd → worker 的 wakeup read 完成 → 处理 pending_fds
    }

    int                                  _port;
    int                                  _server_fd;
    int                                  _thread_count;
    std::atomic<bool>                    _running;
    std::atomic<int>                     _next_rr;
    std::vector<std::unique_ptr<Worker>> _workers;
    ConnectedCallback                    _conn_cb;
    MessageCallback                      _msg_cb;
    ClosedCallback                       _close_cb;
};

} // namespace byBit
