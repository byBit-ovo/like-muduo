#pragma once
/*
 * Proactor vs Reactor 核心区别
 *
 * Reactor (本项目当前实现):
 *   epoll_wait() → "fd 可读了，你自己去 read()" → 用户代码调 read()
 *
 * Proactor (本文件实现):
 *   submit_read(fd, buf) → 内核把数据读到 buf → "读完了，数据已在 buf 里"
 *   I/O 由内核完成，用户只处理完成事件
 *
 * Linux 上实现真正 Proactor 的机制: io_uring (kernel 5.1+)
 *   SQ (Submission Queue): 提交异步操作
 *   CQ (Completion Queue): 接收完成通知
 */
#include <liburing.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

namespace byBit {

// 每个 io_uring 实例绑定到单个线程（io_uring 本身不是线程安全的）
class Proactor {
public:
    struct Completion {
        void* user_data;
        int   result;    // 成功: bytes transferred; 失败: -errno
    };

    explicit Proactor(int queue_depth = 256) {
        int ret = io_uring_queue_init(queue_depth, &_ring, 0);
        assert(ret == 0 && "io_uring_queue_init failed");
        _wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        assert(_wakeup_fd >= 0);
    }

    ~Proactor() {
        io_uring_queue_exit(&_ring);
        close(_wakeup_fd);
    }

    Proactor(const Proactor&)            = delete;
    Proactor& operator=(const Proactor&) = delete;

    // 提交异步 accept；完成时 result = 新连接 fd（或 -errno）
    void SubmitAccept(int server_fd, sockaddr_in* addr, socklen_t* addrlen, void* user_data) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        assert(sqe);
        io_uring_prep_accept(sqe, server_fd, reinterpret_cast<sockaddr*>(addr), addrlen, SOCK_CLOEXEC);
        io_uring_sqe_set_data(sqe, user_data);
        io_uring_submit(&_ring);
    }

    // 提交异步读；完成时 buf 里已有数据，result = 读到的字节数（或 -errno / 0=EOF）
    void SubmitRead(int fd, char* buf, size_t len, void* user_data) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        assert(sqe);
        io_uring_prep_read(sqe, fd, buf, static_cast<unsigned>(len), 0);
        io_uring_sqe_set_data(sqe, user_data);
        io_uring_submit(&_ring);
    }

    // 提交异步写；完成时 result = 写出的字节数（或 -errno）
    void SubmitWrite(int fd, const char* buf, size_t len, void* user_data) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        assert(sqe);
        io_uring_prep_write(sqe, fd, buf, static_cast<unsigned>(len), 0);
        io_uring_sqe_set_data(sqe, user_data);
        io_uring_submit(&_ring);
    }

    // 向自身的 eventfd 注册一次读，用于跨线程唤醒（worker 收到新连接通知）
    // user_data 原样返回，_wakeup_buf 作为读缓冲（Proactor 生命周期内有效）
    void SubmitWakeupRead(void* user_data) {
        io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
        assert(sqe);
        io_uring_prep_read(sqe, _wakeup_fd, &_wakeup_buf, sizeof(_wakeup_buf), 0);
        io_uring_sqe_set_data(sqe, user_data);
        io_uring_submit(&_ring);
    }

    // 从另一个线程唤醒 worker（写 eventfd，触发上面注册的 wakeup read）
    void Wakeup() {
        uint64_t val = 1;
        write(_wakeup_fd, &val, sizeof(val));
    }

    // 阻塞等待一个完成事件
    bool WaitCompletion(Completion& out) {
        io_uring_cqe* cqe = nullptr;
        int ret = io_uring_wait_cqe(&_ring, &cqe);
        if (ret < 0) return false;
        out = { io_uring_cqe_get_data(cqe), cqe->res };
        io_uring_cqe_seen(&_ring, cqe);
        return true;
    }

private:
    io_uring _ring;
    int      _wakeup_fd;
    uint64_t _wakeup_buf = 0;  // eventfd 读缓冲，生命周期与 Proactor 相同
};

} // namespace byBit
