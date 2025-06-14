#include "EventLoop.hpp"
namespace byBit
{
    class LoopThreadPool
    {
    private:
        int _thread_count;
        int _next_loop;
        EventLoop *_base_loop;
        std::vector<LoopThread *> _threads;
        std::vector<EventLoop *> _loops;

    public:
        LoopThreadPool(EventLoop *baseloop) : _thread_count(0), _next_loop(0), _base_loop(baseloop) {}
        ~LoopThreadPool()
        {
            for (auto p : _threads)
            {
                delete p;
            }
        }
        void SetCount(int count)
        {
            if (count > 0)
            {
                _thread_count = count;
            }
        }
        void Create()
        {
            if (_thread_count > 0)
            {
                _threads.resize(_thread_count);
                _loops.resize(_thread_count);
                for (int i = 0; i < _thread_count; ++i)
                {
                    _threads[i] = new LoopThread();
                    _loops[i] = _threads[i]->GetLoop();
                }
            }
        }
        EventLoop *Next()
        {
            if (_thread_count == 0)
            {
                return _base_loop;
            }
            EventLoop *next = _loops[_next_loop];
            _next_loop = (_next_loop + 1) % _thread_count;
            return next;
        }
    };
}