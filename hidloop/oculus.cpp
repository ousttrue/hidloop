#include "oculus.h"
#include <iostream>


namespace hid { namespace oculus {

bool detect(unsigned short vendor_id, unsigned short product_id)
{
    return vendor_id==0x2833 && product_id==0x0001;
}


void Oculus::onConnect(Device *device)
{
	std::cout << __FUNCTION__ << std::endl;
}

void Oculus::onRead(Device *device, std::vector<unsigned char> &data)
{
	std::cout << __FUNCTION__ << std::endl;
}

void Oculus::onDestroy(Device *device)
{
	std::cout << __FUNCTION__ << std::endl;
}

}}