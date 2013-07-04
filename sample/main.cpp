#include <boost/asio.hpp>
#include <hidloop.h>
//#include <boost/asio/windows/stream_handle.hpp>

static bool match(unsigned short vendor_id, unsigned short product_id)
{
    return true;
}


static bool detectWiimote(unsigned short vendor_id, unsigned short product_id)
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


static bool detectOculus(unsigned short vendor_id, unsigned short product_id)
{
    return vendor_id==0x2833 && product_id==0x0001;
}


int main(int argc, char **argv)
{

    hid::DeviceManager devMan;
    //devMan.search(detectOculus);
	devMan.search(detectWiimote);

    auto device=devMan.getDevice(0);
    if(!device){
       return 1; 
    }

    boost::asio::io_service io_service;
    //boost::asio::io_service::work work(io_service);
    device->open(io_service);

    io_service.run();

    return 0;
}

