#include "../source/server.hpp"

using namespace byBit;
using Channel_t = std::shared_ptr<Channel>;
void HandleRead(Channel_t ch){
    LOG(logLevel::DEBUG) << "This is in HandlRead()";
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
    std::cout << out << std::endl;
    ch->CareWrite();
}
void HandleWrite(Channel_t ch)
{
    LOG(logLevel::DEBUG) << "This is in HandleWrite()";
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
void HandleExcept(Channel_t ch){
    ch->Close();
}
void testAccept(Channel_t ch,EventLoop* poller)
{
    LOG(logLevel::ERROR) << "This is testSAccept...";

    int lfd = ch->Fd();
    int newfd = accept(lfd, nullptr, nullptr);
    fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0) | O_NONBLOCK);

    if(newfd<0){
        LOG(logLevel::ERROR) << "Accept error...";
        return;
    }
    //common communication event 
    Channel_t commu = std::make_shared<Channel>(newfd, poller);
    commu->SetReadCallback(std::bind(HandleRead,commu));
    commu->SetWriteCallback(std::bind(HandleWrite,commu));
    commu->SetErrorCallback(std::bind(HandleExcept,commu));
    commu->SetCloseCallback(std::bind(HandleExcept,commu));
    commu->CareRead();
    poller->AddToEpoll(commu);
    return;
}

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
void test2(std::string ip,uint16_t port)
{
    EventLoop loop;
    Socket listener;
    listener.BuildServer(ip, port, false, true);
    Channel_t ch = std::make_shared<Channel>(listener.Fd(), &loop);
    ch->SetReadCallback(std::bind(testAccept, ch, &loop));
    ch->CareRead();
    loop.AddToEpoll(ch);
    loop.Start();
}
int main(int argc,char* argv[])
{
    if(argc != 2){
        LOG(logLevel::ERROR) << "Format error";
        exit(1);
    }
    std::string ip = "127.0.0.1";
    uint16_t port = std::stoi(argv[1]);
    test2(ip,port);

    return 0;
}
