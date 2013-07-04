#pragma once
#include <string>
#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include "icallback.h"


namespace hid {

class Device
{
    unsigned short m_vendorID;
    unsigned short m_productID;
    std::string m_path;
    char m_buf[1024];
    std::shared_ptr<boost::asio::windows::stream_handle> m_stream;
    std::shared_ptr<ICallback> m_callback;

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