#include "../source/server.hpp"

using namespace byBit;
int main(int argc,char* argv[])
{
    if(argc!=3)
    {
        std::cerr << "Format error" << std::endl;
        exit(0);
    }
    Socket client;
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);
    client.BuildClient(ip, port);
    char in[1024];
    std::string message = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    int n = 10;
    while (n--)
    {
        int ret = client.Send(message.c_str(), message.size(), false);
        if(ret<0){
            exit(0);
        }
        sleep(1);
        int n = client.Read(in, 1023, true);
        in[n] = 0; 
        std::cout << in << std::endl;
    }
    std::cout << "sleeping..." << std::endl;
    int t = 10;
    while (1)
    {
        std::cout << t << std::endl;
        t--;
        sleep(1);
    }
    client.Close();
    return 0;
}