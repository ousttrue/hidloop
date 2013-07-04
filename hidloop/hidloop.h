#pragma once
#include <functional>
#include <memory>


namespace hid
{
    class Device
    {
        unsigned short m_vendorID;
        unsigned short m_productID;
        std::string m_path;

    public:
        Device(unsigned short vendorID, unsigned short productID)
            : m_vendorID(vendorID), m_productID(productID)
            {
            }

        void setPath(const std::string &path){ m_path=path; }
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
    };
}

