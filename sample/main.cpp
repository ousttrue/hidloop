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


int main(int argc, char **argv)
{
    hid::DeviceManager devMan;

	devMan.search(&hid::Wiimote::detect);

    auto device=devMan.getDevice(0);
    if(!device){
       return 1; 
    }

    boost::asio::io_service io_service;

	auto wii=std::make_shared<hid::Wiimote>();
    device->open(io_service, wii);

    auto data=wii->createData_EnableAccel();
    device->write(data);

    io_service.run();

    return 0;
}

