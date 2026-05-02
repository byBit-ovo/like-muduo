#pragma once
#include "proactor.hpp"
#include "../class/buffer.hpp"
#include "../class/log.hpp"
#include <memory>
#include <functional>
#include <atomic>
#include <cstring>
#include <unistd.h>

namespace byBit {

class AsyncConn;
using PtrAsyncConn = std::shared_ptr<AsyncConn>;

// 每个挂起的 io_uring 操作对应一个堆上的 IoOp
// 提交前 new，完成处理后 delete（由完成回调负责）
enum class OpType : uint8_t { ACCEPT, READ, WRITE, WAKEUP };

struct IoOp {
    OpType   type;
    int      fd;
    // buf 必须在 io_uring 操作期间保持有效，堆分配保证了这一点
    static constexpr size_t BUF_SZ = 65536;
    char     buf[BUF_SZ];
    // ACCEPT 专用字段
    sockaddr_in client_addr{};
    socklen_t   client_addrlen = sizeof(sockaddr_in);
    // READ/WRITE 专用：持有连接的 shared_ptr，保证操作期间连接不被销毁
    std::shared_ptr<AsyncConn> conn;
};

// ─────────────────────────────────────────────────────────
// AsyncConn: 单个 TCP 连接的异步 I/O 管理
//
// 与 Reactor 模式下 Connection 的对比：
//   Reactor Connection:  epoll 通知"可读" → 自己调 read()
//   AsyncConn:           提交 io_uring READ → 内核填好 buf → OnReadComplete()
//
// 所有方法必须在 worker 线程调用（单线程模型，无锁）
// ─────────────────────────────────────────────────────────
class AsyncConn : public std::enable_shared_from_this<AsyncConn> {
public:
    using ConnectedCallback = std::function<void(const PtrAsyncConn&)>;
    using MessageCallback   = std::function<void(const PtrAsyncConn&, Buffer*)>;
    using ClosedCallback    = std::function<void(const PtrAsyncConn&)>;

    AsyncConn(Proactor* proactor, uint64_t id, int fd)
        : _proactor(proactor), _id(id), _fd(fd), _closed(false), _writing(false)
    {}

    ~AsyncConn() {
        // DoClose 已经 close(_fd)，这里只处理未经 DoClose 直接析构的情况
        if (!_closed.exchange(true)) close(_fd);
        LOG(logLevel::INFO) << "AsyncConn " << _id << " destroyed";
    }

    uint64_t Id()     const { return _id; }
    int      Fd()     const { return _fd; }
    bool     IsClosed() const { return _closed; }

    void SetConnectedCallback(ConnectedCallback cb) { _conn_cb   = std::move(cb); }
    void SetMessageCallback  (MessageCallback   cb) { _msg_cb    = std::move(cb); }
    void SetClosedCallback   (ClosedCallback    cb) { _close_cb  = std::move(cb); }
    void SetSrvClosedCallback(ClosedCallback    cb) { _srv_close = std::move(cb); }

    // 建立连接后调用一次，启动首次异步读
    void Established() {
        if (_conn_cb) _conn_cb(shared_from_this());
        SubmitRead();
    }

    // 发送数据（在 worker 线程调用）
    void Send(const char* data, size_t len) {
        if (_closed) return;
        _out_buf.WriteAndPush(data, len);
        if (!_writing) FlushWrite();
    }

    // ── 完成回调，由 worker 的事件循环调用 ──────────────────

    // io_uring READ 完成：op->buf 里已有 result 字节数据（内核写入，无需再 read()）
    void OnReadComplete(IoOp* op, int result) {
        if (result <= 0) {
            delete op;
            DoClose();
            return;
        }
        _in_buf.WriteAndPush(op->buf, result);
        delete op;
        if (_msg_cb) _msg_cb(shared_from_this(), &_in_buf);
        // 链式提交下一次读
        if (!_closed) SubmitRead();
    }

    // io_uring WRITE 完成
    void OnWriteComplete(IoOp* op, int result) {
        delete op;
        _writing = false;
        if (result <= 0) {
            DoClose();
            return;
        }
        if (_out_buf.ReadableSize() > 0) FlushWrite();
    }

private:
    // 提交一次异步读；内核将数据填入 op->buf，完成后调 OnReadComplete
    void SubmitRead() {
        auto* op  = new IoOp{};
        op->type  = OpType::READ;
        op->fd    = _fd;
        op->conn  = shared_from_this();   // 保证 READ 完成前连接不析构
        _proactor->SubmitRead(_fd, op->buf, IoOp::BUF_SZ, op);
    }

    // 提交一次异步写；数据拷贝到 op->buf，内核负责发送
    void FlushWrite() {
        if (_out_buf.ReadableSize() == 0 || _closed) return;
        _writing  = true;
        auto* op  = new IoOp{};
        op->type  = OpType::WRITE;
        op->fd    = _fd;
        op->conn  = shared_from_this();
        size_t chunk = std::min<size_t>(_out_buf.ReadableSize(), IoOp::BUF_SZ);
        memcpy(op->buf, _out_buf.GetReadPos(), chunk);
        _out_buf.MoveReadOffset(chunk);
        _proactor->SubmitWrite(_fd, op->buf, chunk, op);
    }

    void DoClose() {
        if (_closed.exchange(true)) return;
        close(_fd);
        if (_close_cb)  _close_cb(shared_from_this());
        if (_srv_close) _srv_close(shared_from_this());
    }

    Proactor*   _proactor;
    uint64_t    _id;
    int         _fd;
    std::atomic<bool> _closed;
    bool        _writing;
    Buffer      _in_buf;
    Buffer      _out_buf;
    ConnectedCallback _conn_cb;
    MessageCallback   _msg_cb;
    ClosedCallback    _close_cb;
    ClosedCallback    _srv_close;
};

} // namespace byBit
