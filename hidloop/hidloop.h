#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>


namespace hid
{
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
        bool open(boost::asio::io_service &io);
        void beginRead();
    };


    typedef std::function<bool(unsigned short, unsigned short)> DetectDevice;


    class DeviceManagerImpl;
    class DeviceManager
    {
        std::unique_ptr<DeviceManagerImpl> m_impl;
    public:
        DeviceManager();
        ~DeviceManager();
        void search(DetectDevice detect);
        std::shared_ptr<Device> getDevice(size_t index);
    };
}

