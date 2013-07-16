#pragma once
#include "icallback.h"
#include "wiimote/state.h"
#include "wiimote/memorytask.h"
#include "wiimote/extension.h"
#include <vector>
#include <functional>
#include <list>

namespace hid { namespace wiimote {


class IExtension;
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
    void onRead(Device *device, std::vector<unsigned char> &data)override;
    void onDestroy(Device *device)override;
    void setOnStatus(std::function<void(const wiimote_state &)> onStatus){ m_onStatus=onStatus; }

private:
    std::shared_ptr<IExtension> m_extension;
public:
    void setExtension(std::shared_ptr<IExtension> extension){ m_extension=extension; }
    std::shared_ptr<IExtension> getExtension(){ return m_extension; }

private:
    int ParseStatus(Device *device, std::vector<unsigned char> &buff);
    int ParseExtension(std::vector<unsigned char> &buff, unsigned offset);
    void ParseButtons(std::vector<unsigned char> &buff);
    void ParseAccel(std::vector<unsigned char> &buff);

    // IMemoryTask
    std::list<std::shared_ptr<IMemoryTask>> m_memoryQueue;
    std::shared_ptr<IMemoryTask> m_currentTask;
    void pushMemoryTask(Device *device, std::shared_ptr<IMemoryTask> task);
    void dequeue(Device *device);
    void onMemory(Device *device, std::vector<unsigned char> &buff);
};

}}
