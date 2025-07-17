#include "../source/server.hpp"

namespace byBit
{
    class EchoServer
    {
    private:
        TcpServer _server;
        void ConnectionMessage(PtrConnection conn, Buffer *in_buffer)
        {
            char buff[1024];
            int n = in_buffer->ReadAndPop(buff, sizeof(buff));
            buff[n] = 0;
            // LOG(logLevel::INFO) << buff;
            std::string mess = buff;
            conn->Send(mess.c_str(), mess.size());
            conn->Shutdown();
        }
        void ConnectionEstablish(PtrConnection conn)
        {
            //LOG(logLevel::INFO) << conn->Id() << " is established !";
        }

        void HandleClose(PtrConnection conn)
        {
            //LOG(logLevel::INFO) << conn->Id() << " is closed!";
        }

    public:
        EchoServer(int port) : _server(port)
        {
            _server.SetClosedCallback(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
            _server.SetConnectedCallback(std::bind(&EchoServer::ConnectionEstablish, this, std::placeholders::_1));
            _server.SetMessageCallback(std::bind(&EchoServer::ConnectionMessage, this, std::placeholders::_1, std::placeholders::_2));
            _server.SetThreadsCount(5);
        }
        void Start(){
            _server.Start();
        }
    };
}