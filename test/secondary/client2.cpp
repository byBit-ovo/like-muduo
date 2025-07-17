#include "../source/server.hpp"

using namespace byBit;
int main(int argc, char *argv[])
{
    signal(SIGCHLD, SIG_IGN);
    if (argc != 3)
    {
        std::cerr << "Format error" << std::endl;
        exit(0);
    }

    char in[1024];
    Socket client;
    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);
    client.BuildClient(ip, port);
    std::string message = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 5\r\n\r\nbyBit";
    message += message;
    message += message;
    message += message;
    while (true)
    {
        int ret = client.Send(message.c_str(), message.size(), false);
        if (ret < 0)
        {
            exit(0);
        }
        int n = client.Read(in, 1023, true);
        if (n > 0)
        {
            in[n] = 0;
            std::cout << in << std::endl;
        }
        else
        {
            exit(0);
        }
        sleep(2);
    }
    client.Close();
    return 0;
}