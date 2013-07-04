#pragma once

namespace hid {

class IOnRead
{
public:
    virtual ~IOnRead(){}    
    virtual void onRead(const char *data, size_t size)=0;
};

}