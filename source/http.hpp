#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "log.hpp"
#include <unordered_map>
#include <sys/stat.h>
#include <regex>
#include "server.hpp"
namespace byBit
{
    std::unordered_map<int, std::string> _statu_msg = {
        {100, "Continue"},
        {101, "Switching Protocol"},
        {102, "Processing"},
        {103, "Early Hints"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {207, "Multi-Status"},
        {208, "Already Reported"},
        {226, "IM Used"},
        {300, "Multiple Choice"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {306, "unused"},
        {307, "Temporary Redirect"},
        {308, "Permanent Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Payload Too Large"},
        {414, "URI Too Long"},
        {415, "Unsupported Media Type"},
        {416, "Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {418, "I'm a teapot"},
        {421, "Misdirected Request"},
        {422, "Unprocessable Entity"},
        {423, "Locked"},
        {424, "Failed Dependency"},
        {425, "Too Early"},
        {426, "Upgrade Required"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {431, "Request Header Fields Too Large"},
        {451, "Unavailable For Legal Reasons"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {505, "HTTP Version Not Supported"},
        {506, "Variant Also Negotiates"},
        {507, "Insufficient Storage"},
        {508, "Loop Detected"},
        {510, "Not Extended"},
        {511, "Network Authentication Required"}};

    std::unordered_map<std::string, std::string> _mime_msg = {
        {".aac", "audio/aac"},
        {".abw", "application/x-abiword"},
        {".arc", "application/x-freearc"},
        {".avi", "video/x-msvideo"},
        {".azw", "application/vnd.amazon.ebook"},
        {".bin", "application/octet-stream"},
        {".bmp", "image/bmp"},
        {".bz", "application/x-bzip"},
        {".bz2", "application/x-bzip2"},
        {".csh", "application/x-csh"},
        {".css", "text/css"},
        {".csv", "text/csv"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".eot", "application/vnd.ms-fontobject"},
        {".epub", "application/epub+zip"},
        {".gif", "image/gif"},
        {".htm", "text/html"},
        {".html", "text/html"},
        {".ico", "image/vnd.microsoft.icon"},
        {".ics", "text/calendar"},
        {".jar", "application/java-archive"},
        {".jpeg", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".jsonld", "application/ld+json"},
        {".mid", "audio/midi"},
        {".midi", "audio/x-midi"},
        {".mjs", "text/javascript"},
        {".mp3", "audio/mpeg"},
        {".mpeg", "video/mpeg"},
        {".mpkg", "application/vnd.apple.installer+xml"},
        {".odp", "application/vnd.oasis.opendocument.presentation"},
        {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
        {".odt", "application/vnd.oasis.opendocument.text"},
        {".oga", "audio/ogg"},
        {".ogv", "video/ogg"},
        {".ogx", "application/ogg"},
        {".otf", "font/otf"},
        {".png", "image/png"},
        {".pdf", "application/pdf"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        {".rar", "application/x-rar-compressed"},
        {".rtf", "application/rtf"},
        {".sh", "application/x-sh"},
        {".svg", "image/svg+xml"},
        {".swf", "application/x-shockwave-flash"},
        {".tar", "application/x-tar"},
        {".tif", "image/tiff"},
        {".tiff", "image/tiff"},
        {".ttf", "font/ttf"},
        {".txt", "text/plain"},
        {".vsd", "application/vnd.visio"},
        {".wav", "audio/wav"},
        {".weba", "audio/webm"},
        {".webm", "video/webm"},
        {".webp", "image/webp"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".xhtml", "application/xhtml+xml"},
        {".xls", "application/vnd.ms-excel"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".xml", "application/xml"},
        {".xul", "application/vnd.mozilla.xul+xml"},
        {".zip", "application/zip"},
        {".3gp", "video/3gpp"},
        {".3g2", "video/3gpp2"},
        {".7z", "application/x-7z-compressed"}};

    class Util
    {
    public:
        static void Split(const std::string &src, const std::string &deli, std::vector<std::string> *v)
        {
            int offset = 0;
            while (offset < src.size())
            {
                int pos = src.find(deli, offset);
                if (pos == std::string::npos)
                {
                    v->emplace_back(src.substr(offset));
                    return;
                } // hel,ooo
                if (pos - offset != 0)
                {
                    v->emplace_back(src.substr(offset, pos - offset));
                }
                offset = pos + deli.size();
            }
        }
        static bool ReadFile(const std::string fileName, std::string *s)
        {
            std::ifstream fin(fileName, std::ios::binary);
            if (!fin.is_open())
            {
                LOG(logLevel::ERROR) << "File open error";
                return false;
            }
            size_t fsize = 0;
            fin.seekg(0, std::ios::end);
            fsize = fin.tellg();
            fin.seekg(0, std::ios::beg);
            s->resize(fsize);
            char *pos = &(*s)[0];
            fin.read(pos, fsize);
            if (fin.good() == false)
            {
                LOG(logLevel::ERROR) << "reading from" << fileName << "error";
                return false;
            }
            fin.close();
            return true;
        }
        static void WriteFile(const std::string fileName, const std::string s)
        {
            std::ofstream fout(fileName, std::ios::binary | std::ios::trunc);
            if (!fout.is_open())
            {
                LOG(logLevel::ERROR) << "File open error";
                return;
            }
            fout.write(s.c_str(), s.size());
            if (fout.good() == false)
            {
                LOG(logLevel::ERROR) << "reading from" << fileName << "error";
            }
            fout.close();
        }
        // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产生歧义
        // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀%   C++ -> C%2B%2B
        //   不编码的特殊字符： RFC3986文档规定 . - _ ~ 字母，数字属于绝对不编码字符
        // RFC3986文档规定，编码格式 %HH
        // W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
        static std::string UrlEncode(const std::string url, bool convert_space_to_plus)
        {
            std::string res;
            for (int i = 0; i < url.size(); ++i)
            {
                if (url[i] == '.' || url[i] == '-' || url[i] == '_' || url[i] == '~' || isalnum(url[i]))
                {
                    res += url[i];
                    continue;
                }
                if (convert_space_to_plus && url[i] == ' ')
                {
                    res += '+';
                }
                else
                {
                    char tmp[4] = {0};
                    snprintf(tmp, 4, "%%%02x", url[i]);
                    res += tmp;
                }
            }
            return res;
        }
        static int HexToI(char c)
        {
            if (isupper(c))
            {
                return c - 'A' + 10;
            }
            else if (islower(c))
            {
                return c - 'a' + 10;
            }
            else if (isdigit(c))
            {
                return c - '0';
            }
            return -1;
        }
        static std::string UrlDecode(const std::string url, bool convert_plus_to_space)
        {
            std::string res;
            for (int i = 0; i < url.size(); ++i)
            {
                if (url[i] == '%' && i + 2 < url.size())
                {
                    int h = HexToI(url[i + 1]) << 4;
                    int l = HexToI(url[i + 2]);
                    res += (h + l);
                    i += 2;
                }
                else if (convert_plus_to_space && url[i] == '+')
                {
                    res += ' ';
                }
                else
                {
                    res += url[i];
                }
            }

            return res;
        }
        static std::string StatuDesc(int code)
        {
            auto iter = _statu_msg.find(code);
            if (iter == _statu_msg.end())
            {
                return "Unknow";
            }
            return _statu_msg[code];
        }
        static std::string ExetMime(const std::string &name)
        {
            int pos = name.rfind('.');
            if (pos == std::string::npos)
            {
                return "application/octet-stream";
            }
            std::string mime = name.substr(pos);
            auto iter = _mime_msg.find(mime);
            if (iter == _mime_msg.end())
            {
                return "application/octet-stream";
            }
            return _mime_msg[mime];
        }
        //它同时检查目录是否存在、是否可访问、且是否是一个目录。
        static bool IsDirectory(const std::string &filename)
        {
            struct stat st;
            int ret = stat(filename.c_str(), &st);
            if (ret < 0)
            {
                return false;
            }
            return S_ISDIR(st.st_mode);
        }
        //它同时检查文件是否存在、是否可访问、且是否是一个普通文件。
        static bool IsRegular(const std::string &filename)
        {
            struct stat st;
            int ret = stat(filename.c_str(), &st);
            if (ret < 0)
            {
                return false;
            }
            return S_ISREG(st.st_mode);
        }

        static bool ValidPath(const std::string path)
        {
            std::vector<std::string> v;
            Split(path, "/", &v);
            int level = 0;
            for (int i = 0; i < v.size(); ++i)
            {
                if (v[i] == "..")
                {
                    --level;
                    if (level < 0)
                    {
                        return false;
                    }
                }
                else
                {
                    ++level;
                }
            }
            return true;
        }
    };

    class HttpRequest
    {
    public:
        std::string _method;
        std::string _url;
        std::string _version;
        std::string _body;
        std::smatch _matches;
        std::unordered_map<std::string, std::string> _headers;
        std::unordered_map<std::string, std::string> _parameters;
        HttpRequest() : _version("HTTP/1.1") {}
        void ReSet()
        {
            _method.clear();
            _url.clear();
            _version = "HTTP/1.1";
            _body.clear();
            std::smatch match;
            _matches.swap(match);
            _headers.clear();
            _parameters.clear();
        }
        void SetHeader(const std::string &key, const std::string &val)
        {
            _headers.insert(std::make_pair(key, val));
        }
        bool HasHeader(const std::string &key) const
        {
            auto it = _headers.find(key);
            if (it == _headers.end())
            {
                return false;
            }
            return true;
        }
        std::string GetHeader(const std::string &key) const
        {
            auto it = _headers.find(key);
            if (it == _headers.end())
            {
                return "";
            }
            return it->second;
        }
        void SetParam(const std::string &key, const std::string &val)
        {
            _parameters.insert(std::make_pair(key, val));
        }
        bool HasParam(const std::string &key) const
        {
            auto it = _parameters.find(key);
            if (it == _parameters.end())
            {
                return false;
            }
            return true;
        }
        std::string GetParam(const std::string &key) const
        {
            auto it = _parameters.find(key);
            if (it == _parameters.end())
            {
                return "";
            }
            return it->second;
        }
        size_t ContentLength() const
        {
            // Content-Length: 1234\r\n
            bool ret = HasHeader("Content-Length");
            if (ret == false)
            {
                return 0;
            }
            std::string clen = GetHeader("Content-Length");
            return std::stol(clen);
        }
        bool Close() const
        {
            if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive")
            {
                return false;
            }
            return true;
        }
    };
    
    class HttpResponse
    {
        public:
            int _status;
            bool _redirect;
            std::string _redirect_url;
            std::string _body;
            std::unordered_map<std::string, std::string> _headers;

        HttpResponse():_redirect(false), _status(200) {}
        HttpResponse(int statu):_redirect(false), _status(statu) {} 
        void ReSet() {
            _status = 200;
            _redirect = false;
            _body.clear();
            _redirect_url.clear();
            _headers.clear();
        }
        void SetHeader(const std::string &key, const std::string &val) {
            _headers.insert(std::make_pair(key, val));
        }
        bool HasHeader(const std::string &key) {
            auto it = _headers.find(key);
            if (it == _headers.end()) {
                return false;
            }
            return true;
        }
        std::string GetHeader(const std::string &key) {
            auto it = _headers.find(key);
            if (it == _headers.end()) {
                return "";
            }
            return it->second;
        }
        void SetContent(const std::string &body,  const std::string &type = "text/html") {
            _body = body;
            SetHeader("Content-Type", type);
        }
        void SetRedirect(const std::string &url, int statu = 302) {
            _status = statu;
            _redirect = true;
            _redirect_url = url;
        }
        bool Close() {
            if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive") {
                return false;
            }
            return true;
        }
    };

    typedef enum
    {
        RECV_HTTP_LINE,
        RECV_HTTP_HEADER,
        RECV_HTTP_BODY,
        RECV_HTTP_ERROR,
        RECV_HTTP_OVER
    } HttpStatus;
#define MAX_LENGTH 8192
    class HttpContext
    {
        private:
            int _resp_statu;
            HttpStatus _recv_statu;
            HttpRequest _req;
        public:
            HttpContext():_resp_statu(200),_recv_statu(RECV_HTTP_LINE){}
            HttpStatus RecvStatus() { return _recv_statu; }
            int RespStatus() { return _resp_statu; }
            HttpRequest& GetReq() { return _req; }
            void Reset(){
                _resp_statu = 200;
                _recv_statu = RECV_HTTP_LINE;
                _req.ReSet();
            }
            void RecvHttpReq(Buffer* buf){
                switch(_recv_statu){
                    case RECV_HTTP_LINE:
                        RecvHttpLine(buf);
                    case RECV_HTTP_HEADER:
                        RecvHttpHead(buf);
                    case RECV_HTTP_BODY:
                        RecvBody(buf);
                    }
            }
        private:
            bool RecvHttpLine(Buffer* buf)
            {
                if (_recv_statu != RECV_HTTP_LINE) return false;
                std::string line = buf->GetLineAndPop(); 
                if(line.empty()){
                    if(buf->ReadableSize() > MAX_LENGTH){
                        _recv_statu = RECV_HTTP_ERROR;
                        _resp_statu = 414;
                        return false;
                    }
                    _resp_statu = RECV_HTTP_LINE;
                    return true;
                }
                if(line.size() > MAX_LENGTH){
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 414;
                    return false;
                }
                bool n = ParseHttpLine(line);
                if(n==false){
                    return false;
                }
                _recv_statu = RECV_HTTP_HEADER;
                return true;
            }
            bool ParseHttpLine(const std::string& line){
                std::smatch matches;
                //ignore case to match 
                std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
                bool ret = std::regex_match(line, matches, e);
                if (ret == false) {
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 400;//BAD REQUEST
                    return false;
                }
                //0 : GET /bitejiuyeke/login?user=xiaoming&pass=123123 HTTP/1.1
                //1 : GET
                //2 : /bitejiuyeke/login
                //3 : user=xiaoming&pass=123123
                //4 : HTTP/1.1 
                _req._method = matches[1];
                std::transform(_req._method.begin(), _req._method.end(), _req._method.begin(), ::toupper);
                _req._url = matches[2];
                _req._version = matches[4];
                std::vector<std::string> query_string_arry;
                std::string query_string = matches[3];
                Util::Split(query_string, "&", &query_string_arry);
                for (auto &str : query_string_arry) {
                    size_t pos = str.find("=");
                    if (pos == std::string::npos) {
                        _recv_statu = RECV_HTTP_ERROR;
                        _resp_statu = 400;//BAD REQUEST
                        return false;
                    }
                    std::string key = Util::UrlDecode(str.substr(0, pos), true);  
                    std::string val = Util::UrlDecode(str.substr(pos + 1), true);
                    _req.SetParam(key, val);
                }
                return true;
            }
            bool RecvHttpHead(Buffer* buf)
            {
                if (_recv_statu != RECV_HTTP_HEADER) return false;
                //一行一行取出数据，直到遇到空行为止， 头部的格式 key: val\r\nkey: val\r\n....
                while(1){
                    std::string line = buf->GetLineAndPop();
                    if (line.size() == 0) {
                        if (buf->ReadableSize() > MAX_LENGTH) {
                            _recv_statu = RECV_HTTP_ERROR;
                            _resp_statu = 414;//URI TOO LONG
                            return false;
                        }
                        return true;
                    }
                    if (line.size() > MAX_LENGTH) {
                        _recv_statu = RECV_HTTP_ERROR;
                        _resp_statu = 414;//URI TOO LONG
                        return false;
                    }
                    if (line == "\n" || line == "\r\n") {
                        break;
                    }
                    bool ret = ParseHttpHead(line);
                    if (ret == false) {
                        return false;
                    }
                }
                _recv_statu = RECV_HTTP_BODY;
                return true;
            }
            bool ParseHttpHead(std::string& line)
            {
                //key: val\r\nkey: val\r\n
                if(line.back()=='\n') line.pop_back();
                if(line.back()=='\r') line.pop_back();
                int pos = line.find(": ");
                if(pos==std::string::npos){
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 400;
                    return false;
                }
                std::string key = line.substr(0, pos);
                std::string val = line.substr(pos + 2);
                _req.SetHeader(key, val);
                _recv_statu = RECV_HTTP_BODY;
                return true;
            }
            bool RecvBody(Buffer* buf){
                if(_recv_statu!=RECV_HTTP_BODY){
                    return false;
                }
                size_t len = _req.ContentLength();
                if(len==0){
                    _recv_statu = RECV_HTTP_OVER;
                    return true;
                }
                size_t reallen = len - _req._body.size();
                if(buf->ReadableSize() >= reallen){
                    _req._body.append(buf->GetReadPos(), reallen);
                    buf->MoveReadOffset(reallen);
                    _recv_statu = RECV_HTTP_OVER;
                    return true;
                }
                _req._body.append(buf->GetReadPos(), buf->ReadableSize());
                buf->MoveReadOffset(reallen);
                return true;
            }
    };

    class HttpServer
    {
        private:
            using Handle_t = std::function<void(const HttpRequest& req,HttpResponse* resp)>;
            using Router_t = std::vector<std::pair<std::regex,Handle_t>>;
            int _port;
            Router_t _get_router;
            Router_t _post_router;
            Router_t _put_router;
            Router_t _delete_router;
            std::string _asset_dir;
            TcpServer _server;
        
        private:
            void WriteResponse(const PtrConnection& conn,const HttpRequest& req,HttpResponse& res)
            {
                std::stringstream s;
                s << req._version << " " << std::to_string(res._status) << " " << Util::StatuDesc(res._status) << "\r\n";
                if (req.Close() == true)
                {
                    res.SetHeader("Connection", "close");
                }
                else
                {
                    res.SetHeader("Connection", "keep-alive");
                }
                if (!res._body.empty()&& !res.HasHeader("Content-Length")) {
                    res.SetHeader("Content-Length", std::to_string(res._body.size()));
                }
                if (!res._body.empty()&& !res.HasHeader("Content-Type")) {
                    res.SetHeader("Content-Type", "application/octet-stream");
                }
                if (res._redirect== true) {
                    res.SetHeader("Location", res._redirect_url);
                }
                for(auto &kv:res._headers)
                {
                    s << kv.first << ": " << kv.second << "\r\n";
                }
                s << "\r\n" << res._body;
                conn->Send(s.str().c_str(), s.str().size());
            }
            bool IsStatic(HttpRequest& req)
            {
                //1. _asset_dir must be existed
                //2. Method is either GET or HEAD
                //3. _url must be valid
                // if(_asset_dir.empty() || !Util::ValidPath(req._url)||
                //    req._method!="GET" || req._method!="HEAD") {
                //     return false;
                // }
                if(_asset_dir.empty()|| !Util::ValidPath(req._url)){
                    return false;
                }
                if(req._method != "GET" && req._method!="HEAD"){
                    return false;
                }
                //check if it's regular file
                if (req._url.back() == '/'){
                    req._url.append("index.html");
                }
                std::string url = _asset_dir + req._url;
                if(!Util::IsRegular(url)){//if not,return
                    return false;
                }//else ,append suffix to req._url
                return true;
            }
            void StaticHandler(const HttpRequest& req,HttpResponse* res)
            {
                LOG(logLevel::INFO) << "reading static file...";
                std::string url = _asset_dir + req._url;
                std::cout << url << std::endl;
                bool n = Util::ReadFile(url, &res->_body);
                if(n==false){
                    return;
                }
                std::string mime = Util::ExetMime(url);
                res->SetHeader("Content-Type", mime);
            }
            void Dispatcher(HttpRequest& req,HttpResponse *res,Router_t funcs)
            {
                for(auto &t:funcs)
                {
                    const std::regex e = t.first;
                    Handle_t handler = t.second;
                    if (std::regex_match(req._url, req._matches, e))
                    {
                        return t.second(req, res);
                    }
                }
                res->_status = 404;
            }

            void Router(HttpRequest& req,HttpResponse* res)
            {
                if (IsStatic(req)) {
                //是一个静态资源请求, 则进行静态资源请求的处理
                    return StaticHandler(req, res);
                }
                if (req._method == "GET" || req._method == "HEAD") {
                    return Dispatcher(req, res, _get_router);
                }else if (req._method == "POST") {
                    return Dispatcher(req, res, _post_router);
                }else if (req._method == "PUT") {
                    return Dispatcher(req, res, _put_router);
                }else if (req._method == "DELETE") {
                    return Dispatcher(req, res, _delete_router);
                }
                res->_status = 405;// Method Not Allowed
            }
            void ErrorHandler(const HttpRequest &req, HttpResponse *res)
            {
                std::string body;
                body += "<html>";
                body += "<head>";
                body += "<meta http-equiv='Content-Type' content='text/html;charset=utf-8'>";
                body += "</head>";
                body += "<body>";
                body += "<h1>";
                body += std::to_string(res->_status);
                body += " ";
                body += Util::StatuDesc(res->_status);
                body += "</h1>";
                body += "</body>";
                body += "</html>";
                //2. 将页面数据，当作响应正文，放入rsp中
                res->SetContent(body, "text/html");
            }
            void OnConnected(const PtrConnection &conn)
            {
                conn->SetContext(HttpContext());
                // LOG(logLevel::INFO) << "Connection set up ";
            }
            void OnMessage(const PtrConnection& conn,Buffer* buf){
                HttpContext *context = conn->GetConext()->get<HttpContext>();
                context->RecvHttpReq(buf);                //分析请求
                HttpRequest& req = context->GetReq();
                HttpResponse res(context->RespStatus());
                if (context->RespStatus() >= 400)
                {
                    std::cout << "400" << std::endl;
                    ErrorHandler(req, &res);//填充一个错误显示页面数据到rsp中
                    WriteResponse(conn, req, res);//组织响应发送给客户端
                    context->Reset();
                    buf->MoveReadOffset(buf->ReadableSize());//出错了就把缓冲区数据清空
                    conn->Shutdown();
                    return;
                }
                if(context->RecvStatus()!=RECV_HTTP_OVER){
                    std::cout << "!=RECV_HTTP_OVER" << std::endl;
                    return;
                }
                std::cout << "Router" << std::endl;
                Router(req, &res);
                WriteResponse(conn, req, res);//组织响应发送给客户端
                context->Reset();
                LOG(logLevel::DEBUG) << "_in_buffer.size: " << buf->ReadableSize();
                if (res.Close() == true)
                    conn->Shutdown(); // 短链接则直接关闭
            }

        public:
            HttpServer(int port,int timeout=DEFAULT_TIMEOUT):_port(port),_server(port),_asset_dir("./wwwroot/"){
                _server.SetConnectedCallback(std::bind(&HttpServer::OnConnected,this,std::placeholders::_1));
                _server.SetMessageCallback(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1,std::placeholders::_2));
                _server.EnableInactiveRelease(timeout);

            }
            void AddGetMethod(std::string pattern, Handle_t handler){ 
                _get_router.push_back(std::make_pair(std::regex(pattern),handler));
            }
            void AddPostMethod(std::string pattern, Handle_t handler){
                _post_router.push_back(std::make_pair(std::regex(pattern),handler));

            }
            void AddPutMethod(std::string pattern, Handle_t handler){
                _put_router.push_back(std::make_pair(std::regex(pattern),handler));
            }
            void AddDeleteMethod(std::string pattern, Handle_t handler){
                _delete_router.push_back(std::make_pair(std::regex(pattern),handler));
            }
            void SetAssetDir(const std::string& dir){
                _asset_dir = dir;
            }
            void SetThreadCount(int count){
                _server.SetThreadsCount(count);
            }
            void Start(){
                _server.Start();
            }
    };
    class netWork
    {
        public:
            netWork(){
                signal(SIGPIPE, SIG_IGN);
                LOG(logLevel::INFO) << "server init pipe";
            }
    };
    static netWork nw;
}