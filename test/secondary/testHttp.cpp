#include "../source/http.hpp"
using namespace byBit;

void testSplit()
{
    std::vector<std::string> v;
    std::string src = "hello,byBit,Nice to meet,you!";
    std::string delimeter = ",";
    Util::Split(src, delimeter, &v);
    for(auto& l:v){
        std::cout << l << std::endl;
    }
}
void testRead(){
    std::string s;
    Util::ReadFile("test.cpp", &s);
    std::cout << s << std::endl;
}
void testWrite()
{
    std::string s = "hello\n,my name is jack";
    Util::WriteFile("book.txt",s);
}
void testEncode(){
    std::string url = "/index.html/?name=bybit&password=123";
    std::string pass = Util::UrlEncode(url, false);
    std::cout << pass<< std::endl;
    std::cout << Util::UrlDecode(pass, false) << std::endl;
}
void testStaAndMime(){
    std::string file = "index.456";
    // std::cout << Util::ExetMime(file) << std::endl;
    std::cout << Util::StatuDesc(444) << std::endl;
}
void testValid(){
    std::string path = "index/../../aaa/";
    std::cout << Util::ValidPath(path) << std::endl;
}
int main()
{
    // testRead();
    // testWrite();
    // testEncode();
    // testStaAndMime();
    testValid();
    return 0;
}