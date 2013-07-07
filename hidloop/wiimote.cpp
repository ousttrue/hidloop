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

void Wiimote::initializeExtension(Device *device)
{
	pushMemoryTask(device, IMemoryTask::Create_WriteExtensionInitializeTask1());
	pushMemoryTask(device, IMemoryTask::Create_WriteExtensionInitializeTask2());
	pushMemoryTask(device, IMemoryTask::Create_ReadExtensionTypeTask());
}

void Wiimote::onConnect(Device *device)
{
    // reset
	send_SetReportType(device, IN_BUTTONS, false);

    send_RequestStatusReport(device);

    // calibration
    //readMemory(device, REGISTER_CALIBRATION, 7);
	pushMemoryTask(device, IMemoryTask::Create_ReadCalibrationTask());
	initializeExtension(device);

	/*
    if(true)
    {
        send_InitializeExtension(device);
        send_SetReportType(device, IN_BUTTONS_ACCEL_EXT, false); 
    }
    else if(true){
        // accel
        send_SetReportType(device, IN_BUTTONS_ACCEL, false); 
    }
	*/
}

void Wiimote::onRead(Device *device, const unsigned char *data, size_t size)
{
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
			ParseExtension(data, 6);
            ParseAccel(data);
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

int Wiimote::ParseStatus(Device *device, const unsigned char *buff)
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
            initializeExtension(device);
        }
        else{
            //TRACE(_T("Extension disconnected."));
            m_state.bExtension	   = false;
            m_state.ExtensionType = wiimote_state::NONE;
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

int Wiimote::ParseExtension(const unsigned char *buff, unsigned offset)
{
    int changed = 0;

    switch(m_state.ExtensionType)
    {
        case wiimote_state::NUNCHUK:
            {
                // buttons
                bool c = (buff[offset+5] & 0x02) == 0;
                bool z = (buff[offset+5] & 0x01) == 0;

                if((c != m_state.Nunchuk.C) || (z != m_state.Nunchuk.Z))
                    changed |= NUNCHUK_BUTTONS_CHANGED;

                m_state.Nunchuk.C = c;
                m_state.Nunchuk.Z = z;

                // acceleration
                {
                    wiimote_state::acceleration &accel = m_state.Nunchuk.Acceleration;

                    BYTE raw_x = buff[offset+2];
                    BYTE raw_y = buff[offset+3];
                    BYTE raw_z = buff[offset+4];
                    if((raw_x != accel.RawX) || (raw_y != accel.RawY) || (raw_z != accel.RawZ))
                        changed |= NUNCHUK_ACCEL_CHANGED;

                    accel.RawX = raw_x;
                    accel.RawY = raw_y;
                    accel.RawZ = raw_z;

                    wiimote_state::nunchuk::calibration_info &calib =
                        m_state.Nunchuk.CalibrationInfo;
                    accel.X = ((float)raw_x - calib.X0) / ((float)calib.XG - calib.X0);
                    accel.Y = ((float)raw_y - calib.Y0) / ((float)calib.YG - calib.Y0);
                    accel.Z = ((float)raw_z - calib.Z0) / ((float)calib.ZG - calib.Z0);

                    // try to extract orientation from the accel:
					/*
                    if(EstimateOrientationFrom(accel))
                        changed |= NUNCHUK_ORIENTATION_CHANGED;
						*/
                }
                {
                    // joystick:
                    wiimote_state::joystick &joy = m_state.Nunchuk.Joystick;

                    float raw_x = buff[offset+0];
                    float raw_y = buff[offset+1];

                    if((raw_x != joy.RawX) || (raw_y != joy.RawY))
                        changed |= NUNCHUK_JOYSTICK_CHANGED;

                    joy.RawX = raw_x;
                    joy.RawY = raw_y;

                    // apply the calibration data
                    wiimote_state::nunchuk::calibration_info &calib =
                        m_state.Nunchuk.CalibrationInfo;
                    if(m_state.Nunchuk.CalibrationInfo.MaxX != 0x00)
                        joy.X = ((float)raw_x - calib.MidX) / ((float)calib.MaxX - calib.MinX);
                    if(calib.MaxY != 0x00)
                        joy.Y = ((float)raw_y - calib.MidY) / ((float)calib.MaxY - calib.MinY);

                    // i prefer the outputs to range -1 - +1 (note this also affects the
                    //  deadzone calculations)
                    joy.X *= 2;	joy.Y *= 2;

					/*
                    // apply the public deadzones to the internal state (if set)
                    joy.DeadZone = Nunchuk.Joystick.DeadZone;
                    ApplyJoystickDeadZones(joy);
					*/
                }
            }
            break;

        case wiimote_state::CLASSIC:
        case wiimote_state::GH3_GHWT_GUITAR:
        case wiimote_state::GHWT_DRUMS:
            {
                // buttons:
                WORD bits = *(WORD*)(buff+offset+4);
                bits = ~bits; // need to invert bits since 0 is down, and 1 is up

                if(bits != m_state.ClassicController.Button.Bits)
                    changed |= CLASSIC_BUTTONS_CHANGED;

                m_state.ClassicController.Button.Bits = bits;

                // joysticks:
                wiimote_state::joystick &joyL = m_state.ClassicController.JoystickL;
                wiimote_state::joystick &joyR = m_state.ClassicController.JoystickR;

                float l_raw_x = (float) (buff[offset+0] & 0x3f);
                float l_raw_y = (float) (buff[offset+1] & 0x3f);
                float r_raw_x = (float)((buff[offset+2]		    >> 7) |
                        ((buff[offset+1] & 0xc0) >> 5) |
                        ((buff[offset+0] & 0xc0) >> 3));
                float r_raw_y = (float) (buff[offset+2] & 0x1f);

                if((joyL.RawX != l_raw_x) || (joyL.RawY != l_raw_y))
                    changed |= CLASSIC_JOYSTICK_L_CHANGED;
                if((joyR.RawX != r_raw_x) || (joyR.RawY != r_raw_y))
                    changed |= CLASSIC_JOYSTICK_R_CHANGED;

                joyL.RawX = l_raw_x; joyL.RawY = l_raw_y;
                joyR.RawX = r_raw_x; joyR.RawY = r_raw_y;

                // apply calibration
                wiimote_state::classic_controller::calibration_info &calib =
                    m_state.ClassicController.CalibrationInfo;
                if(calib.MaxXL != 0x00)
                    joyL.X = (joyL.RawX - calib.MidXL) / ((float)calib.MaxXL - calib.MinXL);
                if(calib.MaxYL != 0x00)
                    joyL.Y = (joyL.RawY - calib.MidYL) / ((float)calib.MaxYL - calib.MinYL);
                if(calib.MaxXR != 0x00)
                    joyR.X = (joyR.RawX - calib.MidXR) / ((float)calib.MaxXR - calib.MinXR);
                if(calib.MaxYR != 0x00)
                    joyR.Y = (joyR.RawY - calib.MidYR) / ((float)calib.MaxYR - calib.MinYR);

                // i prefer the joystick outputs to range -1 - +1 (note this also affects
                //  the deadzone calculations)
                joyL.X *= 2; joyL.Y *= 2; joyR.X *= 2; joyR.Y *= 2;

				/*
                // apply the public deadzones to the internal state (if set)
                joyL.DeadZone = ClassicController.JoystickL.DeadZone;
                joyR.DeadZone = ClassicController.JoystickR.DeadZone;
                ApplyJoystickDeadZones(joyL);
                ApplyJoystickDeadZones(joyR);
				*/

                // triggers
                BYTE raw_trigger_l = ((buff[offset+2] & 0x60) >> 2) |
                    (buff[offset+3]		  >> 5);
                BYTE raw_trigger_r =   buff[offset+3] & 0x1f;

                if((raw_trigger_l != m_state.ClassicController.RawTriggerL) ||
                        (raw_trigger_r != m_state.ClassicController.RawTriggerR))
                    changed |= CLASSIC_TRIGGERS_CHANGED;

                m_state.ClassicController.RawTriggerL  = raw_trigger_l;
                m_state.ClassicController.RawTriggerR  = raw_trigger_r;

                if(calib.MaxTriggerL != 0x00)
                    m_state.ClassicController.TriggerL =
                        (float)m_state.ClassicController.RawTriggerL / 
                        ((float)calib.MaxTriggerL -	calib.MinTriggerL);
                if(calib.MaxTriggerR != 0x00)
                    m_state.ClassicController.TriggerR =
                        (float)m_state.ClassicController.RawTriggerR / 
                        ((float)calib.MaxTriggerR - calib.MinTriggerR);
            }
            break;

		case wiimote_state::BALANCE_BOARD:
            {
                wiimote_state::balance_board::sensors_raw prev_raw =
                    m_state.BalanceBoard.Raw;
                m_state.BalanceBoard.Raw.TopR	  =
                    (short)((short)buff[offset+0] << 8 | buff[offset+1]);
                m_state.BalanceBoard.Raw.BottomR =
                    (short)((short)buff[offset+2] << 8 | buff[offset+3]);
                m_state.BalanceBoard.Raw.TopL	  =
                    (short)((short)buff[offset+4] << 8 | buff[offset+5]);
                m_state.BalanceBoard.Raw.BottomL =
                    (short)((short)buff[offset+6] << 8 | buff[offset+7]);

                if((m_state.BalanceBoard.Raw.TopL    != prev_raw.TopL)    ||
                        (m_state.BalanceBoard.Raw.TopR    != prev_raw.TopR)    ||
                        (m_state.BalanceBoard.Raw.BottomL != prev_raw.BottomL) ||
                        (m_state.BalanceBoard.Raw.BottomR != prev_raw.BottomR))
                    changed |= BALANCE_WEIGHT_CHANGED;

				/*
                m_state.BalanceBoard.Kg.TopL	 =
                    GetBalanceValue(m_state.BalanceBoard.Raw.TopL,
                            m_state.BalanceBoard.CalibrationInfo.Kg0 .TopL,
                            m_state.BalanceBoard.CalibrationInfo.Kg17.TopL,
                            m_state.BalanceBoard.CalibrationInfo.Kg34.TopL);
                m_state.BalanceBoard.Kg.TopR	 =
                    GetBalanceValue(m_state.BalanceBoard.Raw.TopR,
                            m_state.BalanceBoard.CalibrationInfo.Kg0 .TopR,
                            m_state.BalanceBoard.CalibrationInfo.Kg17.TopR,
                            m_state.BalanceBoard.CalibrationInfo.Kg34.TopR);
                m_state.BalanceBoard.Kg.BottomL =
                    GetBalanceValue(m_state.BalanceBoard.Raw.BottomL,
                            m_state.BalanceBoard.CalibrationInfo.Kg0 .BottomL,
                            m_state.BalanceBoard.CalibrationInfo.Kg17.BottomL,
                            m_state.BalanceBoard.CalibrationInfo.Kg34.BottomL);
                m_state.BalanceBoard.Kg.BottomR =
                    GetBalanceValue(m_state.BalanceBoard.Raw.BottomR,
                            m_state.BalanceBoard.CalibrationInfo.Kg0 .BottomR,
                            m_state.BalanceBoard.CalibrationInfo.Kg17.BottomR,
                            m_state.BalanceBoard.CalibrationInfo.Kg34.BottomR);

                // uses these as the 'at rest' offsets? (immediately after Connect(),
                //  or if the app called CalibrateAtRest())
                if(bCalibrateAtRest) {
                    bCalibrateAtRest = false;
                    TRACE(_T(".. Auto-removing 'at rest' BBoard offsets."));
                    m_state.BalanceBoard.AtRestKg = m_state.BalanceBoard.Kg;
                }

                // remove the 'at rest' offsets
                m_state.BalanceBoard.Kg.TopL	 -= BalanceBoard.AtRestKg.TopL;
                m_state.BalanceBoard.Kg.TopR	 -= BalanceBoard.AtRestKg.TopR;
                m_state.BalanceBoard.Kg.BottomL -= BalanceBoard.AtRestKg.BottomL;
                m_state.BalanceBoard.Kg.BottomR -= BalanceBoard.AtRestKg.BottomR;

                // compute the average
                m_state.BalanceBoard.Kg.Total	  = m_state.BalanceBoard.Kg.TopL    +
                    m_state.BalanceBoard.Kg.TopR    +
                    m_state.BalanceBoard.Kg.BottomL +
                    m_state.BalanceBoard.Kg.BottomR;
                // and convert to Lbs
                const float KG2LB = 2.20462262f;
                m_state.BalanceBoard.Lb		  = m_state.BalanceBoard.Kg;
                m_state.BalanceBoard.Lb.TopL	 *= KG2LB;
                m_state.BalanceBoard.Lb.TopR	 *= KG2LB;
                m_state.BalanceBoard.Lb.BottomL *= KG2LB;
                m_state.BalanceBoard.Lb.BottomR *= KG2LB;
                m_state.BalanceBoard.Lb.Total	 *= KG2LB;
							*/
            }
            break;

		case wiimote_state::MOTION_PLUS:
            {
                bMotionPlusDetected = true;
				/*
                bMotionPlusEnabled  = true;
				*/

                short yaw   = ((unsigned short)buff[offset+3] & 0xFC)<<6 |
                    (unsigned short)buff[offset+0];
                short pitch = ((unsigned short)buff[offset+5] & 0xFC)<<6 |
                    (unsigned short)buff[offset+2];
                short roll  = ((unsigned short)buff[offset+4] & 0xFC)<<6 |
                    (unsigned short)buff[offset+1];

                // we get one set of bogus values when the MotionPlus is disconnected,
                //  so ignore them
                if((yaw != 0x3fff) || (pitch != 0x3fff) || (roll != 0x3fff))
                {
                    wiimote_state::motion_plus::sensors_raw &raw = m_state.MotionPlus.Raw;

                    if((raw.Yaw != yaw) || (raw.Pitch != pitch) || (raw.Roll  != roll))
                        changed |= MOTIONPLUS_SPEED_CHANGED;

                    raw.Yaw   = yaw;
                    raw.Pitch = pitch;
                    raw.Roll  = roll;

                    // convert to float values
                    bool    yaw_slow = (buff[offset+3] & 0x2) == 0x2;
                    bool  pitch_slow = (buff[offset+3] & 0x1) == 0x1;
                    bool   roll_slow = (buff[offset+4] & 0x2) == 0x2;
                    float y_scale    =   yaw_slow? 0.05f : 0.25f;
                    float p_scale    = pitch_slow? 0.05f : 0.25f;
                    float r_scale    =  roll_slow? 0.05f : 0.25f;

                    m_state.MotionPlus.Speed.Yaw   = -(raw.Yaw   - 0x1F7F) * y_scale;
                    m_state.MotionPlus.Speed.Pitch = -(raw.Pitch - 0x1F7F) * p_scale;
                    m_state.MotionPlus.Speed.Roll  = -(raw.Roll  - 0x1F7F) * r_scale;

                    // show if there's an extension plugged into the MotionPlus:
                    bool extension = buff[offset+4] & 1;
                    //if(extension != bMotionPlusExtension)
                    {
                        if(extension) {
                            //TRACE(_T(".. MotionPlus extension found."));
                            changed |= MOTIONPLUS_EXTENSION_CONNECTED;
                        }
                        else{
                            //TRACE(_T(".. MotionPlus' extension disconnected."));
                            changed |= MOTIONPLUS_EXTENSION_DISCONNECTED;
                        }
                    }
                    //bMotionPlusExtension = extension;
                }
                // while we're getting data, the plus is obviously detected/enabled
                //			bMotionPlusDetected = bMotionPlusEnabled = true;
            }
            break;
    }

    return changed;
}

void Wiimote::ParseButtons(const unsigned char* buff)
{
    unsigned short bits = *(unsigned short*)(buff+1) & m_state.Button.ALL;
    m_state.Button.Bits=bits;
}

void Wiimote::ParseAccel(const unsigned char* buff)
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

void Wiimote::onMemory(Device *device, const unsigned char *data)
{
    assert(m_currentTask);
    // do task
    m_currentTask->process(this, data);
    // done
    m_currentTask=0;

	dequeue(device);
}

}
