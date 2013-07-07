#include <boost/asio.hpp>
#include "wiimote.h"
#include "device.h"
#include <iostream>
#include <mmsystem.h>


#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif


enum IN_TYPE
{
    IN_STATUS = 0x20,
    IN_READADDRESS = 0x21,
    IN_ACK = 0x22,
    IN_BUTTONS				 = 0x30,
    IN_BUTTONS_ACCEL		 = 0x31,
    IN_BUTTONS_ACCEL_IR		 = 0x33,
    IN_BUTTONS_ACCEL_EXT	 = 0x35,
    IN_BUTTONS_ACCEL_IR_EXT	 = 0x37,
    IN_BUTTONS_BALANCE_BOARD = 0x32,
};

enum OUT_TYPE
{
    OUT_NONE			= 0x00,
    OUT_LEDs			= 0x11,
    OUT_TYPE			= 0x12,
    OUT_IR				= 0x13,
    OUT_SPEAKER_ENABLE	= 0x14,
    OUT_STATUS			= 0x15,
    OUT_WRITEMEMORY		= 0x16,
    OUT_READMEMORY		= 0x17,
    OUT_SPEAKER_DATA	= 0x18,
    OUT_SPEAKER_MUTE	= 0x19,
    OUT_IR2				= 0x1a,
};

static const int REPORT_LENGTH=22;

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


static void send_SetReportType(hid::Device *device, IN_TYPE type, bool continuous=false)
{
    std::vector<unsigned char> buff(REPORT_LENGTH, 0);
    buff[0] = (BYTE)OUT_TYPE;
    buff[1] = (continuous ? 0x04 : 0x00); //| GetRumbleBit();
    buff[2] = (BYTE)type;
    device->write(buff);
}

static void send_ReadAddress(hid::Device *device, int address, short size)
{
    // asynchronous
    std::vector<unsigned char> buff(REPORT_LENGTH, 0);
    buff[0] = OUT_READMEMORY;
    buff[1] = (BYTE)(((address & 0xff000000) >> 24)); // | GetRumbleBit());
    buff[2] = (BYTE)( (address & 0x00ff0000) >> 16);
    buff[3] = (BYTE)( (address & 0x0000ff00) >>  8);
    buff[4] = (BYTE)( (address & 0x000000ff));
    buff[5] = (BYTE)( (size	   & 0xff00	   ) >>  8);
    buff[6] = (BYTE)( (size	   & 0xff));
    device->write(buff);
}

static void send_WriteAddress(hid::Device *device, int address, BYTE size, 
        const BYTE* buff)
{
    // asynchronous
    std::vector<unsigned char> write(REPORT_LENGTH, 0);
    write[0] = OUT_WRITEMEMORY;
    write[1] = (BYTE)(((address & 0xff000000) >> 24)); // | GetRumbleBit());
    write[2] = (BYTE)( (address & 0x00ff0000) >> 16);
    write[3] = (BYTE)( (address & 0x0000ff00) >>  8);
    write[4] = (BYTE)( (address & 0x000000ff));
    write[5] = size;
    memcpy(&write[6], buff, size);
    device->write(write);
}

static void send_WriteAddress(hid::Device *device, int address, BYTE data)
{ 
    // write a single BYTE to a wiimote address or register
    send_WriteAddress(device, address, 1, &data); 
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
    : m_lastTime(0), bMotionPlusDetected(false), m_currentMemory(MEMORY_NONE)
{
}

Wiimote::~Wiimote()
{
}

void Wiimote::initializeExtension(hid::Device *device)
{
    // motion plus
    //send_ReadAddress(device, REGISTER_MOTIONPLUS_DETECT, 6);
    //send_WriteAddress(device, REGISTER_MOTIONPLUS_INIT  , 0x55);
    // Enable it (this maps it to the standard extension port):
    //send_WriteAddress(device, REGISTER_MOTIONPLUS_ENABLE, 0x04);

    // wibrew.org: The new way to initialize the extension is by writing 0x55 to
    //	0x(4)A400F0, then writing 0x00 to 0x(4)A400FB. It works on all extensions, and
    //  makes the extension type bytes unencrypted. This means that you no longer have
    //  to decrypt the extension bytes using the transform listed above. 
    //bInitInProgress = true;
    // only initialize if it's not a MotionPlus
    //if(!bEnablingMotionPlus) {
    writeMemory(device, REGISTER_EXTENSION_INIT1, 0x55);
    writeMemory(device, REGISTER_EXTENSION_INIT2, 0x00);
    //}
    //else
    //    bEnablingMotionPlus = false;
    //}
    readMemory(device, REGISTER_EXTENSION_TYPE , 6);
}

void Wiimote::onConnect(Device *device)
{
    // reset
	//send_SetReportType(device, IN_BUTTONS, false);

    //send_RequestStatusReport(device);

    // calibration
    readMemory(device, REGISTER_CALIBRATION, 7);

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
            ParseReadAddress(device, data);
            onReadMemory(device);
            break;

        case IN_ACK:
            onWriteMemory(device);
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

int Wiimote::ParseReadAddress(Device *device, const unsigned char* buff)
{
    // decode the address that was queried:
    int address = buff[4]<<8 | buff[5];
    int size    = buff[3] >> 4;
    int changed	= 0;

    if((buff[3] & 0x08) != 0) {
        //WARN(_T("error: read address not valid."));
        //_ASSERT(0);
        return NO_CHANGE;
    }
    // address read failed (write-only)?
    else if((buff[3] & 0x07) != 0)
    {
        // this also happens when attempting to detect a non-existant MotionPlus
        //if(MotionPlusDetectCount)
        {
            //--MotionPlusDetectCount;
            if(m_state.ExtensionType == wiimote_state::MOTION_PLUS)
            {
                /*
                if(bMotionPlusDetected)
                    TRACE(_T(".. MotionPlus removed."));
                    */
                bMotionPlusDetected  = false;
                //bMotionPlusEnabled   = false;
                // the MotionPlus can sometimes get confused - initializing
                //  extenions fixes it:
                //				if(address == 0xfa)
                //					InitializeExtension();
            }
        }
        //else
			//WARN(_T("error: attempt to read from write-only register 0x%X."), buff[3]);
        //return NO_CHANGE;
    }

	// *NOTE*: this is a major (but convenient) hack!  The returned data only
	//          contains the lower two bytes of the address that was queried.
	//			as these don't collide between any of the addresses/registers
	//			we currently read, it's OK to match just those two bytes

	// skip the header
	buff += 6;

	switch(address)
    {
        case (REGISTER_CALIBRATION & 0xffff):
            {
                _ASSERT(size == 6);
                //TRACE(_T(".. got wiimote calibration."));
                m_state.CalibrationInfo.X0 = buff[0];
                m_state.CalibrationInfo.Y0 = buff[1];
                m_state.CalibrationInfo.Z0 = buff[2];
                m_state.CalibrationInfo.XG = buff[4];
                m_state.CalibrationInfo.YG = buff[5];
                m_state.CalibrationInfo.ZG = buff[6];
                //changed |= CALIBRATION_CHANGED;	
            }
            break;
			
            // note: this covers both the normal extension and motion plus extension
            //        addresses (0x4a400fa / 0x4a600fa)
        case (REGISTER_EXTENSION_TYPE & 0xffff):
            {
                //_ASSERT(size == 5);
                _ASSERT(size >= 5);
                QWORD type = *(QWORD*)buff;

                //			TRACE(_T("(found extension 0x%I64x)"), type);

                static const QWORD NUNCHUK		       = 0x000020A40000ULL;
                static const QWORD CLASSIC		       = 0x010120A40000ULL;
                static const QWORD GH3_GHWT_GUITAR     = 0x030120A40000ULL;
                static const QWORD GHWT_DRUMS	       = 0x030120A40001ULL;
                static const QWORD BALANCE_BOARD	   = 0x020420A40000ULL;
                static const QWORD MOTION_PLUS		   = 0x050420A40000ULL;
                static const QWORD MOTION_PLUS_DETECT  = 0x050020a60000ULL;
                static const QWORD MOTION_PLUS_DETECT2 = 0x050420a60000ULL;
                static const QWORD MOTION_PLUS_DETECT3 = 0x050020A40001ULL;
                static const QWORD PARTIALLY_INSERTED  = 0xffffffffffffULL;
                // MotionPlus: _before_ it's been activated
                if((type == MOTION_PLUS_DETECT) 
                        || (type == MOTION_PLUS_DETECT2)
                        || (type == MOTION_PLUS_DETECT3)
                  )
                {
					/*
                    if(!bMotionPlusDetected) {
                        TRACE(_T("Motion Plus detected!"));
                        changed |= MOTIONPLUS_DETECTED;
                    }
                    */
                    bMotionPlusDetected = true;
                    //--MotionPlusDetectCount;
                    break;
                }

#define IF_TYPE(id)	if(type == id) { \
    /* sometimes it comes in more than once */ \
    if(m_state.ExtensionType == wiimote_state::id)\
    break; \
    m_state.ExtensionType = wiimote_state::id;

                // MotionPlus: once it's activated & mapped to the standard ext. port
                IF_TYPE(MOTION_PLUS)
                    //TRACE(_T(".. Motion Plus!"));
                    // and start a query for the calibration data
                    readMemory(device, REGISTER_EXTENSION_CALIBRATION, 16);
                    bMotionPlusDetected = true;
                }
                else IF_TYPE(NUNCHUK)
                    //TRACE(_T(".. Nunchuk!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    readMemory(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(CLASSIC)
                    //TRACE(_T(".. Classic Controller!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    readMemory(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(GH3_GHWT_GUITAR)
                    // sometimes it comes in more than once?
                    //TRACE(_T(".. GH3/GHWT Guitar Controller!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    readMemory(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(GHWT_DRUMS)
                    //TRACE(_T(".. GHWT Drums!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    readMemory(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(BALANCE_BOARD)
                    //TRACE(_T(".. Balance Board!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    readMemory(device, REGISTER_BALANCE_CALIBRATION, 24);
                }
                else if(type == PARTIALLY_INSERTED) {
                    // sometimes it comes in more than once?
                    if(m_state.ExtensionType == wiimote_state::PARTIALLY_INSERTED)
                        Sleep(50);
                    //TRACE(_T(".. partially inserted!"));
                    //bMotionPlusEnabled = false;
                    m_state.ExtensionType = wiimote_state::PARTIALLY_INSERTED;
                    //changed |= EXTENSION_PARTIALLY_INSERTED;
                    // try initializing the extension again by requesting another
                    //  status report (this usually fixes it)
                    m_state.bExtension = false;
                    //RequestStatusReport();
                }
                else{
                    //TRACE(_T("unknown extension controller found (0x%I64x)"), type);
                }
            }
            break;
		
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
void Wiimote::writeMemory(Device *device, int addresses, unsigned char data)
{
    m_memoryQueue.push_back(MemoryData(MEMORY_WRITE, addresses, data));
    dequeue(device);
}

void Wiimote::onWriteMemory(Device *device)
{
    assert(m_currentMemory==MEMORY_WRITE);
    m_currentMemory=MEMORY_NONE;
    dequeue(device);
}

void Wiimote::readMemory(Device *device, int addresses, unsigned char data)
{
    m_memoryQueue.push_back(MemoryData(MEMORY_READ, addresses, data));
    dequeue(device);
}

void Wiimote::onReadMemory(Device *device)
{
    assert(m_currentMemory==MEMORY_READ);
    m_currentMemory=MEMORY_NONE;
    dequeue(device);
}

void Wiimote::dequeue(Device *device)
{
	if(m_memoryQueue.empty()){
		return;
	}
    if(m_currentMemory!=MEMORY_NONE){
        return;
    }
    auto data=m_memoryQueue.front();
    m_memoryQueue.pop_front();
std::cout << "memory " << m_memoryQueue.size() << std::endl;
    m_currentMemory=data.type;
    if(data.type==MEMORY_WRITE){
        send_WriteAddress(device, data.address, data.data);
    }
    else{
        send_ReadAddress(device, data.address, data.data);
    }
}


}
