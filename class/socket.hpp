#include <sys/socket.h>
#include "log.hpp"
#include "inet_addr.hpp"
#include <fcntl.h>

namespace byBit
{
    class Socket
    {
    private:
        int _fd;
        netAddr _addr;

    public:
        Socket(int fd = -1) : _fd(fd) {}
        ~Socket() { Close(); }
        void Close(){ if (_fd >= 0)close(_fd);}
        int Fd() { return _fd; }
        int Read(char* out,int len,bool block){
            int flag = block==true?0:MSG_DONTWAIT;
            int n = recv(_fd, out, len, flag);
            if(n<0){
                if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR){
                    return 0;
                }
                return -1;
            }
            out[n] = 0;
            return n;
        }
        int Send(const char* in,int len,bool block)
        {
            int flag = block == true ? 0 : MSG_DONTWAIT;
            int n = send(_fd, in, len, flag);
            if(n<0){
                if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR){
                    return 0;
                }
                return -1;
            }
            return n;
        }
        void Create()
        {
            _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (_fd < 0)
            {
                LOG(logLevel::ERROR) << "Create socket error...";
                exit(1);
            }
        }
        bool Bind(std::string ip, uint16_t port)
        {
            netAddr addr(ip, port);
            _addr = addr;
            int n = bind(_fd, &_addr, _addr.size());
            if (n < 0)
            {
                LOG(logLevel::ERROR) << "Bind socket error...";
                return false;
            }
            return true;
        }
        bool Listen()
        {
            int n = ::listen(_fd, 8);
            if(n<0){
                LOG(logLevel::ERROR) << "Set listen status error...";
                return false;
            }
            return true;
        }
        int Accept(){
            int fd = accept(_fd, nullptr, nullptr);
            if(_fd<0){
                LOG(logLevel::ERROR) << "Accept error...";
            }
            return fd;
        }
        bool Connect(std::string ip, uint16_t port){
            netAddr peer(ip, port);
            int n = connect(_fd, &peer, peer.size());
            if(n<0){
                LOG(logLevel::ERROR) << "Connect error...";
                std::cout << strerror(errno) << std::endl;
                return false;
            }
            return true;
        }
        void SetNoBlock(){
            int flag = fcntl(_fd, F_GETFL, 0);
            fcntl(_fd, F_SETFL, flag | O_NONBLOCK);
        }
        void ReuseAddress() {
            // int setsockopt(int fd, int leve, int optname, void *val, int vallen)
            int val = 1;
            setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&val, sizeof(int));
            setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, (void*)&val, sizeof(int));
        }
        bool BuildServer(std::string ip, uint16_t port,bool isBlock,bool isReuse){
            Create();
            if(!Bind(ip,port)) return false;
            if(isBlock) SetNoBlock();
            if(isReuse) ReuseAddress();
            if(!Listen()) return false;
            return true;
        }
        bool BuildClient(std::string ip, uint16_t port){
            Create();
            return Connect(ip, port);
        }
    };
}