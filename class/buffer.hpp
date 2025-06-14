#include<vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <assert.h>


#define DEFAULTSIZE 1024


//[_read_idx..._write_idx)
namespace byBit{
    class CircleBuffer{
        private:
            std::vector<char> _buffer; 
            uint32_t _read_idx;
            uint32_t _write_idx;
            int _capacity;

        public:
            CircleBuffer(int size=DEFAULTSIZE):_buffer(size+1),_read_idx(0),_write_idx(0),_capacity(size+1){}
            ~CircleBuffer(){}
            bool isFull(){return (_write_idx+1)%_capacity==_read_idx;}
            bool isEmpty(){return _read_idx==_write_idx;}
            int size() { return (_write_idx + _capacity - _read_idx) % _capacity; }
            int Read(std::string &out)
            {
                char *outc = (char *)out.c_str();
                return Read(outc, out.size());
            }
            int Read(char* out,int maxlen)
            {
                if(isEmpty()) return 0;
                int len = std::min(maxlen,size());
                if(_read_idx<_write_idx) //no circle
                {
                    auto iter = _buffer.begin() + _read_idx;
                    std::copy(iter,iter+len,out); 
                    _read_idx = (_read_idx + len) % _capacity;
                }
                else                     //have circle
                {
                    //[..._wr...rd...]
                    int suffix = _capacity - _read_idx, prefix = _write_idx;//remaining data
                    if (len <= suffix)
                    {
                        auto iter = _buffer.begin() + _read_idx;
                        std::copy(iter,iter+len,out);
                        _read_idx = (_read_idx + len) % _capacity;
                    }
                    else{
                        auto iter = _buffer.begin() + _read_idx;
                        std::copy(iter,iter+suffix,out);
                        iter = _buffer.begin();
                        std::copy(iter,iter+len-suffix,out+suffix);
                        _read_idx = len - suffix;
                    }
                }
                return len;
            }
            int Write(const std::string &in) { return Write(in.c_str(), in.size()); }
            int Write(const char* in,int len)
            {
                if(_capacity-1-size() < len)
                {
                    _capacity = _buffer.size() + 2 * len;
                    _buffer.resize(_capacity);
                }
                if(_read_idx<_write_idx)//[...rd...wr...]
                {
                    int suffix = _capacity - _write_idx, prefix = _read_idx;//remaining room
                    if(len <= suffix){
                        auto iter = _buffer.begin() + _write_idx;
                        std::copy(in, in + len, iter);
                        _write_idx = (_write_idx + len) % _capacity;
                    }
                    else{
                        auto iter = _buffer.begin() + _write_idx;
                        std::copy(in, in + suffix, iter);
                        iter = _buffer.begin();
                        std::copy(in + suffix, in + len, iter);
                        _write_idx = len - suffix;
                    }
                }
                else{//[..wr...rd]
                    std::copy(in, in + len, _buffer.begin() + _write_idx);
                    _write_idx = (_write_idx + len) % _capacity;
                }
                return len;
            }

    };
}

namespace byBit{

class Buffer
{
private:
    int _write_idx;
    int _read_idx;
    std::vector<char> _buffer;

public:
    Buffer():_write_idx(0),_read_idx(0),_buffer(DEFAULTSIZE){}
    char *GetWritePos() { return &_buffer[0] + _write_idx; }
    const char *GetReadPos() { return &_buffer[0] + _read_idx; }
    int Capacity() { return _buffer.size(); }
    int PostIdle() { return Capacity() - _write_idx; }
    int PreIdle() { return _read_idx; }
    int ReadableSize() { return _write_idx - _read_idx; }
    void MoveReadOffset(int len) {
        assert(len <= ReadableSize());
        _read_idx += len;
    }
    void MoveWriteOffset(int len) {
        assert(len <= PostIdle());
        _write_idx += len;
    }
    void Clear() { _read_idx = _write_idx = 0; }
    void EnsureWrite(int len){
        if(PostIdle() >= len){
            return;
        }
        if(PreIdle()+PostIdle()>=len){
            std::copy(GetReadPos(), GetReadPos() + ReadableSize(), _buffer.begin());
            _read_idx = 0;
            _write_idx = ReadableSize();
        }else{
            _buffer.resize(Capacity() + 2*len);
        }
    }
    int Write(const void* data,int len){
        EnsureWrite(len);
        const char *Data = (const char*)data;
        std::copy(Data, Data + len, GetWritePos());
        return len;
    }
    int Write(const std::string& s){
        return Write(s.c_str(), s.size());
    }
    int WriteAndPush(const void* data,int len){
        Write(data, len);
        MoveWriteOffset(len);
        return len;
    }
    int WriteAndPush(const std::string& str){
        return WriteAndPush(str.c_str(), str.size());
    }
    int Read(void* d,int len){
        int lenth = std::min(ReadableSize(), len);
        char *data = (char *)d;
        std::copy(GetReadPos(), GetReadPos() + lenth, data);
        return len;
    }
    int Read(std::string* out){
        char *ot = (char*)out->c_str();
        return Read(ot, out->size());
    }
    int ReadAndPop(void* d,int len){
        int l = Read(d, len);
        MoveReadOffset(l);
        return l;
    }
    int ReadAndPop(std::string* out){
        return ReadAndPop((char *)out->c_str(), out->size());
    }
    std::string GetLine(){
        
    }
};
}