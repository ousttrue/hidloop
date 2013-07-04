#include <hidloop.h>


static bool match(unsigned vendor_id, unsigned product_id)
{
    return true;
}


int main(int argc, char **argv)
{
    if(!hid::initialize()){
        return 1;
    }

    std::list<hid::Device> list;
    hid::search(list, match);
    if(list.empty()){
        return 2;
    }

    return 0;
}

