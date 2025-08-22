#include "EventLoop.hpp"
#include "channel.hpp"
#include "Socket.hpp"
namespace byBit
{
    class Acceptor
    {
    private:
        using Handle_accept = std::function<void(int)>;
        Socket _socket;
        Handle_accept _call_back;
        EventLoop *_loop;
        Channel_t _channel;
    public:
        Acceptor(uint16_t port,EventLoop* loop):_loop(loop)
        {
            bool n = _socket.BuildServer(port);
            assert(n != false);
            _channel = std::make_shared<Channel>(_socket.Fd(),_loop);
            _channel->SetReadCallback(std::bind(&Acceptor::accept,this));
        }
        void SetReadCallback(const Handle_accept& cb){
            _call_back = cb;
            _channel->CareRead();//              must be cared after _call_back is set,or fd will be leaked!
        }
        void accept()
        {
            int newfd = _socket.Accept();
            if(newfd < 0){
                return;
            }
            if(_call_back){
                _call_back(newfd);
            }
        }
        int Fd() { return _socket.Fd(); }
    };
}