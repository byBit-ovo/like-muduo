#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include "log.hpp"
#include <sys/epoll.h>
#include "channel.hpp"
#include "epoller.hpp"
#include "EventLoop.hpp"
struct epoll_event;
using Task_t = std::function<void()>;
using Remove_t = std::function<void()>;

namespace byBit
{
	class TimerTask
	{
	private:
		uint64_t _id;
		uint32_t _timeout;
		// uint32_t _rounds;
		Task_t _task;
		Remove_t _remove;
		bool _isValid;

	public:
		TimerTask(uint64_t id, Task_t task, uint64_t timeout) : _id(id), _timeout(timeout), _task(task), _isValid(true) {}
		~TimerTask(){
			if (_task &&_isValid){
				_task();
				// LOG(logLevel::DEBUG) << "Task is done";
			}
			if(_remove){
				_remove();
			}
		}
		void Cancel() { _isValid = false; }
		void Activate() { _isValid = true; }
		uint32_t TimeOut() { return _timeout; }
		void SetRemove(Remove_t remove) { _remove = remove; }
		void SetTask(Task_t task) { _task = task; }
	};

	class Timerfd
	{
	private:
		int _fd;
	public:
		Timerfd():_fd(timerfd_create(CLOCK_MONOTONIC, 0)){
			struct itimerspec t;
			t.it_value.tv_nsec = 0;
			t.it_value.tv_sec = 1;
			t.it_interval.tv_nsec = 0;
			t.it_interval.tv_sec = 1;
		    int n = timerfd_settime(_fd, 0, &t, nullptr);
		}
		~Timerfd() { close(_fd); }
		int Fd() { return _fd; }
		void Read(){
			uint64_t val = 0;
			int n = read(_fd, &val, sizeof(uint64_t));
			if(n<0){
				LOG(logLevel::ERROR) << "Timerfd read error...";
			}
		}
	};

	class EventLoop;
	class TimeWheel
	{
	private:
		using Task_Sptr = std::shared_ptr<TimerTask>;
		using Task_Wptr = std::weak_ptr<TimerTask>;
		EventLoop *_loop;                              //monitor
		int _tick;
		int _capacity;
		std::vector<std::vector<Task_Sptr>> _clock;
		std::unordered_map<uint64_t, Task_Wptr> _tasks;
		Timerfd _tfd;                                  //driver
	private:
		void Increment(){
			++_tick;
			_tick %= _capacity;
			_clock[_tick].clear();
		}
		void Remove(uint64_t id) { _tasks.erase(id); }
		void Tick() { _tfd.Read();Increment();}

		void Cancel(uint64_t id){
			auto iter = _tasks.find(id);
			if(iter==_tasks.end()){
				return;
			}
			Task_Sptr sp = iter->second.lock();
			if(sp){
				sp->Cancel();
			}
		}
		void Update(uint64_t id){
			if (_tasks.count(id)){
				Task_Sptr sp = _tasks[id].lock();
				int delay = sp->TimeOut();
				_clock[(_tick + delay) % _capacity].push_back(sp);
				// LOG(logLevel::DEBUG) << "Update the " << id << "task once...";
			}
		}
		void Add(uint64_t id, Task_t task, uint64_t timeout){
			if (_tasks.count(id) == 0){
				Task_Sptr sp = std::make_shared<TimerTask>(id, task, timeout);
				_tasks[id] = sp;
				sp->SetRemove(std::bind(&TimeWheel::Remove, this, id));
				int delay = sp->TimeOut();
				_clock[(_tick + delay) % _capacity].push_back(sp);
				// LOG(logLevel::DEBUG) << "New task" << id << " was inserted into wheel...";
			}
		}
	public:
		TimeWheel(EventLoop *loop) : _loop(loop), _tick(0), _capacity(60), _clock(_capacity),_tfd(){
			Channel_t tch = std::make_shared<Channel>(_tfd.Fd(), _loop);
			tch->SetReadCallback(std::bind(&TimeWheel::Tick,this));
			tch->CareRead();
		}
		void RemoveInLoop(uint64_t id) { _loop->PushTask(std::bind(&TimeWheel::Remove,this,id)); }
		void CancelInLoop(uint64_t id) { _loop->PushTask(std::bind(&TimeWheel::Cancel, this, id)); }
		void UpdateInLoop(uint64_t id) { _loop->PushTask(std::bind(&TimeWheel::Update, this, id)); }
		void AddInLoop(uint64_t id,Task_t t,uint64_t timeout)
		{ _loop->PushTask(std::bind(&TimeWheel::Add, this, id,t,timeout));}

	};
}