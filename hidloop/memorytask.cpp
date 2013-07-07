#include "memorytask.h"
#include "device.h"
#include "wiimote.h"
#include "wiimote_const.h"
#include <vector>


#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif


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

	void process(Wiimote *wiimote, const unsigned char *data)override{}
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

    void process(Wiimote *wiimote, const unsigned char* buff)override
    {
        // decode the address that was queried:
        int addr = buff[4]<<8 | buff[5];
        int size    = buff[3] >> 4;

        //int changed	= 0;

        assert(addr==(address() & 0xFFFF));
        //assert(size==data());

        read(wiimote, buff+6);
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


class ReadExtensionTypeTask: public IReadMemoryTask
{
public:
    int address()const override{ return REGISTER_EXTENSION_TYPE; }
    unsigned char data()const override{ return 6; }
    void read(Wiimote *wiimote, const unsigned char *buff)override
    {
std::cout << __FUNCTION__ << std::endl;
        auto &state=wiimote->getState();
        QWORD type = *(QWORD*)buff;
		state.ExtensionType=static_cast<wiimote_state::extension_type>(type);
        //readMemory(device, REGISTER_EXTENSION_CALIBRATION, 16);
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


std::shared_ptr<IMemoryTask> IMemoryTask::Create_WriteExtensionInitializeTask1()
{
    return std::make_shared<WriteExtensionInitializeTask1>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_WriteExtensionInitializeTask2()
{
    return std::make_shared<WriteExtensionInitializeTask2>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_ReadExtensionTypeTask()
{
    return std::make_shared<ReadExtensionTypeTask>();
}

std::shared_ptr<IMemoryTask> IMemoryTask::Create_ReadCalibrationTask()
{
    return std::make_shared<ReadCalibrationTask>();
}

/*
std::shared_ptr<IMemoryTask> IMemoryTask::Create_ReadExtensionCalibrationTask()
{
    return std::make_shared<ReadExtensionCalibrationTask>();
}
*/

#if 0
int Wiimote::ParseReadAddress(Device *device, const unsigned char* buff)
{
	switch(address)
    {
		case (REGISTER_EXTENSION_CALIBRATION & 0xffff):
		case (REGISTER_BALANCE_CALIBRATION   & 0xffff):
			{
//			_ASSERT(((m_state.ExtensionType == BALANCE_BOARD) && (size == 31)) ||
//					((m_state.ExtensionType != BALANCE_BOARD) && (size == 15)));

			switch(m_state.ExtensionType)
				{
				case wiimote_state::NUNCHUK:
					{
					wiimote_state::nunchuk::calibration_info
						&calib = m_state.Nunchuk.CalibrationInfo;

					calib.X0   = buff[ 0];
					calib.Y0   = buff[ 1];
					calib.Z0   = buff[ 2];
					calib.XG   = buff[ 4];
					calib.YG   = buff[ 5];
					calib.ZG   = buff[ 6];
					calib.MaxX = buff[ 8];
					calib.MinX = buff[ 9];
					calib.MidX = buff[10];
					calib.MaxY = buff[11];
					calib.MinY = buff[12];
					calib.MidY = buff[13];

					changed |= NUNCHUK_CONNECTED;//|NUNCHUK_CALIBRATION_CHANGED;
					// reenable reports
//					SetReportType(ReportType);
					}
					break;
				
				case wiimote_state::CLASSIC:
				case wiimote_state::GH3_GHWT_GUITAR:
				case wiimote_state::GHWT_DRUMS:
					{
					wiimote_state::classic_controller::calibration_info
						&calib = m_state.ClassicController.CalibrationInfo;
					
					calib.MaxXL = buff[ 0] >> 2;
					calib.MinXL = buff[ 1] >> 2;
					calib.MidXL = buff[ 2] >> 2;
					calib.MaxYL = buff[ 3] >> 2;
					calib.MinYL = buff[ 4] >> 2;
					calib.MidYL = buff[ 5] >> 2;
					calib.MaxXR = buff[ 6] >> 3;
					calib.MinXR = buff[ 7] >> 3;
					calib.MidXR = buff[ 8] >> 3;
					calib.MaxYR = buff[ 9] >> 3;
					calib.MinYR = buff[10] >> 3;
					calib.MidYR = buff[11] >> 3;
					// this doesn't seem right...
					//	calib.MinTriggerL = buff[12] >> 3;
					//	calib.MaxTriggerL = buff[14] >> 3;
					//	calib.MinTriggerR = buff[13] >> 3;
					//	calib.MaxTriggerR = buff[15] >> 3;
					calib.MinTriggerL = 0;
					calib.MaxTriggerL = 31;
					calib.MinTriggerR = 0;
					calib.MaxTriggerR = 31;

					changed |= CLASSIC_CONNECTED;//|CLASSIC_CALIBRATION_CHANGED;
					// reenable reports
//					SetReportType(ReportType);
					}
					break;

				case wiimote_state::BALANCE_BOARD:
					{
					// first part, 0 & 17kg calibration values
					wiimote_state::balance_board::calibration_info
						&calib = m_state.BalanceBoard.CalibrationInfo;

					calib.Kg0 .TopR	   = (short)((short)buff[0] << 8 | buff[1]);
					calib.Kg0 .BottomR = (short)((short)buff[2] << 8 | buff[3]);
					calib.Kg0 .TopL	   = (short)((short)buff[4] << 8 | buff[5]);
					calib.Kg0 .BottomL = (short)((short)buff[6] << 8 | buff[7]);

					calib.Kg17.TopR	   = (short)((short)buff[8] << 8 | buff[9]);
					calib.Kg17.BottomR = (short)((short)buff[10] << 8 | buff[11]);
					calib.Kg17.TopL	   = (short)((short)buff[12] << 8 | buff[13]);
					calib.Kg17.BottomL = (short)((short)buff[14] << 8 | buff[15]);

					// 2nd part is scanned above
					}
					break;

				case wiimote_state::MOTION_PLUS:
					{
					// TODO: not known how the calibration values work
					changed |= MOTIONPLUS_ENABLED;
					//bMotionPlusEnabled = true;
					//bInitInProgress	   = false;
					// reenable reports
//					SetReportType(ReportType);
					}
					break;
				}
			case 0x34:
				{
				if(m_state.ExtensionType == wiimote_state::BALANCE_BOARD)
					{
					wiimote_state::balance_board::calibration_info
						&calib = m_state.BalanceBoard.CalibrationInfo;

					// 2nd part of the balance board calibration,
					//  34kg calibration values
					calib.Kg34.TopR    = (short)((short)buff[0] << 8 | buff[1]);
					calib.Kg34.BottomR = (short)((short)buff[2] << 8 | buff[3]);
					calib.Kg34.TopL    = (short)((short)buff[4] << 8 | buff[5]);
					calib.Kg34.BottomL = (short)((short)buff[6] << 8 | buff[7]);
						
					changed |= BALANCE_CONNECTED;
					// reenable reports
					send_SetReportType(device, IN_BUTTONS_BALANCE_BOARD);
					}
				// else unknown what these are for
				}
			//bInitInProgress = false;
			}
			break;

		default:
//			_ASSERT(0); // shouldn't happen
			break;
		}
	
	return changed;
	}

#endif

}
