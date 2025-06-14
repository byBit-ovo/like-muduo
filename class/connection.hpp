#include <memory>
#include "EventLoop.hpp"
#include "Buffer.hpp"

namespace byBit
{
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
        Channel_t _channel;    //event manager
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
            char buff[65535];
            int n = _sock.Read(buff, sizeof(buff), false);
            if(n<=0){
                LOG(logLevel::ERROR) << "Connection read error";
                ShutdownInLoop();                              //_out_buffer may have data to send
            }
            _in_buffer.WriteAndPush(buff, n);
            if(_in_buffer.ReadableSize()>0){
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
            if(_channel->WriteAble()==false){
                _channel->CareWrite();
            }
        }

    public:
        Connection(EventLoop* loop,uint64_t id,int sockfd):_id(id),_sock_fd(sockfd),_sock(_sock_fd),_status(CONNECTING),
        _loop(loop),_releaseOrNot(false),_channel(std::make_shared<Channel>(_sock_fd,_loop))
        {
            _channel->SetReadCallback(std::bind(&Connection::HandleRead, this));
            _channel->SetWriteCallback(std::bind(&Connection::HandleWrite, this));
            _channel->SetCloseCallback(std::bind(&Connection::HandleClose, this));
            _channel->SetErrorCallback(std::bind(&Connection::HandleError, this));
            _channel->SetAnyCallback(std::bind(&Connection::HandleAny, this));
        }
        ~Connection(){
            LOG(logLevel::INFO) << _id << " Connection release.";
            LOG(logLevel::INFO) << "thread id: " << pthread_self();
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
        void Send(const char *data, size_t len) {   //data maybe local,we need to save it first
            Buffer buf;
            buf.WriteAndPush(data, len);  
            _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, buf));
        }
        void Shutdown(){
            _loop->RunInLoop(std::bind(&Connection::ShutdownInLoop, this));
        }
        void Release(){
            _loop->RunInLoop(std::bind(&Connection::ReleaseInLoop, this));
        }
        void EnableInactiveRelease(int sec) {
            _loop->RunInLoop(std::bind(&Connection::EnableInactiveReleaseInLoop, this, sec));
        }
        void CancelInactiveRelease() {
            _loop->RunInLoop(std::bind(&Connection::CancelInactiveReleaseInLoop, this));
        }

        //make sure switch protocol in eventloop,or we may handle events through expired protocol 
        void SwitchProtocol(const Any &context, const ConnectedCallback &conn, const MessageCallback &msg, 
                     const ClosedCallback &closed, const AnyEventCallback &any) {
            _loop->RunInLoop(std::bind(&Connection::UpgradeInLoop, this, conn, msg, closed, any,context));
        }
    };
}