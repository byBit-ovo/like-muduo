#pragma once
#include <unistd.h>
#include <sys/epoll.h>
#include "log.hpp"
#include <functional>

namespace byBit
{
    class EventLoop;
    class Channel
    {
    private:
        int _fd;
        EventLoop *_loop;
        uint32_t _events;
        uint32_t _revents;
        using Callback_t = std::function<void()>;
        Callback_t _read_callback;  
        Callback_t _write_callback; 
        Callback_t _error_callback;
        Callback_t _close_callback; 
        Callback_t _any_callback;
    public:
        Channel( int fd,EventLoop* loop) : _fd(fd),_loop(loop), _events(0), _revents(0) {}
        int Fd() { return _fd; }
        void Close() { close(_fd);
            LOG(logLevel::DEBUG) << "close " << _fd;
        }
        uint32_t Events() { return _events; }                   
        void SetRevents(uint32_t events) { _revents = events; } 
        void SetReadCallback(const Callback_t &cb) { _read_callback = cb; }
        void SetWriteCallback(const Callback_t &cb) { _write_callback = cb; }
        void SetErrorCallback(const Callback_t &cb) { _error_callback = cb; }
        void SetCloseCallback(const Callback_t &cb) { _close_callback = cb; }
        void SetAnyCallback(const Callback_t &cb) { _any_callback = cb; }
        bool ReadAble() { return (_events & EPOLLIN); }
        bool WriteAble() { return (_events & EPOLLOUT); }
        void CareRead()  {_events |= EPOLLIN;UpdateToEpoll();}
        void CareWrite(){_events |= EPOLLOUT; UpdateToEpoll(); }
        void DisableRead() {_events &= ~EPOLLIN;UpdateToEpoll();}
        void DisableWrite() {_events &= ~EPOLLOUT;UpdateToEpoll();}
        void DisableAll() {_events = 0;UpdateToEpoll();}
        void RemoveFromEpoll();
        void UpdateToEpoll();
        void HandleEvents()
        {
            LOG(logLevel::INFO) << "This is in HandleEvents...";
            if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP))
            {
                if (_read_callback){
                    // std::cout <<"This is "<< _fd <<" read callback" << std::endl;
                    _read_callback();
                }
            }

            if ((_revents & EPOLLOUT) && _write_callback)
            {
                _write_callback();
            }
            else if (_revents & EPOLLERR && _error_callback)
            {
                _error_callback();
            }
            else if (_revents & EPOLLHUP && _close_callback)
            {
                _close_callback();
            }
            if (_any_callback) _any_callback();

        }
    };
}