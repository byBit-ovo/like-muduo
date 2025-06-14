#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <cstdint>


enum 
{
    SOCKERROR=1,
    BINDERROR,
    ACCEPTERROR,
    LISTENERROR,
    CONNECTERROR
};

#define CONV(addr) (struct sockaddr *)(addr)

#define DIE(x)   \
    do           \
    {            \
        exit(x); \
    } while (0)


class netAddr
{
public:
    netAddr(){}
    netAddr(const std::string &ip, uint16_t port) : _ip(ip), _port(port) // for client
    {
        _addr.sin_family = AF_INET;
        _addr.sin_addr.s_addr = ::inet_addr(_ip.c_str());
        _addr.sin_port = htons(_port);
    }
    netAddr(const sockaddr_in &addr) : _addr(addr)
    {
        ipToHost();
        portToHost();
    }
    netAddr(uint16_t port) : _ip("all ips of this host"), _port(port) // for server
    {
        _addr.sin_family = AF_INET;
        _addr.sin_addr.s_addr = INADDR_ANY;
        _addr.sin_port = htons(_port);
    }
    std::string name() const
    {
        std::string name = getIp();
        name += ' ';
        name += std::to_string(_port);
        return name;
    }
    bool operator==(const netAddr &other) const
    {
        return _ip == other._ip && _port == other._port;
    }
    struct sockaddr *operator&() { return CONV(&_addr); }
    std::string getIp() const { return _ip; }
    uint16_t getPort() const { return _port; }
    socklen_t size() const { return sizeof(_addr); }
    struct sockaddr_in GetAddr() { return _addr; }

private:
    void ipToHost()
    {
        char buffer[1024];
        const char *dst = ::inet_ntop(AF_INET, &_addr.sin_addr, buffer, sizeof(buffer));
        _ip = buffer;
    }
    void portToHost() {_port = ::ntohs(_addr.sin_port);}
    struct sockaddr_in _addr;
    std::string _ip;
    uint16_t _port;
};