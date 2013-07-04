#include "wiimote.h"
#include <iostream>


static const int REPORT_LENGTH=22;


#define SHOW_BUTTON(BUTTON) \
    #BUTTON << "=" << state.Button.BUTTON() \



static void showStatus(std::ostream &os, wiimote_state &state)
{
    os
        /*
        << SHOW_BUTTON(A)
        << "," << SHOW_BUTTON(B)
        << "," << SHOW_BUTTON(Plus)
        << "," << SHOW_BUTTON(Home)
        << "," << SHOW_BUTTON(Minus)
        << "," << SHOW_BUTTON(One)
        << "," << SHOW_BUTTON(Two)
        << "," << SHOW_BUTTON(Up)
        << "," << SHOW_BUTTON(Down)
        << "," << SHOW_BUTTON(Left)
        << "," << SHOW_BUTTON(Right)
        */
        << ",X=" << (int)state.Acceleration.RawX
        << ",Y=" << (int)state.Acceleration.RawY
        << ",Z=" << (int)state.Acceleration.RawZ
        << std::endl
        ;
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
                //std::cout << "IN_BUTTONS" << std::endl;
                unsigned short bits = *(unsigned short*)(data+1) & m_state.Button.ALL;
                m_state.Button.Bits=bits;
                showStatus(std::cout, m_state);
            }
            break;

        case IN_BUTTONS_ACCEL:
            {
                //std::cout << "IN_BUTTONS_ACCEL: " << d << std::endl;
                unsigned short bits = *(unsigned short*)(data+1) & m_state.Button.ALL;
                m_state.Button.Bits=bits;
                ParseAccel(data);
                showStatus(std::cout, m_state);
            }
            break;

        default:
            std::cout << "unknown data: " << type << std::endl;
            break;
    }
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
    //if(m_state.CalibrationInfo.X0)
    if(true)
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
