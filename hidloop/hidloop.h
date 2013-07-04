#pragma once
#include <list>
#include <functional>


namespace hid
{
    class Device
    {
        unsigned short m_vendorID;
        unsigned short m_productID;

    public:
        Device(unsigned short vendorID, unsigned short productID)
            : m_vendorID(vendorID), m_productID(productID)
            {
            }
    };


    typedef std::function<bool(unsigned short, unsigned short)> DetectDevice;


    bool initialize();
    void search(std::list<Device> &list, DetectDevice find);
}

