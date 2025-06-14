#include "../source/server.hpp"
using namespace byBit; 
void ConnectionMessage(PtrConnection conn,Buffer* in_buffer)
{
    char buff[1024];
    int n = in_buffer->ReadAndPop(buff,sizeof(buff));
    buff[n] = 0;
    LOG(logLevel::INFO) << buff;
    std::string mess = buff;
    mess += " too! ";
    conn->Send(mess.c_str(), mess.size());
}
void ConnectionEstablish(PtrConnection conn){
    LOG(logLevel::INFO) << conn->Id() << " is established !";
}

void HandleClose(PtrConnection conn){
    LOG(logLevel::INFO) << conn->Id() << " is closed!";
}
int main(int argc, char *argv[])
{
    if(argc != 2){
        LOG(logLevel::ERROR) << "Format error";
        exit(1);
    }
    uint16_t port = std::stoi(argv[1]);
    // test2(port);
    TcpServer server(port);
    server.SetThreadsCount(3);
    server.EnableInactiveRelease(20);
    server.SetConnectedCallback(ConnectionEstablish);
    server.SetMessageCallback(ConnectionMessage);
    server.SetClosedCallback(HandleClose);
    server.Start();

    return 0;
}
