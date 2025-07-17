#include "../source/server.hpp"
using namespace byBit;
std::unordered_map<int,PtrConnection> connections;
int conn_id = 1;
using Channel_t = std::shared_ptr<Channel>;
// void test(std::string ip,uint16_t port)
// {

//     Epoller poller;//3
//     //create listen socket
//     Socket listener;
//     listener.BuildServer(ip, port, false, true);//4
//     Channel_t lisChannel = std::make_shared<Channel>(listener.Fd(), &poller);
    
//     lisChannel->SetReadCallback(std::bind(testAccept,lisChannel,&poller));
//     lisChannel->CareRead();
//     poller.Add(lisChannel);
//     //Start to epoll
//     while (true)
//     {
//         std::vector<Channel_t> active;
//         poller.Monitor(&active);
//         LOG(logLevel::INFO) << "Channels are active!";
//         for(auto& a:active){
//             a->HandleEvents();
//         }
//     }
// }
void HandleRead(Channel_t ch){
    // LOG(logLevel::DEBUG) << "This is in HandlRead()";
    int flag = MSG_DONTWAIT;
    int fd = ch->Fd();
    char out[1024];
    int n = recv(fd, out, 1023, flag);
    if(n==0){
        LOG(logLevel::DEBUG) << "Client close session";

    }
    else if (n < 0)
    {
        if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR){
            return;
        }
    }
    out[n] = 0;
    LOG(logLevel::INFO) << out;
    ch->CareWrite();
}
void HandleWrite(Channel_t ch)
{
    // LOG(logLevel::DEBUG) << "This is in HandleWrite()";
    std::string s = "Hello,byBit!";
    int flag = MSG_DONTWAIT;
    int fd = ch->Fd();
    int n = send(fd, s.c_str(), s.size(), flag);
    if(n<0){
        if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR){
            return;
        }
    }
    ch->DisableWrite();

}
void HandleClose(PtrConnection conn){
    connections.erase(conn->Id());
    LOG(logLevel::INFO) << conn->Id() << " is closed!";
}
void AnyCall(EventLoop* loop,Channel_t ch)
{
    loop->UpdateTimer(1);
}

void ConnectionMessage(PtrConnection conn,Buffer* in_buffer)
{
    char buff[1024];
    int n = in_buffer->ReadAndPop(buff,sizeof(buff));
    buff[n] = 0;
    LOG(logLevel::INFO) << buff;
    std::string mess = buff;
    mess += " too! ";
    conn->Send(mess.c_str(), mess.size());
}
void ConnectionEstablish(PtrConnection conn){
    LOG(logLevel::INFO) << conn->Id() << " is established !";
}
void testAccept(Channel_t ch,EventLoop* poller)
{
    // LOG(logLevel::ERROR) << "This is testSAccept...";

    int lfd = ch->Fd();
    int newfd = accept(lfd, nullptr, nullptr);
    fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0) | O_NONBLOCK);

    if(newfd<0){
        LOG(logLevel::ERROR) << "Accept error...";
        return;
    }
    //common communication event 
    // Channel_t commu = std::make_shared<Channel>(newfd, poller);
    PtrConnection conn = std::make_shared<Connection>(poller, conn_id, newfd);
    conn->SetClosedCallback(std::bind(HandleClose,std::placeholders::_1));
    conn->SetMessageCallback(std::bind(ConnectionMessage, std::placeholders::_1, std::placeholders::_2));
    conn->SetConnectedCallback(std::bind(ConnectionEstablish, std::placeholders::_1));
    conn->EnableInactiveRelease(10);
    conn->Established();
    connections[conn->Id()] = conn;
    ++conn_id;

    return;
}
std::vector<LoopThread> threads(5);
int threadId = 0;
LoopThreadPool *pool;
void AcceptCallback(EventLoop *poller, int newfd)
{
    // threadId = (threadId + 1) % 5;
    PtrConnection conn = std::make_shared<Connection>(pool->Next(), conn_id, newfd);
    conn->SetClosedCallback(std::bind(HandleClose,std::placeholders::_1));
    conn->SetMessageCallback(std::bind(ConnectionMessage, std::placeholders::_1, std::placeholders::_2));
    conn->SetConnectedCallback(std::bind(ConnectionEstablish, std::placeholders::_1));
    conn->EnableInactiveRelease(20);
    conn->Established();
    connections[conn->Id()] = conn;
    ++conn_id;
}
void test2(uint16_t port)
{
    EventLoop loop;
    pool = new LoopThreadPool(&loop);
    pool->SetCount(3);
    pool->Create();
    Acceptor listener(port, &loop);
    listener.SetReadCallback(std::bind(AcceptCallback, &loop, std::placeholders::_1));
    LOG(logLevel::INFO) << "main thread id: " << pthread_self();
    loop.Start();
    delete pool;
}
void test(){
    static int a = 1;
    ++a;
}
int main(int argc,char* argv[])
{
    // if(argc != 2){
    //     LOG(logLevel::ERROR) << "Format error";
    //     exit(1);
    // }
    // uint16_t port = std::stoi(argv[1]);
    // // test2(port);
    // TcpServer server(port);
    // server.SetThreadsCount(3);
    // server.EnableInactiveRelease(20);
    // server.Start();

    return 0;
}
