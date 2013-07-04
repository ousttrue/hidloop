#pragma once
#include <memory>
#include <functional>

namespace hid {

typedef std::function<bool(unsigned short, unsigned short)> DetectDevice;

class DeviceManagerImpl;
class Device;
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
