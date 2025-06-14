#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include "epoller.hpp"
#include "channel.hpp"
#include <sys/eventfd.h>
#include "timerwheel.hpp"

namespace byBit
{
    class Eventfd
    {
    private:
        int _fd;

    public:
        Eventfd() : _fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {}
        ~Eventfd() { close(_fd); }
        int Fd() { return _fd; }
        void Awake(uint64_t val = 1)
        {
            int n = write(_fd, &val, sizeof(uint64_t));
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    return;
                }
                LOG(logLevel::ERROR) << "Awake error...";
                abort();
            }
        }
        void Read()
        {
            uint64_t val = 0;
            int n = read(_fd, &val, sizeof(uint64_t));
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EINTR)
                {
                    return;
                }
                else
                {
                    LOG(logLevel::ERROR) << "Eventfd read error...";
                    abort();
                }
            }
        }
    };

    class EventLoop
    {
    private:
        using Task_t = std::function<void()>;
        std::thread::id _tid;
        Eventfd _event_fd; // to awake thread
        Epoller _poller;   // event driver
        std::vector<Task_t> _tasks;
        std::mutex _mutex;
        Channel_t _event_ch; // to awake thread
        TimeWheel _wheel;    // we may need timecount task to disconnect lazy link

    public:
        EventLoop() : _tid(std::this_thread::get_id()),
                      _event_ch(std::make_shared<Channel>(_event_fd.Fd(), this)), _wheel(this)
        {
            _event_ch->SetReadCallback(std::bind(&Eventfd::Read, &_event_fd));
            _event_ch->SetWriteCallback(std::bind(&Eventfd::Awake, &_event_fd, 1));
            _event_ch->CareRead();
            _poller.Add(_event_ch);
        }
        ~EventLoop() {}
        void Start()
        {
            while (true)
            {
                std::vector<Channel_t> pending;
                _poller.Monitor(&pending);
                for (auto &ch : pending)
                {
                    ch->HandleEvents();
                }
                RunTasks();
            }
        }
        bool isInLoop() { return std::this_thread::get_id() == _tid; }
        void RunInLoop(const Task_t &t)
        {
            if (isInLoop())
            {
                t();
                return;
            }
            PushTask(t);
        }

        void PushTask(const Task_t &t)
        {
            {
                std::unique_lock<std::mutex> guard(_mutex);
                _tasks.push_back(t);
            }
            _event_fd.Awake();
        }

        void RunTasks()
        {
            std::vector<Task_t> pending;
            {
                std::unique_lock<std::mutex> guard(_mutex);
                pending.swap(_tasks);
            }
            for (auto &t : pending)
            {
                t();
            }
        }
    };
    void UpdateEvent(Channel_t ch) { _poller.UpdateOrAdd(ch); }
    void RemoveEvent(Channel_t ch) { _poller.Remove(ch); }
    // void AddToEpoll(Channel_t ch) { _poller.Add(ch); }
    void AddTimer(uint64_t id, Task_t task, uint64_t timeout) { _wheel.AddInLoop(id, task, timeout); }
    void CancelTimer(uint64_t id) { _wheel.CancelInLoop(id); }
    void UpdateTimer(uint64_t id) { _wheel.UpdateInLoop(id); }
    bool HasTimer(uint64_t id) { return _wheel.HasTimer(id); }
    // class EventLoop
    // {
    // private:
    //     using Task_t = std::function<void()>;
    //     std::thread::id _tid;
    //     Eventfd _event_fd;
    //     Epoller _poller;
    //     std::vector<Task_t> _tasks;
    //     std::mutex _mutex;
    //     Channel_t _event_ch;

    // public:
    //     EventLoop():_tid(std::this_thread::get_id()),_event_ch(std::make_shared<Channel>(_event_fd.Fd(),this)){
    //         _event_ch->SetReadCallback(std::bind(&Eventfd::Read, &_event_fd));
    //         _event_ch->SetWriteCallback(std::bind(&Eventfd::Awake, &_event_fd, 1));
    //         _event_ch->CareRead();
    //         _poller.Add(_event_ch);
    //     }
    //     ~EventLoop(){}
    //     void Start(){
    //         while(true){
    //             std::vector<Channel_t> pending;
    //             _poller.Monitor(&pending);
    //             for(auto& ch:pending){
    //                 ch->HandleEvents();
    //             }
    //             RunTasks();
    //         }
    //     }
    //     bool isInLoop() { return std::this_thread::get_id() == _tid; }
    //     void RunInLoop(const Task_t& t){
    //         if(isInLoop()){
    //             t();
    //             return;
    //         }
    //         PushTask(t);
    //     }

    //     void PushTask(const Task_t& t){
    //         {
    //             std::unique_lock<std::mutex> guard(_mutex);
    //             _tasks.push_back(t);
    //         }
    //         _event_fd.Awake();
    //     }

    //     void RunTasks(){
    //         std::vector<Task_t> pending;
    //         {
    //             std::unique_lock<std::mutex> guard(_mutex);
    //             pending.swap(_tasks);
    //         }
    //         for(auto& t:pending){
    //             t();
    //         }
    //     }

    //     void UpdateEvent(Channel_t ch) { _poller.UpdateOrAdd(ch); }
    //     void RemoveEvent(Channel_t ch) { _poller.Remove(ch); }
    //     void AddToEpoll(Channel_t ch) { _poller.Add(ch); }
    // };
    // void Channel::RemoveFromEpoll() {_loop->RemoveEvent(std::make_shared<Channel>(*this));}
    // void Channel::UpdateToEpoll()  { _loop->UpdateEvent(std::make_shared<Channel>(*this)); }
}