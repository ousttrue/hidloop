#pragma once
#include <memory>
#include <vector>

#ifndef QWORD
    typedef unsigned __int64 QWORD;
#endif


namespace hid {

class Wiimote;
class IExtension
{
public:
    virtual ~IExtension(){}
    virtual void parse(Wiimote* wiimote, std::vector<unsigned char> &buff)=0;
    virtual void calibration(Wiimote* wiimote, const unsigned char *buff)=0;

    static std::shared_ptr<IExtension> Create(QWORD type);
};

}
