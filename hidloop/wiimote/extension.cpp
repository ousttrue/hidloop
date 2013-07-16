#include "extension.h"
#include "wiimote.h"


namespace hid {


class MotionPlusExtension: public IExtension
{
    void parse(Wiimote *wiimote, std::vector<unsigned char> &buff)override
    {
        short yaw   = ((unsigned short)buff[3] & 0xFC)<<6 |
            (unsigned short)buff[0];
        short pitch = ((unsigned short)buff[5] & 0xFC)<<6 |
            (unsigned short)buff[2];
        short roll  = ((unsigned short)buff[4] & 0xFC)<<6 |
            (unsigned short)buff[1];

        auto &state=wiimote->getState();

        // we get one set of bogus values when the MotionPlus is disconnected,
        //  so ignore them
        if((yaw != 0x3fff) || (pitch != 0x3fff) || (roll != 0x3fff))
        {
            wiimote_state::motion_plus::sensors_raw &raw = state.MotionPlus.Raw;

            /*
            if((raw.Yaw != yaw) || (raw.Pitch != pitch) || (raw.Roll  != roll))
                changed |= MOTIONPLUS_SPEED_CHANGED;
                */

            raw.Yaw   = yaw;
            raw.Pitch = pitch;
            raw.Roll  = roll;

            // convert to float values
            bool    yaw_slow = (buff[3] & 0x2) == 0x2;
            bool  pitch_slow = (buff[3] & 0x1) == 0x1;
            bool   roll_slow = (buff[4] & 0x2) == 0x2;
            float y_scale    =   yaw_slow? 0.05f : 0.25f;
            float p_scale    = pitch_slow? 0.05f : 0.25f;
            float r_scale    =  roll_slow? 0.05f : 0.25f;

            state.MotionPlus.Speed.Yaw   = -(raw.Yaw   - 0x1F7F) * y_scale;
            state.MotionPlus.Speed.Pitch = -(raw.Pitch - 0x1F7F) * p_scale;
            state.MotionPlus.Speed.Roll  = -(raw.Roll  - 0x1F7F) * r_scale;

            /*
            // show if there's an extension plugged into the MotionPlus:
            bool extension = buff[4] & 1;
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
            */
            //bMotionPlusExtension = extension;
        }
        // while we're getting data, the plus is obviously detected/enabled
        //			bMotionPlusDetected = bMotionPlusEnabled = true;
    }

    void calibration(Wiimote *wiimote, const unsigned char *buff)override
    {
    }
};


static const QWORD NUNCHUK		       = 0x000020A40000ULL;
static const QWORD CLASSIC		       = 0x010120A40000ULL;
static const QWORD GH3_GHWT_GUITAR     = 0x030120A40000ULL;
static const QWORD GHWT_DRUMS	       = 0x030120A40001ULL;
static const QWORD BALANCE_BOARD	   = 0x020420A40000ULL;
static const QWORD MOTION_PLUS		   = 0x050420A40000ULL;
static const QWORD MOTION_PLUS_INSIDE   = 0x050420A40001ULL;
static const QWORD MOTION_PLUS_DETECT  = 0x050020a60000ULL;
static const QWORD MOTION_PLUS_DETECT2 = 0x050420a60000ULL;
static const QWORD MOTION_PLUS_DETECT3 = 0x050020A40001ULL;
static const QWORD PARTIALLY_INSERTED  = 0xffffffffffffULL;


std::shared_ptr<IExtension> IExtension::Create(QWORD type)
{
    if(type==MOTION_PLUS){
        return std::make_shared<MotionPlusExtension>();
    }
    if(type==MOTION_PLUS_INSIDE){
        return std::make_shared<MotionPlusExtension>();
    }

    return 0;
}

}
