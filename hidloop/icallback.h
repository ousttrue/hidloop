#pragma once
#include <vector>


namespace hid {

class Device;
class ICallback
{
public:
    virtual ~ICallback(){}    
    virtual void onConnect(Device *device)=0;
    virtual void onRead(Device *device, const unsigned char *data, size_t size)=0;
    virtual void onDestroy(Device *device)=0;
};

}
