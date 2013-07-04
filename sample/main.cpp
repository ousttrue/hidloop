#include <hidloop.h>


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

    devMan.search(detectOculus);

    return 0;
}

