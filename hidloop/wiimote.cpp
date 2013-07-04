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
    IN_BUTTONS				 = 0x30,
    IN_BUTTONS_ACCEL		 = 0x31,
    IN_BUTTONS_ACCEL_IR		 = 0x33,
    IN_BUTTONS_ACCEL_EXT	 = 0x35,
    IN_BUTTONS_ACCEL_IR_EXT	 = 0x37,
    IN_BUTTONS_BALANCE_BOARD = 0x32,
};
// input reports used only internally:
static const int IN_STATUS						= 0x20;
static const int IN_READADDRESS					= 0x21;

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


static void send_SetReportType(std::shared_ptr<hid::Device> device, IN_TYPE type, bool continuous=false)
{
    std::vector<unsigned char> buff(REPORT_LENGTH, 0);
    buff[0] = (BYTE)OUT_TYPE;
    buff[1] = (continuous ? 0x04 : 0x00); //| GetRumbleBit();
    buff[2] = (BYTE)type;
    device->write(buff);
}

static void send_ReadAddress(std::shared_ptr<hid::Device> device, int address, short size)
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
    : m_lastTime(0)
{
}

Wiimote::~Wiimote()
{
}

void Wiimote::onConnect(std::shared_ptr<Device> device)
{
    // calibration
    send_ReadAddress(device, REGISTER_CALIBRATION, 7);
    // accel
    send_SetReportType(device, IN_BUTTONS_ACCEL, false); 
}

void Wiimote::onRead(std::shared_ptr<Device> device, const unsigned char *data, size_t size)
{
    auto now=timeGetTime();
    auto d=now-m_lastTime;
    m_lastTime=now;

    IN_TYPE type=static_cast<IN_TYPE>(data[0]);
    switch(type)
    {
        case IN_BUTTONS:
            ParseButtons(data);
            break;

        case IN_BUTTONS_ACCEL:
            ParseButtons(data);
            ParseAccel(data);
            break;

        case IN_READADDRESS:
            ParseButtons(data);
            ParseReadAddress(device, data);
            break;

        default:
            std::cout << "unknown data: " << type << std::endl;
            break;
    }

    if(m_onStatus){
        m_onStatus(m_state);
    }
}

void Wiimote::onDestroy(std::shared_ptr<Device> device)
{
    send_SetReportType(device, IN_BUTTONS, false);
}

int Wiimote::ParseReadAddress(std::shared_ptr<Device> device, const unsigned char* buff)
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
        /*
        // this also happens when attempting to detect a non-existant MotionPlus
        if(MotionPlusDetectCount)
        {
            --MotionPlusDetectCount;
            if(m_state.ExtensionType == MOTION_PLUS)
            {
                if(bMotionPlusDetected)
                    TRACE(_T(".. MotionPlus removed."));
                bMotionPlusDetected  = false;
                bMotionPlusEnabled   = false;
                // the MotionPlus can sometimes get confused - initializing
                //  extenions fixes it:
                //				if(address == 0xfa)
                //					InitializeExtension();
            }
        }
        else
			WARN(_T("error: attempt to read from write-only register 0x%X."), buff[3]);
        */
        return NO_CHANGE;
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
                _ASSERT(size == 5);
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
                    bMotionPlusDetected = true;
                    --MotionPlusDetectCount;
					*/
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
                    send_ReadAddress(device, REGISTER_EXTENSION_CALIBRATION, 16);
                    //bMotionPlusDetected = true;
                }
                else IF_TYPE(NUNCHUK)
                    //TRACE(_T(".. Nunchuk!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    send_ReadAddress(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(CLASSIC)
                    //TRACE(_T(".. Classic Controller!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    send_ReadAddress(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(GH3_GHWT_GUITAR)
                    // sometimes it comes in more than once?
                    //TRACE(_T(".. GH3/GHWT Guitar Controller!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    send_ReadAddress(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(GHWT_DRUMS)
                    //TRACE(_T(".. GHWT Drums!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    send_ReadAddress(device, REGISTER_EXTENSION_CALIBRATION, 16);
                }
                else IF_TYPE(BALANCE_BOARD)
                    //TRACE(_T(".. Balance Board!"));
                    //bMotionPlusEnabled = false;
                    // and start a query for the calibration data
                    send_ReadAddress(device, REGISTER_BALANCE_CALIBRATION, 24);
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

}
