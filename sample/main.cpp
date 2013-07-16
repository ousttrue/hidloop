#include <boost/asio.hpp>
#include <hidloop.h>


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

	devMan.search(&hid::oculus::detect);

    auto device=devMan.getDevice(0);
    if(!device){
       return 1; 
    }

	auto callback=std::make_shared<hid::oculus::Oculus>();
    device->open(io_service, callback);

    io_service.run();

    return 0;
}

