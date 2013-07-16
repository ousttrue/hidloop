#pragma once
#include <string>
#include <memory>
#include <vector>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <boost/asio.hpp>
#endif

#include "icallback.h"


namespace hid {

class Device: public std::enable_shared_from_this<Device>
{
    unsigned short m_vendorID;
    unsigned short m_productID;
    std::string m_path;
    char m_buf[1024];
    std::shared_ptr<boost::asio::windows::stream_handle> m_stream;
    std::shared_ptr<ICallback> m_callback;
    std::shared_ptr<void> m_handle;

public:
    Device(unsigned short vendorID, unsigned short productID);
    ~Device();
    void setPath(const std::string &path){ m_path=path; }
    bool open(boost::asio::io_service &io, std::shared_ptr<ICallback> callback);
    void write(std::vector<unsigned char> &data);

private:
    void beginRead();
};

}
