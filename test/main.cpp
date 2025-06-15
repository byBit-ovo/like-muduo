#include "../source/http.hpp"
using namespace byBit;
#define WWWROOT "./wwwroot/"
std::string RequestStr(const HttpRequest &req) {
    std::stringstream ss;
    ss << req._method << " " << req._url << " " << req._version << "\r\n";
    for (auto &it : req._parameters) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    for (auto &it : req._headers) {
        ss << it.first << ": " << it.second << "\r\n";
    }
    ss << "\r\n";
    ss << req._body;
    return ss.str();
}
void Hello(const HttpRequest &req, HttpResponse *rsp) 
{
    rsp->SetContent(RequestStr(req), "text/plain");
    // sleep(15);
}
void Login(const HttpRequest &req, HttpResponse *rsp) 
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
void PutFile(const HttpRequest &req, HttpResponse *rsp) 
{
    LOG(logLevel::INFO) << "PUT ...";
    std::string pathname = WWWROOT + req._url;
    Util::WriteFile(pathname, req._body); 
}
void DelFile(const HttpRequest &req, HttpResponse *rsp) 
{
    rsp->SetContent(RequestStr(req), "text/plain");
}
int main()
{
    HttpServer server(8888);
    server.SetThreadCount(3);
    server.SetAssetDir(WWWROOT);
    server.AddDeleteMethod("/quit",DelFile);
    server.AddPutMethod("/big.txt",PutFile);
    server.AddGetMethod("/hello",Hello);
    server.AddPostMethod("/nice",Hello);
    server.AddGetMethod("/hello",Hello);
    server.Start();
    return 0;
}

// GET /hello HTTP/1.1