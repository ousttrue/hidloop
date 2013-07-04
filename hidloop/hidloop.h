#pragma once
#include <list>
#include <functional>


namespace hid
{
    class Device
    {
        unsigned long m_vendorID;
        unsigned long m_productID;

    public:
        Device(unsigned long vendorID, unsigned long productID)
            : m_vendorID(vendorID), m_productID(productID)
            {
            }
    };


    typedef std::function<bool(unsigned long, unsigned long)> DetectDevice;


    bool initialize();
    void search(std::list<Device> &list, DetectDevice find);
}

