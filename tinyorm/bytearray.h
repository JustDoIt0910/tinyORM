//
// Created by zr on 22-11-4.
//

#ifndef __BYTEARRAY_HPP__
#define __BYTEARRAY_HPP__

#include <string.h>
#include "spdlog/spdlog.h"

class ByteArray
{
public:
    ByteArray(): len(0)
    {
        buf = wp = nullptr;
    }

    ByteArray(int _len): len(_len)
    {
        buf = (char*)malloc(_len);
        if(buf == nullptr)
            spdlog::warn("Malloc failed(out of memory)");
        memset(buf, 0, len);
        wp = buf;
    }

    ByteArray(const char *data, int offset, int _len): ByteArray(_len)
    {
        if(buf == nullptr)
            return;
        memcpy(buf, data + offset, _len);
        wp += _len;
    }

    ByteArray(const char *data, int _len): ByteArray(data, 0, _len){}

    ByteArray(std::ifstream &ifs, int _len): ByteArray(_len)
    {
        ifs.read(buf, len);
        wp += len;
    }

    ~ByteArray()
    {
        if(buf != nullptr)
        {
            free(buf);
            buf = wp = nullptr;
        }
    }

    int length() {return len;}

    char* value() {return buf;}

    ByteArray(const ByteArray& arr): len(arr.len)
    {
        if(this == &arr)
            return;
        buf = (char*)malloc(arr.len);
        if(buf == nullptr)
        {
            spdlog::warn("Malloc failed(out of memory)");
            return;
        }
        memcpy(buf, arr.buf, arr.len);
        wp = buf + len;
    }

    ByteArray& operator=(const ByteArray& arr)
    {
        if(this == &arr)
            return *this;
        if(buf != nullptr)
            free(buf);
        len = arr.len;
        buf = (char*)malloc(arr.len);
        if(buf == nullptr)
        {
            spdlog::warn("Malloc failed(out of memory)");
            return *this;
        }
        memcpy(buf, arr.buf, arr.len);
        wp = buf + len;
        return *this;
    }

    ByteArray(ByteArray&& arr): len(arr.len)
    {
        if(this == &arr)
            return;
        buf = arr.buf;
        wp = arr.wp;
        arr.buf = nullptr;
        arr.wp = nullptr;
    }

    ByteArray& operator=(ByteArray&& arr)
    {
        if(this == &arr)
            return *this;
        len = arr.len;
        if(buf != nullptr)
            free(buf);
        buf = arr.buf;
        wp = arr.wp;
        arr.buf = nullptr;
        arr.wp = nullptr;
        return *this;
    }

    void fill(const char *data, int _len, int start, int offset)
    {
        if(start + _len > len)
        {
            spdlog::warn("index out of range in fill(const char *data, int _len, int start, int offset)");
            return;
        }
        memcpy(buf + start, data + offset, _len);
    }

    void fill(const char *data, int _len, int offset)
    {
        fill(data, _len, wp - buf, offset);
        wp += _len;
    }

    void fill(const char *data, int _len)
    {
        fill(data, _len, 0);
    }

    void fill(char val, int _len)
    {
        int cnt = 0;
        while(wp - buf < len && cnt < _len)
        {
            *(wp++) = val;
            cnt++;
        }
    }
private:
    char* buf;
    int len;
    char* wp;
};

#endif
