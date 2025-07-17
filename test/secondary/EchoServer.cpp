#include "EchoServer.hpp"
using namespace byBit;
int main(int argc, char *argv[])
{
    if(argc != 2){
        LOG(logLevel::ERROR) << "Format error";
        exit(1);
    }
    uint16_t port = std::stoi(argv[1]);
    EchoServer server(port);
    server.Start();
    return 0;
}