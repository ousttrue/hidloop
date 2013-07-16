#include <boost/asio.hpp>
#include <hidloop.h>
//#include <boost/asio/windows/stream_handle.hpp>

static bool match(unsigned short vendor_id, unsigned short product_id)
{
    return true;
}



static bool detectOculus(unsigned short vendor_id, unsigned short product_id)
{
    return vendor_id==0x2833 && product_id==0x0001;
}


#define SHOW_BUTTON(BUTTON) \
    #BUTTON << "=" << state.Button.BUTTON() \


static void showStatus(std::ostream &os, const wiimote_state &state)
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
        /*
        << ",X=" << (int)state.Acceleration.RawX
        << ",Y=" << (int)state.Acceleration.RawY
        << ",Z=" << (int)state.Acceleration.RawZ
        << ",X=" << state.Acceleration.X
        << ",Y=" << state.Acceleration.Y
        << ",Z=" << state.Acceleration.Z
        */
        << ",X=" << (int)state.MotionPlus.Raw.Yaw
        << ",Y=" << (int)state.MotionPlus.Raw.Pitch
        << ",Z=" << (int)state.MotionPlus.Raw.Roll
        << ",X=" << state.MotionPlus.Speed.Yaw
        << ",Y=" << state.MotionPlus.Speed.Pitch
        << ",Z=" << state.MotionPlus.Speed.Roll
        << std::endl
        ;
}


int main(int argc, char **argv)
{
    boost::asio::io_service io_service;

    hid::DeviceManager devMan;

	devMan.search(&hid::wiimote::Wiimote::detect);

    auto device=devMan.getDevice(0);
    if(!device){
       return 1; 
    }

	auto wii=std::make_shared<hid::wiimote::Wiimote>();
    device->open(io_service, wii);

    auto on_status=[](const wiimote_state &state){ 
        showStatus(std::cout, state); 
    };
    wii->setOnStatus(on_status);

    io_service.run();

    return 0;
}

