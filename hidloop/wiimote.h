#pragma once
#include "icallback.h"
#include "wiimote_state.h"
#include <vector>

namespace hid {

class Wiimote: public ICallback
{
    wiimote_state m_state;
    unsigned int m_lastTime;

public:
    static bool detect(unsigned short vendor_id, unsigned short product_id);
    Wiimote();
    ~Wiimote();
    void onConnect(std::shared_ptr<Device> device)override;
    void onRead(std::shared_ptr<Device> device, const unsigned char *data, size_t size)override;
    void onDestroy(std::shared_ptr<Device> device)override;
    void enableAccel(std::shared_ptr<Device> device);

private:
    void ParseAccel(const unsigned char* buff);
};

}
