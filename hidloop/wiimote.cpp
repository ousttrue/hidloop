#include "wiimote.h"
#include <iostream>


static const int REPORT_LENGTH=22;


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
    : m_inType(IN_BUTTONS), m_lastTime(0)
{
}

Wiimote::~Wiimote()
{
}

std::vector<unsigned char> Wiimote::createData_SetReportType(
        Wiimote::IN_TYPE type, bool continuous)
{
    /*
    _ASSERT(IsConnected());
    if(!IsConnected())
        return;
        */
    m_inType=type;

    /*
    switch(type)
    {
        case IN_BUTTONS_ACCEL_IR:
            EnableIR(wiimote_state::ir::EXTENDED);
            break;
        case IN_BUTTONS_ACCEL_IR_EXT:
            EnableIR(wiimote_state::ir::BASIC);
            break;
        default:
            DisableIR();
            break;
    }
    */

    std::vector<unsigned char> buff(REPORT_LENGTH, 0);
    buff[0] = (BYTE)OUT_TYPE;
    buff[1] = (continuous ? 0x04 : 0x00); //| GetRumbleBit();
    buff[2] = (BYTE)type;
    return  buff;
}

void Wiimote::onRead(const unsigned char *data, size_t size)
{
    auto now=timeGetTime();
    auto d=now-m_lastTime;
    m_lastTime=now;

    IN_TYPE type=static_cast<IN_TYPE>(data[0]);
    switch(type)
    {
        case IN_BUTTONS:
            {
                std::cout << "IN_BUTTONS" << std::endl;
                unsigned short bits = *(unsigned short*)(data+1) & m_state.Button.ALL;
                /*
                if(m_state.Button.Bits!=bits){
                    // button status changed
                }
                */
                m_state.Button.Bits=bits;
                showButtonStatus();
            }
            break;

        case IN_BUTTONS_ACCEL:
            {
                std::cout << "IN_BUTTONS_ACCEL: " << d << std::endl;
            }
            break;

        default:
            std::cout << "unknown data: " << type << std::endl;
            break;
    }
}

#define SHOW(BUTTON) \
    #BUTTON << "=" << m_state.Button.BUTTON() \

void Wiimote::showButtonStatus()
{
    std::cout
        << SHOW(A)
        << ", " << SHOW(B)
        << ", " << SHOW(Plus)
        << ", " << SHOW(Home)
        << ", " << SHOW(Minus)
        << ", " << SHOW(One)
        << ", " << SHOW(Two)
        << ", " << SHOW(Up)
        << ", " << SHOW(Down)
        << ", " << SHOW(Left)
        << ", " << SHOW(Right)
        << std::endl
        ;
}

}
