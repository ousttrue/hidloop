#pragma once
#include "ionread.h"
#include <windows.h>
#include "wiimote_state.h"
#include <vector>

namespace hid {

class Wiimote: public IOnRead
{
    enum IN_TYPE
    {
        IN_BUTTONS				 = 0x30,
        IN_BUTTONS_ACCEL		 = 0x31,
        IN_BUTTONS_ACCEL_IR		 = 0x33,
        IN_BUTTONS_ACCEL_EXT	 = 0x35,
        IN_BUTTONS_ACCEL_IR_EXT	 = 0x37,
        IN_BUTTONS_BALANCE_BOARD = 0x32,
    };
    IN_TYPE m_inType;

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

    wiimote_state m_state;
    unsigned int m_lastTime;

public:
    static bool detect(unsigned short vendor_id, unsigned short product_id);
    Wiimote();
    ~Wiimote();
    void onRead(const unsigned char *data, size_t size)override;
    std::vector<unsigned char> createData_EnableAccel(){ 
        return createData_SetReportType(IN_BUTTONS_ACCEL, false); 
    }
    std::vector<unsigned char> createData_SetReportType(IN_TYPE type, bool continuous);

private:
    void ParseAccel(const unsigned char* buff);
};

}
