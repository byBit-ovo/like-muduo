#ifndef _MY_SERVER_H__
#define _MY_SERVER_H__
#include "inet_addr.hpp"
#include "log.hpp"
#include <unistd.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <assert.h>
#include <typeinfo>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <functional>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <signal.h>
#define DEFAULTSIZE 1024
#define DEFAULT_TIMEOUT 10
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
        void Close(){
            // std::cout << "close fd is" << _fd << std::endl;
            if (_fd >= 0)
                close(_fd);
            _fd = -1;
        }
        int Fd() { return _fd; }
        int Read(char* out,int len,bool block){
            // LOG(logLevel::DEBUG) << "socket reading...";
            int flag = block == true ? 0 : MSG_DONTWAIT;
            int n = recv(_fd, out, len, flag);
            if(n<0){
                if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR){
                    return 0;
                }
                return -1;
            }
            out[n] = 0;
            // LOG(logLevel::DEBUG) << "socket read over...";
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
        bool Bind(uint16_t port)
        {
            netAddr addr(port);
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
            int a =setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&val, sizeof(int));
            if(a<0){
                LOG(logLevel::ERROR) << "Set " << _fd << " addr reuse error";
            }
            int b = setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, (void*)&val, sizeof(int));
            if(b<0){
                LOG(logLevel::ERROR) << "Set " << _fd << " port reuse error";
            }
        }
        bool BuildServer(uint16_t port,std::string ip="0.0.0.0",bool isBlock=false,bool isReuse=true){
            Create();
            if(!Bind(ip,port)) return false;
            if(!isBlock) SetNoBlock();
            if(isReuse) ReuseAddress();
            if(!Listen()) return false;
            return true;
        }
        bool BuildClient(std::string ip, uint16_t port){
            Create();
            return Connect(ip, port);
        }
    };

    class Any
    {
    private:
        class holder
        {
        public:
            virtual ~holder() {}
            virtual holder *clone() = 0;
            virtual const std::type_info &type() = 0;
        };

        template <class T>
        class placeholder : public holder
        {
        public:
            T _val;

        public:
            placeholder<T>(const T &val) : _val(val) {}
            virtual ~placeholder<T>() override {}
            virtual holder *clone() override { return new placeholder<T>(_val); }
            virtual const std::type_info &type() { return typeid(_val); }
        };

    private:
        holder *_dummy;

    public:
        Any() : _dummy(nullptr) {}
        template <class T>
        Any(const T &val) : _dummy(new placeholder<T>(val)) {}
        Any(const Any &other) : _dummy(other._dummy ? (other._dummy->clone()) : nullptr) {}
        ~Any() { delete _dummy; }
        void swap(Any &other)
        {
            std::swap(_dummy, other._dummy);
        }
        Any &operator=(const Any &other)
        {
            Any(other).swap(*this);
            return *this;
        }
        template <class T>
        Any &operator=(const T &val)
        {
            Any(val).swap(*this);
            return *this;
        }
        template <class T>
        T *get()
        {
            if (_dummy == nullptr || _dummy->type() != typeid(T))
            {
                return nullptr;
            }
            return &((placeholder<T> *)_dummy)->_val;
            // return &(((placeholder<T> *)_dummy)->_val);
            // return &((placeholder<T>*)_dummy)->_val;
        }
    };

class Buffer
{
private:
    int _write_idx;
    int _read_idx;
    std::vector<char> _buffer;

public:
    Buffer():_write_idx(0),_read_idx(0),_buffer(DEFAULTSIZE){}
    char *GetWritePos() { return &_buffer[0] + _write_idx; }
    const char *GetReadPos() const{ return &_buffer[0] + _read_idx; }
    int Capacity() { return _buffer.size(); }
    int PostIdle() { return Capacity() - _write_idx; }
    int PreIdle() { return _read_idx; }
    int ReadableSize() const{ return _write_idx - _read_idx; }
    void MoveReadOffset(int len) {
        assert(len <= ReadableSize());
        _read_idx += len;
    }
    void MoveWriteOffset(int len) {
        if(len>=PostIdle()){
            LOG(logLevel::DEBUG) << "len: " << len << " PostIdle: " << PostIdle();
            abort();
        }
        _write_idx += len;
    }
    void Clear() { _read_idx = _write_idx = 0; }
    void EnsureWrite(int len){
        if(PostIdle() > len){
            return;
        }
        if(PreIdle()+PostIdle()>len){
            size_t size = ReadableSize();
            std::copy(GetReadPos(), GetReadPos() + ReadableSize(), _buffer.begin());
            _read_idx = 0;
            _write_idx = size;
        }else{
            _buffer.resize(Capacity() + 2*len);
        }
    }
    int Write(const void* data,int len){
        EnsureWrite(len);
        const char *Data = (const char*)data;
        std::copy(Data, Data + len, GetWritePos());
        return len;
    }
    int Write(const Buffer& other){
        return Write(other.GetReadPos(), other.ReadableSize());
    }
    int Write(const std::string& s){
        return Write(s.c_str(), s.size());
    }
    int WriteAndPush(const void* data,int len){
        Write(data, len);
        MoveWriteOffset(len);
        return len;
    }
    int WriteAndPush(const std::string& str){
        return WriteAndPush(str.c_str(), str.size());
    }
    int WriteAndPush(const Buffer& other){
        return WriteAndPush(other.GetReadPos(), other.ReadableSize());
    }
    int Read(void* d,int len){
        int lenth = std::min(ReadableSize(), len);
        char *data = (char *)d;
        std::copy(GetReadPos(), GetReadPos() + lenth, data);
        return lenth;
    }
    int Read(std::string* out){
        if(out->empty())
            return 0;
        char *ot = &(*out)[0];
        return Read(ot, out->size());
    }
    int ReadAndPop(void* d,int len){
        int l = Read(d, len);
        MoveReadOffset(l);
        return l;
    }
    int ReadAndPop(std::string* out){
        if(out->empty())
            return 0;
        int len = out->size();
        char bu[len];
        int lenth = ReadAndPop(bu,len);
        *out = bu;
        return lenth;
    }
    char* FindCRLF(){
        char *pos = (char *)memchr(GetReadPos(), '\n', ReadableSize());
        return pos;
    }
    std::string GetLine(){
        char *pos = FindCRLF();
        if (pos == nullptr)
            return "";
        int len = pos - GetReadPos() + 1;
        char buf[len+1];
        Read(buf, len);
        buf[len] = 0;
        return buf;
    }
    std::string GetLineAndPop(){
        std::string line = GetLine();
        int len = line.size();
        MoveReadOffset(len);
        return line;
    }
};

     
    class EventLoop;
    class Channel:public std::enable_shared_from_this<Channel>
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
        void Close() { close(_fd); }
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
            // LOG(logLevel::INFO) << "This is in HandleEvents...";
            if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP))
            {
                if (_read_callback){
                    _read_callback();
                }
            }

            if ((_revents & EPOLLOUT) && _write_callback)
            {
                _write_callback();
            }
            else if ((_revents & EPOLLERR) && _error_callback)
            {
                _error_callback();
            }
            else if ((_revents & EPOLLHUP) && _close_callback)
            {
                _close_callback();
            }
            if (_any_callback) _any_callback();

        }
    };

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
                _epfd = -1;
            }
        }
        void Monitor(std::vector<Channel_t> *actives){
            actives->clear();
            // LOG(logLevel::INFO) << "Monitoring events...";
            int n = epoll_wait(_epfd, _notification, eventnum, -1);
            // LOG(logLevel::INFO) << n << " event" << (n > 1 ? "s are " : " is ") << "ready!";
            if (n > 0)
            {
                for (int i = 0; i < n;++i){
                    int fd = _notification[i].data.fd;
                    // LOG(logLevel::INFO) << fd<<" descriptor is ready!";
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
                _channels.insert(std::make_pair(ch->Fd(), ch));
                uint32_t care = ch->Events();
                int fd = ch->Fd();  
                struct epoll_event ev;
                ev.data.fd = ch->Fd();
                ev.events = care;
                int n = epoll_ctl(_epfd, EPOLL_CTL_ADD, ch->Fd(), &ev);
                if(n<0){
                    perror("error:");
                    LOG(logLevel::ERROR) << "Epoll add the " << fd << " descriptor error";
                    return false;
                }
                // else{
                    // LOG(logLevel::INFO) << "Added the "<<fd<<" to epoll";
                // }
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
                // LOG(logLevel::ERROR) << "Epoll remove the "<<fd<< " descriptor";
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
                    LOG(logLevel::ERROR) << "Epoll Update the "<<fd<< " descriptor error";
                    LOG(logLevel::ERROR) << strerror(errno);
                }
            }else{
                Add(ch);
            }
        }
        bool HasChannel(Channel_t ch)
        {
            auto iter = _channels.find(ch->Fd());
            if(iter==_channels.end()){
                return false;
            }
            return true;
        }
        bool HasChannel(int fd){
            auto iter = _channels.find(fd);
            if(iter==_channels.end()){
                return false;
            }
            return true;
        }
    };

    using Task_t = std::function<void()>;
    using Remove_t = std::function<void()>;
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
		int Read(){
			uint64_t val = 0;
			int n = read(_fd, &val, sizeof(uint64_t));
			if(n<0){
				LOG(logLevel::ERROR) << "Timerfd read error...";
			}
            return val;
        }
    };


    
    class Eventfd
    {
    private:
        int _fd;
    public:
        Eventfd():_fd(eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK)){}
        ~Eventfd(){close(_fd);}
        int Fd() { return _fd; }
        void Awake(uint64_t val=1){
            int n = write(_fd, &val, sizeof(uint64_t));
            if(n<0){
                if(errno==EINTR){
                    return;
                }
                LOG(logLevel::ERROR) << "Awake error...";
                abort();
            }
        }
        void Read(){
            uint64_t val = 0;
            int n = read(_fd, &val, sizeof(uint64_t));
            if(n<0){
                if(errno==EAGAIN || errno==EINTR){
                    return;
                }
                else{
                    LOG(logLevel::ERROR) << "Eventfd read error...";
                    abort();
                }
            }
        }
    };
    
    
	class TimeWheel
	{
	private:
		using Task_Sptr = std::shared_ptr<TimerTask>;
		using Task_Wptr = std::weak_ptr<TimerTask>;
		EventLoop* _loop;
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
		void Tick() {
            int times = _tfd.Read();
            for (int i = 0; i < times;++i){
                Increment();
            }
        }

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
        bool HasTimer(uint64_t id) { return _tasks.count(id); }
        void RemoveInLoop(uint64_t id);
		void CancelInLoop(uint64_t id);
		void UpdateInLoop(uint64_t id);
        void AddInLoop(uint64_t id, Task_t t, uint64_t timeout);
    };

    class EventLoop
    {
    private:
        using Task_t = std::function<void()>;
        std::thread::id _tid;
        Eventfd _event_fd;                   //to awake thread
        Epoller _poller;                     //event driver , manage fds
        std::vector<Task_t> _tasks;
        std::mutex _mutex;                  //实测如果这把锁不加，程序瞬间崩
        Channel_t _event_ch;                //to awake thread
        TimeWheel _wheel;                   //we may need timecount task to disconnect lazy link

    public:
        EventLoop():_tid(std::this_thread::get_id()),
            _event_ch(std::make_shared<Channel>(_event_fd.Fd(),this)),_wheel(this){
            _event_ch->SetReadCallback(std::bind(&Eventfd::Read, &_event_fd));
            _event_ch->SetWriteCallback(std::bind(&Eventfd::Awake, &_event_fd, 1));
            _event_ch->CareRead();
        }
        void Start(){
            while(true){
                std::vector<Channel_t> pending;
                _poller.Monitor(&pending);
                for(auto& ch:pending){
                    ch->HandleEvents();
                }
                RunTasks();
            }
        }
        bool isInLoop() { return std::this_thread::get_id() == _tid; }
        void RunInLoop(const Task_t& t){
            if(isInLoop()){
                t();
                return;
            }
            PushTask(t);
        }

        void PushTask(const Task_t& t){
            {
                std::unique_lock<std::mutex> guard(_mutex);
                _tasks.push_back(t);
            }
            _event_fd.Awake();
        }

        void RunTasks(){
            std::vector<Task_t> pending;
            {
                std::unique_lock<std::mutex> guard(_mutex);
                pending.swap(_tasks);
            }
            for(auto& t:pending){
                t();
            }
        }
        
        void UpdateEvent(const Channel_t& ch) { _poller.UpdateOrAdd(ch); }
        void RemoveEvent(const Channel_t& ch) { _poller.Remove(ch); }
        // void AddToEpoll(Channel_t ch) { _poller.Add(ch); }
        void AddTimer(uint64_t id, Task_t task, uint64_t timeout) { _wheel.AddInLoop(id, task, timeout); }
        void CancelTimer(uint64_t id){_wheel.CancelInLoop(id);}
        void UpdateTimer(uint64_t id) { _wheel.UpdateInLoop(id); }
        bool HasTimer(uint64_t id) { return _wheel.HasTimer(id); }
    };

    // void Channel::RemoveFromEpoll() {_loop->RemoveEvent(std::make_shared<Channel>(*this));}
    // void Channel::UpdateToEpoll()  { _loop->UpdateEvent(std::make_shared<Channel>(*this)); }
    void Channel::RemoveFromEpoll() {_loop->RemoveEvent(shared_from_this());}
    void Channel::UpdateToEpoll()  { _loop->UpdateEvent(shared_from_this()); }
    void TimeWheel::RemoveInLoop(uint64_t id) { _loop->PushTask(std::bind(&TimeWheel::Remove,this,id)); }
    void TimeWheel::CancelInLoop(uint64_t id) { _loop->PushTask(std::bind(&TimeWheel::Cancel, this, id)); }
    void TimeWheel::UpdateInLoop(uint64_t id) { _loop->PushTask(std::bind(&TimeWheel::Update, this, id)); }
    void TimeWheel::AddInLoop(uint64_t id,Task_t t,uint64_t timeout)
    { _loop->PushTask(std::bind(&TimeWheel::Add, this, id,t,timeout));}


    class LoopThread
    {
    private:
        EventLoop* _loop;
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
        LoopThread():_loop(nullptr),_thread(std::bind(&LoopThread::ThreadEntry,this)){}
        EventLoop* GetLoop(){
            if(_loop==nullptr){
                std::unique_lock<std::mutex> guard(_mutex);
                _cond.wait(guard, [&](){ return _loop != nullptr; });
            }
            return _loop;
        }
    };

    class LoopThreadPool
    {
    private:
        int _thread_count;
        int _next_loop;
        EventLoop *_base_loop;
        std::vector<LoopThread*> _threads;
        std::vector<EventLoop *> _loops;
    public:
        LoopThreadPool(EventLoop* baseloop):_thread_count(0),_next_loop(0),_base_loop(baseloop){}
        ~LoopThreadPool(){
            for(auto p:_threads){
                delete p;
            }
        }
        void SetCount(int count) {
            if(count>0){
                _thread_count = count;
            }            
        }
        void Create(){
            if(_thread_count>0){
                _threads.resize(_thread_count);
                _loops.resize(_thread_count);
                for (int i = 0;i<_thread_count;++i){
                    _threads[i] = new LoopThread();
                    _loops[i] = _threads[i]->GetLoop();
                }
            }
        }
        EventLoop* Next()
        {
            if(_thread_count==0){
                return _base_loop;
            }
            EventLoop *next = _loops[_next_loop];
            _next_loop = (_next_loop + 1) % _thread_count;
            return next;
        }
    };
    typedef enum
    {
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
        DISCONNECTING
    } ConnStat;
    class Connection;
    using PtrConnection = std::shared_ptr<Connection>;
    class Connection: public std::enable_shared_from_this<Connection>
    {
    private:
        uint64_t _id;          //also be used as timerid
        int _sock_fd;
        Socket _sock;
        EventLoop *_loop;      //master
        bool _releaseOrNot;
        Channel_t _channel;    
        Buffer _in_buffer;
        Buffer _out_buffer;
        Any _context;
        ConnStat _status;
        using ConnectedCallback = std::function<void(const PtrConnection&)>;
        using MessageCallback = std::function<void(const PtrConnection&, Buffer *)>;
        using ClosedCallback = std::function<void(const PtrConnection&)>;
        using AnyEventCallback = std::function<void(const PtrConnection&)>;
        ConnectedCallback _connected_callback;
        MessageCallback _message_callback;
        ClosedCallback _closed_callback;
        AnyEventCallback _any_callback;
        /*组件内的连接关闭回调--组件内设置的，因为服务器组件内会把所有的连接管理起来，一旦某个连接要关闭*/
        /*就应该从管理的地方移除掉自己的信息*/
        ClosedCallback _server_closed_callback;
    private:
        void HandleRead(){
            // LOG(logLevel::DEBUG) << "Handle read...";
            char buff[65535];
            int n = _sock.Read(buff, sizeof(buff), false);
            if(n<0){
                LOG(logLevel::ERROR) << "Connection read error";
                ShutdownInLoop();                              //_out_buffer may have data to send
            }
            // if(_message_callback){
            //     LOG(logLevel::DEBUG) << "messagecallback is not null";
            // }
            _in_buffer.WriteAndPush(buff, n);
            if(_in_buffer.ReadableSize()>0){
                // LOG(logLevel::DEBUG) << "callback message";
                _message_callback(shared_from_this(), &_in_buffer);
            }
        }
        void HandleWrite(){
            int n = _sock.Send(_out_buffer.GetReadPos(), _out_buffer.ReadableSize(), false);
            if(n<0){
                if(_in_buffer.ReadableSize()>0){
                    _message_callback(shared_from_this(), &_in_buffer);
                }
                Release();
            }
            _out_buffer.MoveReadOffset(n);
            if(_out_buffer.ReadableSize()==0){
                _channel->DisableWrite();
                if(_status==DISCONNECTING){
                    Release();
                }
            }
        }
        void HandleClose(){
            if(_in_buffer.ReadableSize()>0){
                _message_callback(shared_from_this(), &_in_buffer);
            }
            Release();
        }
        void HandleError(){
            HandleClose();
        }
        void HandleAny(){
            if(_releaseOrNot){
                _loop->UpdateTimer(_id);                    //connection id also be used as timer id for convenience
            }
            if(_any_callback){
                _any_callback(shared_from_this());
            }
        }
        void EnableInactiveReleaseInLoop(int sec){
            _releaseOrNot = true;
            if(_loop->HasTimer(_id)){
                _loop->UpdateTimer(_id);
            }else{
                _loop->AddTimer(_id, std::bind(&Connection::Release, this),sec);
            }
        }
        void CancelInactiveReleaseInLoop(){
            _releaseOrNot = false;
            if(_loop->HasTimer(_id)){
                _loop->CancelTimer(_id);
            }
        }
        void EstablishedInLoop(){
            assert(_status == CONNECTING);
            _status = CONNECTED;
            _channel->CareRead();
            if(_connected_callback){
                _connected_callback(shared_from_this());
            }
        }
        void ShutdownInLoop(){
            _status = DISCONNECTING;
            if(_in_buffer.ReadableSize()>0){
                _message_callback(shared_from_this(), &_in_buffer);
            }
            if(_out_buffer.ReadableSize()==0){
                Release();
            }else{
                if(false==_channel->WriteAble()){
                    _channel->CareWrite();
                }
            }
        }
        void ReleaseInLoop(){
            // LOG(logLevel::INFO) << "release connection...";
            _status = DISCONNECTED;
            _channel->RemoveFromEpoll();
            _sock.Close();
            if(_releaseOrNot){
                CancelInactiveReleaseInLoop();
            }
            if(_closed_callback){
                _closed_callback(shared_from_this());
            }
            if(_server_closed_callback){
                _server_closed_callback(shared_from_this());
            }
        }
        void UpgradeInLoop(const ConnectedCallback& conn,const MessageCallback& mess,const ClosedCallback& clo, 
                           const AnyEventCallback& any,const Any& context){
            _connected_callback = conn;
            _closed_callback = clo;
            _any_callback = any;
            _context = context;
        }
        void SendInLoop(const Buffer& buff){
            if(_status==DISCONNECTED){
                return;
            }
            _out_buffer.WriteAndPush(buff);
            // LOG(logLevel::DEBUG) << "_out_buffer.size(): " << _out_buffer.ReadableSize();
            if (_channel->WriteAble() == false)
            {
                _channel->CareWrite();
            }
        }

    public:
        Connection(EventLoop* loop,uint64_t id,int sockfd):_id(id),_sock_fd(sockfd),_sock(_sock_fd),_status(CONNECTING),
        _loop(loop),_releaseOrNot(false),_channel(std::make_shared<Channel>(_sock_fd,_loop))
        {
            // std::cout << "_sock_fd is" << _sock_fd << std::endl;
            _channel->SetReadCallback(std::bind(&Connection::HandleRead, this));
            _channel->SetWriteCallback(std::bind(&Connection::HandleWrite, this));
            _channel->SetCloseCallback(std::bind(&Connection::HandleClose, this));
            _channel->SetErrorCallback(std::bind(&Connection::HandleError, this));
            _channel->SetAnyCallback(std::bind(&Connection::HandleAny, this));
        }
        ~Connection(){
            // LOG(logLevel::INFO) << _id << " Connection release.";
        }
        int Fd() { return _sock_fd; }
        int Id(){return _id;}
        bool IsConnected(){return _status==CONNECTED;}
        void SetContext(const Any &context) { _context = context; }
        Any *GetConext() { return &_context; }
        void SetConnectedCallback(const ConnectedCallback&cb) { _connected_callback = cb; }
        void SetMessageCallback(const MessageCallback&cb) { _message_callback = cb; }
        void SetClosedCallback(const ClosedCallback&cb) { _closed_callback = cb; }
        void SetAnyEventCallback(const AnyEventCallback&cb) { _any_callback = cb; }
        void SetSrvClosedCallback(const ClosedCallback&cb) { _server_closed_callback = cb; }
        void Established() {
            _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, this));
        }
        void Send(const char *data, size_t len) {   
            Buffer buf;
            buf.WriteAndPush(data, len); 
            _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, std::move(buf)));
        }
        void Shutdown(){
            _loop->RunInLoop(std::bind(&Connection::ShutdownInLoop, this));
        }
        void Release(){
            // LOG(logLevel::DEBUG) <<"准备释放连接...";
            _loop->PushTask(std::bind(&Connection::ReleaseInLoop, this));
        }
        void EnableInactiveRelease(int sec) {
            _loop->RunInLoop(std::bind(&Connection::EnableInactiveReleaseInLoop, this, sec));
        }
        void CancelInactiveRelease() {
            _loop->RunInLoop(std::bind(&Connection::CancelInactiveReleaseInLoop, this));
        }

        void SwitchProtocol(const Any &context, const ConnectedCallback &conn, const MessageCallback &msg, 
                     const ClosedCallback &closed, const AnyEventCallback &any) {
            _loop->RunInLoop(std::bind(&Connection::UpgradeInLoop, this, conn, msg, closed, any,context));
        }
    };

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


    class TcpServer
    {
    private:
        uint64_t _next_id;
        int _port;
        int _timeout;
        bool _releaseOrNot;
        EventLoop _base_loop; 
        Acceptor _acceptor;
        LoopThreadPool _pool;
        std::unordered_map<uint64_t, PtrConnection> _connections;
        using ConnectedCallback = std::function<void(const PtrConnection&)>;
        using MessageCallback = std::function<void(const PtrConnection&, Buffer *)>;// these are from external
        using ClosedCallback = std::function<void(const PtrConnection&)>;
        using AnyEventCallback = std::function<void(const PtrConnection&)>;
        using Functor = std::function<void()>;
        ConnectedCallback _connected_callback;
        MessageCallback _message_callback;
        ClosedCallback _closed_callback;
        AnyEventCallback _any_callback;
        ClosedCallback _server_closed_callback;

        //this function will be called when new link is coming in acceptor thread
        void NewConnection(int fd)
        {
            ++_next_id;
            PtrConnection conn = std::make_shared<Connection>(_pool.Next(), _next_id, fd);
            conn->SetClosedCallback(_closed_callback);
            conn->SetMessageCallback(_message_callback);
            conn->SetConnectedCallback(_connected_callback);
            conn->SetAnyEventCallback(_any_callback);
            conn->SetSrvClosedCallback(std::bind(&TcpServer::RemoveConnection, this,std::placeholders::_1));
            if(_releaseOrNot){
                conn->EnableInactiveRelease(_timeout);
            }
            conn->Established();               //main thread put tasks into slave thread to ensure thread safety
            _connections[_next_id] = conn;
            // LOG(logLevel::DEBUG) << "New connection is added.";
        }
        void RemoveConnectionInLoop(const PtrConnection& conn){
            _connections.erase(conn->Id());
            // LOG(logLevel::DEBUG) << "_connection.size():" << _connections.size();
        }
        void RemoveConnection(const PtrConnection& conn){
            _base_loop.RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
        }
        void RunAfterInLoop(const Functor& task,int delay){
            ++_next_id;
            _base_loop.AddTimer(_next_id, task, delay);
        }

    public:
        TcpServer(int port):_port(port),_next_id(0),_timeout(-1),_releaseOrNot(false),
        _base_loop(),_acceptor(_port,&_base_loop),_pool(&_base_loop){
            _acceptor.SetReadCallback(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));
        }
        void SetThreadsCount(int count) { _pool.SetCount(count); }
        void SetConnectedCallback(const ConnectedCallback&cb) { _connected_callback = cb; }
        void SetMessageCallback(const MessageCallback&cb) { _message_callback = cb; }
        void SetClosedCallback(const ClosedCallback&cb) { _closed_callback = cb; }
        void SetAnyEventCallback(const AnyEventCallback&cb) { _any_callback = cb; }
        void EnableInactiveRelease(int timeout) { _timeout = timeout; _releaseOrNot = true;}
        void RunAfter(const Functor &task, int delay) {
            _base_loop.RunInLoop(std::bind(&TcpServer::RunAfterInLoop, this, task, delay));
        }
        void Start(){
            _pool.Create();
            _base_loop.Start();
        }
    };

}
#endif