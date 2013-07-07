#pragma once
#include "icallback.h"
#include "wiimote_state.h"
#include "memorytask.h"
#include <vector>
#include <functional>
#include <list>

namespace hid {


class Wiimote: public ICallback
{
    wiimote_state m_state;
    unsigned int m_lastTime;
    std::function<void(const wiimote_state &)> m_onStatus;
    bool bMotionPlusDetected;

public:
    static bool detect(unsigned short vendor_id, unsigned short product_id);
    Wiimote();
    ~Wiimote();
    const wiimote_state &getState()const{ return m_state; }
    wiimote_state &getState(){ return m_state; }
    void onConnect(Device *device)override;
    void onRead(Device *device, const unsigned char *data, size_t size)override;
    void onDestroy(Device *device)override;
    void setOnStatus(std::function<void(const wiimote_state &)> onStatus){ m_onStatus=onStatus; }

private:
    int ParseStatus(Device *device, const unsigned char *buff);
    int ParseExtension(const unsigned char *buff, unsigned offset);
    void ParseButtons(const unsigned char* buff);
    void ParseAccel(const unsigned char* buff);

    std::list<std::shared_ptr<IMemoryTask>> m_memoryQueue;
    std::shared_ptr<IMemoryTask> m_currentTask;
    void pushMemoryTask(Device *device, std::shared_ptr<IMemoryTask> task);
    void dequeue(Device *device);
    void onMemory(Device *device, const unsigned char *data);
    void initializeExtension(Device *device);
};

}
