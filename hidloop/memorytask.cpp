#include "memorytask.h"
#include "device.h"
#include "wiimote.h"
#include "wiimote_const.h"
#include "extension.h"
#include <vector>


// wiimote registers
static const int REGISTER_CALIBRATION			= 0x0016;
static const int REGISTER_IR					= 0x4b00030;
static const int REGISTER_IR_SENSITIVITY_1		= 0x4b00000;
static const int REGISTER_IR_SENSITIVITY_2		= 0x4b0001a;
static const int REGISTER_IR_MODE				= 0x4b00033;
static const int REGISTER_EXTENSION_INIT1		= 0x4a400f0;
static const int REGISTER_EXTENSION_INIT2		= 0x4a400fb;
static const int REGISTER_EXTENSION_TYPE		= 0x4a400fa;
static const int REGISTER_EXTENSION_CALIBRATION	= 0x4a40020;
static const int REGISTER_BALANCE_CALIBRATION	= 0x4a40024;
static const int REGISTER_MOTIONPLUS_DETECT		= 0x4a600fa;
static const int REGISTER_MOTIONPLUS_INIT		= 0x4a600f0;
static const int REGISTER_MOTIONPLUS_ENABLE		= 0x4a600fe;


namespace hid {


class IWriteMemoryTask: public IMemoryTask
{
public:
	MEMORY_TYPE type()const override{ return IMemoryTask::MEMORY_WRITE; }

    void send(hid::Device *device) override
    {
std::cout << __FUNCTION__ << std::endl;
        int addr=address();
        unsigned char size=1;
        const unsigned char buff[]={ data() };

        std::vector<unsigned char> write(REPORT_LENGTH, 0);
        write[0] = OUT_WRITEMEMORY;
        write[1] = (unsigned char)(((addr & 0xff000000) >> 24)); // | GetRumbleBit());
        write[2] = (unsigned char)( (addr & 0x00ff0000) >> 16);
        write[3] = (unsigned char)( (addr & 0x0000ff00) >>  8);
        write[4] = (unsigned char)( (addr & 0x000000ff));
        write[5] = size;
        memcpy(&write[6], buff, size);
        device->write(write);
    }

	void process(Wiimote *wiimote, std::vector<unsigned char> &buff)override{}
};


class IReadMemoryTask: public IMemoryTask
{
public:
	MEMORY_TYPE type()const override{ return IMemoryTask::MEMORY_READ; }

    void send(hid::Device *device) override
    {
std::cout << __FUNCTION__ << std::endl;
        int addr=address();
        short size=data();

        std::vector<unsigned char> buff(REPORT_LENGTH, 0);
        buff[0] = OUT_READMEMORY;
        buff[1] = (unsigned char)(((addr & 0xff000000) >> 24)); // | GetRumbleBit());
        buff[2] = (unsigned char)( (addr & 0x00ff0000) >> 16);
        buff[3] = (unsigned char)( (addr & 0x0000ff00) >>  8);
        buff[4] = (unsigned char)( (addr & 0x000000ff));
        buff[5] = (unsigned char)( (size	   & 0xff00	   ) >>  8);
        buff[6] = (unsigned char)( (size	   & 0xff));
        device->write(buff);
    }

    void process(Wiimote *wiimote, std::vector<unsigned char> &buff)override
    {
        // decode the address that was queried:
        int addr = buff[4]<<8 | buff[5];
        int size    = buff[3] >> 4;

        //int changed	= 0;

        assert(addr==(address() & 0xFFFF));
        //assert((size+1)==data());

        read(wiimote, &buff[6]);
    }

    virtual void read(Wiimote *wiimote, const unsigned char *buff)=0;
};


class WriteExtensionInitializeTask1: public IWriteMemoryTask
{
public:
    int address()const override{ return REGISTER_EXTENSION_INIT1; }
    unsigned char data()const override{ return 0x55; }
};


class WriteExtensionInitializeTask2: public IWriteMemoryTask
{
public:
    int address()const override{ return REGISTER_EXTENSION_INIT2; }
    unsigned char data()const override{ return 0x00; }
};


class WriteMotionPlusEnableTask: public IWriteMemoryTask
{
public:
    int address()const override{ return REGISTER_MOTIONPLUS_ENABLE; }
    unsigned char data()const override{ return 0x04; }
};


class ReadExtensionTypeTask: public IReadMemoryTask
{
public:
    int address()const override{ return REGISTER_EXTENSION_TYPE; }
    unsigned char data()const override{ return 6; }
    void read(Wiimote *wiimote, const unsigned char *buff)override
    {
std::cout << __FUNCTION__ << std::endl;
        QWORD type = *((QWORD*)buff);
        wiimote->setExtension(IExtension::Create(type));
    }
};


class ReadCalibrationTask: public IReadMemoryTask
{
public:
    int address()const override{ return REGISTER_CALIBRATION; }
    unsigned char data()const override{ return 7; }
    void read(Wiimote *wiimote, const unsigned char *buff)override
    {
std::cout << __FUNCTION__ << std::endl;
        auto &state=wiimote->getState();
        state.CalibrationInfo.X0 = buff[0];
        state.CalibrationInfo.Y0 = buff[1];
        state.CalibrationInfo.Z0 = buff[2];
        state.CalibrationInfo.XG = buff[4];
        state.CalibrationInfo.YG = buff[5];
        state.CalibrationInfo.ZG = buff[6];
    }
};


class ReadExtensionCalibrationTask: public IReadMemoryTask
{
    int address()const override{ return REGISTER_EXTENSION_CALIBRATION; }
    unsigned char data()const override{ return 16; }
    void read(Wiimote *wiimote, const unsigned char *buff)override
    {
		wiimote->getExtension()->calibration(wiimote, buff);
    }
};


std::shared_ptr<IMemoryTask> IMemoryTask::Create_WriteExtensionInitializeTask1()
{
    return std::make_shared<WriteExtensionInitializeTask1>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_WriteExtensionInitializeTask2()
{
    return std::make_shared<WriteExtensionInitializeTask2>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_WriteMotionPlusEnableTask()
{
    return std::make_shared<WriteMotionPlusEnableTask>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_ReadExtensionTypeTask()
{
    return std::make_shared<ReadExtensionTypeTask>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_ReadCalibrationTask()
{
    return std::make_shared<ReadCalibrationTask>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_ReadExtensionCalibrationTask()
{
    return std::make_shared<ReadExtensionCalibrationTask>();
}

}
