#include <sys/socket.h>
#include "../source/inet_addr.hpp"
#include "../source/log.hpp"
#include <fcntl.h>

bool Connect(int fd ,std::string ip, uint16_t port)
{
    netAddr peer(ip, port);
    int n = connect(fd, &peer, peer.size());
    if (n < 0)
    {
        LOG(logLevel::ERROR) << "Connect error...";
        std::cout << strerror(errno) << std::endl;
        return false;
    }
    return true;
}
int Create()
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        LOG(logLevel::ERROR) << "Create socket error...";
        exit(1);
    }
    return fd;
}
bool Bind(int fd,std::string ip, uint16_t port)
{
    netAddr addr(ip, port);
    addr = addr;
    int n = bind(fd, &addr, addr.size());
    if (n < 0)
    {
        LOG(logLevel::ERROR) << "Bind socket error...";
        return false;
    }
    return true;
}

int main()
{
    int fd = Create();
    Bind(fd,"127.0.0.1",8877);
    Connect(fd, "127.0.0.1", 8877);
    return 0;
}