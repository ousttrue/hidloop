#pragma once
#include <memory>
#include <vector>


namespace hid { 
    
class Device;
    
namespace wiimote {

class Wiimote;
class IMemoryTask
{
public:
    enum MEMORY_TYPE {
        MEMORY_NONE,
        MEMORY_READ,
        MEMORY_WRITE,
    };
    virtual ~IMemoryTask(){}
    virtual MEMORY_TYPE type()const=0;
    virtual int address()const=0;
    virtual unsigned char data()const=0;
    virtual void send(Device *device)=0;
    virtual void process(Wiimote *wiimote, std::vector<unsigned char> &buff)=0;

    static std::shared_ptr<IMemoryTask> Create_WriteExtensionInitializeTask1();
    static std::shared_ptr<IMemoryTask> Create_WriteExtensionInitializeTask2();
    static std::shared_ptr<IMemoryTask> Create_WriteMotionPlusEnableTask();
    static std::shared_ptr<IMemoryTask> Create_ReadExtensionTypeTask();
    static std::shared_ptr<IMemoryTask> Create_ReadCalibrationTask();
    static std::shared_ptr<IMemoryTask> Create_ReadExtensionCalibrationTask();
};

}}
