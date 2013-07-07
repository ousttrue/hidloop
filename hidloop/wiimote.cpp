#include <boost/asio.hpp>
#include "wiimote.h"
#include "device.h"
#include "wiimote_const.h"
#include <iostream>
#include <mmsystem.h>


static void send_SetReportType(hid::Device *device, IN_TYPE type, bool continuous=false)
{
    std::vector<unsigned char> buff(REPORT_LENGTH, 0);
    buff[0] = (BYTE)OUT_TYPE;
    buff[1] = (continuous ? 0x04 : 0x00); //| GetRumbleBit();
    buff[2] = (BYTE)type;
    device->write(buff);
}

static void send_RequestStatusReport(hid::Device *device)
{
    // (this can be called before we're fully connected)
    std::vector<unsigned char> buff(REPORT_LENGTH, 0);
    buff[0] = OUT_STATUS;
    buff[1] = 0; //GetRumbleBit();
    device->write(buff);
}


namespace hid {



// static
bool Wiimote::detect(unsigned short vendor_id, unsigned short product_id)
{
    if(vendor_id!=0x057e){
        return false;
    }
    // Nintendo

    if(product_id==0x0306){
        // old model
        return true;
    }

    if(product_id==0x0330){
        // internal Wiimote Plus
        return true;
    }

    return false;
}


Wiimote::Wiimote()
    : m_lastTime(0), bMotionPlusDetected(false)
{
}

Wiimote::~Wiimote()
{
}

void Wiimote::onConnect(Device *device)
{
    // reset
	send_SetReportType(device, IN_BUTTONS_ACCEL_EXT, false);

    send_RequestStatusReport(device);

    // calibration
	pushMemoryTask(device, IMemoryTask::Create_ReadCalibrationTask());

	//pushMemoryTask(device, IMemoryTask::Create_WriteExtensionInitializeTask1());
	//pushMemoryTask(device, IMemoryTask::Create_WriteExtensionInitializeTask2());
	pushMemoryTask(device, IMemoryTask::Create_WriteMotionPlusEnableTask());
	pushMemoryTask(device, IMemoryTask::Create_ReadExtensionTypeTask());

	//send_SetReportType(device, IN_BUTTONS_ACCEL_EXT, false);
}

void Wiimote::onRead(Device *device, std::vector<unsigned char> &data)
{
    if(data.empty()){
        return;
    }
    auto now=timeGetTime();
    auto d=now-m_lastTime;
    m_lastTime=now;

    IN_TYPE type=static_cast<IN_TYPE>(data[0]);
    switch(type)
    {
        case IN_STATUS:
            ParseStatus(device, data);
            // show that we received the status report (used for output method
            //  detection during Connect())
            //bStatusReceived = true;
            break;

        case IN_READADDRESS:
            ParseButtons(data);
            onMemory(device, data);
            break;

        case IN_ACK:
            onMemory(device, data);
            break;

        case IN_BUTTONS:
            ParseButtons(data);
            break;

        case IN_BUTTONS_ACCEL:
            ParseButtons(data);
            ParseAccel(data);
            break;

		case IN_BUTTONS_ACCEL_EXT:
			ParseButtons(data);
            ParseAccel(data);
			if(m_extension){
				m_extension->parse(this, data);
			}
			break;

        default:
            std::cout << "unknown data: " << type << std::endl;
            break;
    }

    if(m_onStatus){
        m_onStatus(m_state);
    }
}

void Wiimote::onDestroy(Device *device)
{
    send_SetReportType(device, IN_BUTTONS, false);
}

int Wiimote::ParseStatus(Device *device, std::vector<unsigned char> &buff)
{
	int changed=0;

    // parse the buttons
    ParseButtons(buff);

    // get the battery level
    BYTE battery_raw = buff[6];
    if(m_state.BatteryRaw != battery_raw)
        changed |= BATTERY_CHANGED;
    m_state.BatteryRaw	 = battery_raw;
    // it is estimated that ~200 is the maximum battery level
    m_state.BatteryPercent = battery_raw / 2;

    // there is also a flag that shows if the battery is nearly empty
    bool drained = buff[3] & 0x01;
	/*
    if(drained != bBatteryDrained)
    {
        bBatteryDrained = drained;
        if(drained)
            changed |= BATTERY_DRAINED;
    }
	*/

    // leds
    BYTE leds = buff[3] >> 4;
    if(leds != m_state.LED.Bits)
        changed |= LEDS_CHANGED;
    m_state.LED.Bits = leds;

    // don't handle extensions until a connection is complete
    //	if(bConnectInProgress)
    //		return changed;

    bool extension = ((buff[3] & 0x02) != 0);
    //	TRACE(_T("(extension = %s)"), (extension? _T("TRUE") : _T("false")));

    //if(extension != m_state.bExtension)
    {
        if(!m_state.bExtension)
        {
            //TRACE(_T("Extension connected:"));
            m_state.bExtension = true;
            //initializeExtension(device);
        }
        else{
            //TRACE(_T("Extension disconnected."));
            m_state.bExtension	   = false;
            //m_state.ExtensionType = wiimote_state::NONE;
            //bMotionPlusEnabled	   = false;
            //bMotionPlusExtension   = false;
            bMotionPlusDetected	   = false;
            //bInitInProgress		   = false;
            //bEnablingMotionPlus	   = false;
            changed				  |= EXTENSION_DISCONNECTED;
            // renable reports
            //			SetReportType(ReportType);
        }
    }

    return changed;
}

void Wiimote::ParseButtons(std::vector<unsigned char> &buff)
{
    unsigned short bits = *(unsigned short*)(&buff[1]) & m_state.Button.ALL;
    m_state.Button.Bits=bits;
}

void Wiimote::ParseAccel(std::vector<unsigned char> &buff)
{
    BYTE raw_x = buff[3];
    BYTE raw_y = buff[4];
    BYTE raw_z = buff[5];

    m_state.Acceleration.RawX = raw_x;
    m_state.Acceleration.RawY = raw_y;
    m_state.Acceleration.RawZ = raw_z;

    // avoid / 0.0 when calibration data hasn't arrived yet
    if(m_state.CalibrationInfo.X0)
    {
        m_state.Acceleration.X =
            ((float)m_state.Acceleration.RawX  - m_state.CalibrationInfo.X0) / 
            ((float)m_state.CalibrationInfo.XG - m_state.CalibrationInfo.X0);
        m_state.Acceleration.Y =
            ((float)m_state.Acceleration.RawY  - m_state.CalibrationInfo.Y0) /
            ((float)m_state.CalibrationInfo.YG - m_state.CalibrationInfo.Y0);
        m_state.Acceleration.Z =
            ((float)m_state.Acceleration.RawZ  - m_state.CalibrationInfo.Z0) /
            ((float)m_state.CalibrationInfo.ZG - m_state.CalibrationInfo.Z0);
    }
    else{
        m_state.Acceleration.X =
            m_state.Acceleration.Y =
            m_state.Acceleration.Z = 0.f;
    }

    /*
    // see if we can estimate the orientation from the current values
    if(EstimateOrientationFrom(m_state.Acceleration))
        changed |= ORIENTATION_CHANGED;
        */
}

//////////////////////////////////////////////////////////////////////////////
void Wiimote::pushMemoryTask(Device *device, std::shared_ptr<IMemoryTask> task)
{
    m_memoryQueue.push_back(task);
    dequeue(device);
}

void Wiimote::dequeue(Device *device)
{
    if(m_currentTask){
        return;
    }
	if(m_memoryQueue.empty()){
		return;
	}
    m_currentTask=m_memoryQueue.front();
    m_memoryQueue.pop_front();
    m_currentTask->send(device);
}

void Wiimote::onMemory(Device *device, std::vector<unsigned char> &buff)
{
    assert(m_currentTask);
    // do task
    m_currentTask->process(this, buff);
    // done
    m_currentTask=0;

	dequeue(device);
}

}
