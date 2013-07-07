#pragma once
#include <vector>


namespace hid {

class Device;
class ICallback
{
public:
    virtual ~ICallback(){}    
    virtual void onConnect(Device *device)=0;
    virtual void onRead(Device *device, std::vector<unsigned char> &data)=0;
    virtual void onDestroy(Device *device)=0;
};

}
