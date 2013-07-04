#pragma once
#include "ionread.h"
#include <windows.h>
#include "wiimote_state.h"

namespace hid {

class Wiimote: public IOnRead
{
    // wiimote data input mode (use with SetReportType())
    //  (only enable what you need to save battery power)
    enum IN_TYPE
    {
        // combinations if buttons/acceleration/IR/Extension data
        IN_BUTTONS				 = 0x30,
        IN_BUTTONS_ACCEL		 = 0x31,
        IN_BUTTONS_ACCEL_IR		 = 0x33, // reports IR EXTENDED data (dot sizes)
        IN_BUTTONS_ACCEL_EXT	 = 0x35,
        IN_BUTTONS_ACCEL_IR_EXT	 = 0x37, // reports IR BASIC data (no dot sizes)
        IN_BUTTONS_BALANCE_BOARD = 0x32, // must use this for the balance board
    };

    wiimote_state m_state;

public:
    static bool detect(unsigned short vendor_id, unsigned short product_id);
    void onRead(const char *data, size_t size);

private:
    void showButtonStatus();
};

}
