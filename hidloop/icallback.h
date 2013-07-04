#pragma once
#include <vector>


namespace hid {

class Device;
class ICallback
{
public:
    virtual ~ICallback(){}    
    virtual void onConnect(std::shared_ptr<Device> device)=0;
    virtual void onRead(std::shared_ptr<Device> device, const unsigned char *data, size_t size)=0;
    virtual void onDestroy(std::shared_ptr<Device> device)=0;
};

}
