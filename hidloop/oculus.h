#pragma once
#include "icallback.h"


namespace hid { namespace oculus {


bool detect(unsigned short vendor_id, unsigned short product_id);


class Oculus: public ICallback
{
public:
    void onConnect(Device *device)override;
    void onRead(Device *device, std::vector<unsigned char> &data)override;
    void onDestroy(Device *device)override;
};


}}
