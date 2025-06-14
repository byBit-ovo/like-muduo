#include "EventLoop.hpp"

namespace byBit
{
    class LoopThread
    {
    private:
        EventLoop *_loop;
        std::thread _thread;
        std::mutex _mutex;
        std::condition_variable _cond;
        void ThreadEntry()
        {
            EventLoop loop; //              although in stack , this function typically does not return;
            {
                std::unique_lock<std::mutex> guard(_mutex);
                _loop = &loop;
                _cond.notify_all();
            }
            _loop->Start(); //              cus this is a loop
        }

    public:
        LoopThread() : _loop(nullptr), _thread(std::bind(&LoopThread::ThreadEntry, this)) {}
        EventLoop *GetLoop()
        {
            if (_loop == nullptr)
            {
                std::unique_lock<std::mutex> guard(_mutex);
                _cond.wait(guard, [&]()
                           { return _loop != nullptr; });
            }
            return _loop;
        }
    };
}