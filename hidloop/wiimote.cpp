#include "wiimote.h"
#include <iostream>


namespace hid {

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

void Wiimote::onRead(const char *data, size_t size)
{
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
