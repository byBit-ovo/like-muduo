#include "../source/server.hpp"

using namespace byBit;

void task_1(){
    std::cout << "task_1" << std::endl;
}
void task_2(){
    std::cout << "task_2" << std::endl;
}
void task_3(){
    std::cout << "task_3" << std::endl;
}
void task_4(){
    std::cout << "task_4" << std::endl;
}

int Load()
{
    struct itimerspec t;
    t.it_value.tv_nsec = 0;
    t.it_value.tv_sec = 1;
    t.it_interval.tv_nsec = 0;
    t.it_interval.tv_sec = 1;
    int trfd = timerfd_create(CLOCK_MONOTONIC, 0);
    int n = timerfd_settime(trfd, 0, &t, nullptr);
    return trfd;
}
// class Test
// {
// public:
//     Test(const Test& other)
//     {
//         std::cout << "const Test& other" << std::endl;
//     }
//     Test()
//     {
//         std::cout << "Test()" << std::endl;
//     }
//     ~Test()
//     {
//         std::cout << "~Test()" << std::endl;
//     }
// };
// Test test()
// {
//     Test t;
//     return t;
// }
int main()
{
    int fd = Load();
    while(1){
        uint64_t val = 0;
        int n = read(fd, &val, sizeof(uint64_t));
        std::cout << val << std::endl;
    }
    // time_t now = time(0);
    // struct tm *time = localtime(&now);
    // char buffer[50];
    // strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S\n", time);
    // fprintf(stdout, "%s", buffer);
    // test();
    // while(1)
    //     ;

    // std::cout << typeid(stdout).name() << std::endl;
    // std::cout << stdin->_fileno << std::endl;
    // std::cout << stdout->_fileno << std::endl;
    // std::cout << stderr->_fileno << std::endl;
    // fprintf(stdout, "%s""unleash %d\n", "byBit", 888);
    // std::unique_ptr<TimeWheel> wheel = std::make_unique<TimeWheel>(0,60);
    // wheel->Add(1, task_1, 5);//5s
    // wheel->Add(2, task_2, 6);//6s
    // wheel->Add(3, task_3, 10);//20s
    // int trfd = Load();
    // for (int i = 0; i < 8;++i)
    // {
    //     sleep(1);
    //     wheel->UpdateTask(1);
    //     std::cout << "A new 5s." << std::endl;
    //     wheel->Increment();
    // }
    // wheel->Cancel(1);
    // for (int i = 0; i < 20; ++i)
    // {
    //     sleep(1);
    //     wheel->Increment();
    //     std::cout << "tick move one step..." << std::endl;
    // }

    return 0;
}
