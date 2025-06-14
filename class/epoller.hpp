#pragma once
#include "log.hpp"
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

namespace byBit
{
    class Channel;
    using Channel_t = std::shared_ptr<Channel>;
    class Epoller
    {
    private:
        int _epfd;
        int eventnum;
        struct epoll_event _notification[1024];
        std::unordered_map<int, Channel_t> _channels;
    public:
        Epoller():_epfd(-1),eventnum(1024)
        {
            _epfd= epoll_create(8);
            if(_epfd<0){
                LOG(logLevel::FATAL) << "Create Epoll error...";
                exit(1);
            }
        }
        ~Epoller(){
            if(_epfd >=0){
                close(_epfd);
            }
        }
        void Monitor(std::vector<Channel_t> *actives){
            actives->clear();
            LOG(logLevel::INFO) << "Monitoring events...";
            int n = epoll_wait(_epfd, _notification, eventnum, -1);
            LOG(logLevel::INFO) << n << " event" << (n > 1 ? "s are " : " is ") << "ready!";
            if (n > 0)
            {
                for (int i = 0; i < n;++i){
                    int fd = _notification[i].data.fd;
                    LOG(logLevel::INFO) << fd<<" descriptor is ready!";
                    uint32_t ready = _notification[i].events;
                    if(!HasChannel(fd)){
                        LOG(logLevel::INFO) << fd<<"descriptor is not in _channels!";
                        continue;
                    }
                    Channel_t ch = _channels[fd];
                    ch->SetRevents(ready);
                    actives->emplace_back(ch);
                }
            }
        }
        bool Add(Channel_t ch){
            if(!HasChannel(ch)){
                _channels[ch->Fd()] = ch;
                uint32_t care = ch->Events();
                int fd = ch->Fd();
                struct epoll_event ev;
                ev.data.fd = ch->Fd();
                ev.events = care;
                int n = epoll_ctl(_epfd, EPOLL_CTL_ADD, ch->Fd(), &ev);
                if(n<0){
                    LOG(logLevel::ERROR) << "Epoll add the "<<fd<< " descriptor error";
                    return false;
                }else
                {
                    LOG(logLevel::INFO) << "Added the "<<fd<<" to epoll";
                }
                return true;
            }
            return false;
        }
        void Remove(Channel_t ch){
            if(HasChannel(ch)){
                int fd = ch->Fd();
                int n = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
                if(n<0){
                    LOG(logLevel::ERROR) << "Epoll remove the "<<fd<< "descriptor error";
                }
                _channels.erase(fd);
            }
        }
        void UpdateOrAdd(Channel_t ch){
            if(HasChannel(ch)){
                int fd = ch->Fd();
                uint32_t newCare = ch->Events();
                struct epoll_event ev;
                ev.data.fd = fd;
                ev.events = newCare;
                int n = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd,&ev);
                if(n<0){
                    LOG(logLevel::ERROR) << "Epoll Update the "<<fd<< "descriptor error";
                }
            }
            else{
                Add(ch);
            }
        }
        bool HasChannel(Channel_t ch)
        {
            return _channels.count(ch->Fd());
        }
        bool HasChannel(int fd){
            return _channels.count(fd);
        }
    };
}