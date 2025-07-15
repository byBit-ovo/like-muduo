#include "../source/server.hpp"

int main(int argc,char* argv[])
{
    if(argc!=2){
        LOG(logLevel::ERROR) << "Format error..";
        exit(1);
    }
    
    byBit::Socket sk;
    sk.BuildServer(std::stoi(argv[1]));
    byBit::Epoller poller;
    std::shared_ptr<byBit::Channel> listener = std::make_shared<byBit::Channel>(sk.Fd());
    std::function<void()> rcd = std::bind(&byBit::Socket::Accept, &sk);
    listener->SetReadCallback(rcd);
    poller.Add(listener);
   
    return 0;
}
