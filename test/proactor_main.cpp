#include "../proactor/proactor_server.hpp"
using namespace byBit;

// 对比：Reactor 模式下的 echo server（见 test/secondary/tcpserver.cpp）
// 回调签名完全相同，差异完全封装在底层：
//   Reactor: epoll → read() → 回调
//   Proactor: io_uring READ → 完成 → 回调（数据已在 buf 里）

int main() {
    ProactorServer server(9999);
    server.SetThreadsCount(4);

    server.SetConnectedCallback([](const PtrAsyncConn& conn) {
        LOG(logLevel::INFO) << "Connected: fd=" << conn->Fd();
    });

    server.SetMessageCallback([](const PtrAsyncConn& conn, Buffer* buf) {
        // buf 里的数据是内核直接写入的（io_uring READ 完成），无需再调 read()
        std::string data(buf->GetReadPos(), buf->ReadableSize());
        buf->MoveReadOffset(buf->ReadableSize());
        conn->Send(data.c_str(), data.size());   // echo back
    });

    server.SetClosedCallback([](const PtrAsyncConn& conn) {
        LOG(logLevel::INFO) << "Closed: fd=" << conn->Fd();
    });

    server.Start();
    return 0;
}
