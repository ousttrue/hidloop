#pragma once
#include <vector>


namespace hid {

class ICallback
{
public:
    virtual ~ICallback(){}    
    virtual void onRead(const unsigned char *data, size_t size)=0;
    virtual std::vector<unsigned char> onDestroy()=0;
};

}
