#include "../source/server.hpp"
using namespace byBit;

int main()
{
    Buffer b;
    std::string s1 = "Hello byBit!";
    for (int i = 0; i < 1000;++i)
    {
        b.WriteAndPush(s1 + std::to_string(i)+'\n');
    }
    char s2[20*1500];
    int n = b.ReadAndPop(s2,20*1500);
    s2[n] = 0;
    std::cout << s2 << std::endl;
    std::string s3(1000,' ');
    b.Read(&s3);
    std::cout << s3 << std::endl;

    return 0;
}
