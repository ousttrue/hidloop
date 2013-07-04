#pragma once
#include <string>
#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include "ionread.h"


namespace hid {

class Device
{
    unsigned short m_vendorID;
    unsigned short m_productID;
    std::string m_path;
    char m_buf[1024];
    std::shared_ptr<boost::asio::windows::stream_handle> m_stream;

public:
    Device(unsigned short vendorID, unsigned short productID)
        : m_vendorID(vendorID), m_productID(productID)
        {
        }

    void setPath(const std::string &path){ m_path=path; }
    bool open(boost::asio::io_service &io, std::shared_ptr<IOnRead> callback);
    void write(std::vector<unsigned char> &data);

private:
    void beginRead(std::shared_ptr<IOnRead> callback);
};

}
