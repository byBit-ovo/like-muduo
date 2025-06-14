#include "../source/server.hpp"

namespace byBit
{
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
            LOG(logLevel::DEBUG) << "New connection is added.";
        }
        void RemoveConnectionInLoop(const PtrConnection& conn){
            _connections.erase(conn->Id());
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