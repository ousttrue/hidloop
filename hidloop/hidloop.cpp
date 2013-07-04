#include "hidloop.h"
#include <windows.h>

#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")

#define GET_PROC(name, type){ \
    name = (type*)GetProcAddress(hModule, #name); \
    if(!name){ \
        return false; \
    } \
} \

#include <vector>


namespace {

    typedef struct _HIDD_ATTRIBUTES {
        ULONG   Size;            // = sizeof (struct _HIDD_ATTRIBUTES)
        USHORT  VendorID;
        USHORT  ProductID;
        USHORT  VersionNumber;
    } HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

    typedef BOOL __stdcall HIDD_GETHIDGUID(GUID *pGuid);
    HIDD_GETHIDGUID *HidD_GetHidGuid;

    typedef BOOL __stdcall HIDD_GETATTRIBUTES(HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attributes);
    HIDD_GETATTRIBUTES *HidD_GetAttributes=0;
}


namespace hid {

bool initialize()
{
    HMODULE hModule= LoadLibrary( "HID.DLL" );
    GET_PROC(HidD_GetHidGuid, HIDD_GETHIDGUID);
    GET_PROC(HidD_GetAttributes, HIDD_GETATTRIBUTES);

    return true;
}


static void processHandle(std::list<Device> &list, DetectDevice detect, HANDLE handle)
{
    // get the device attributes
    HIDD_ATTRIBUTES attrib;
    attrib.Size = sizeof(attrib);
    if(!HidD_GetAttributes(handle, &attrib))
    {
        return;
    }

    if(!detect(attrib.VendorID, attrib.ProductID)){
        return;
    }

    list.push_back(Device(attrib.VendorID, attrib.ProductID));
}


void search(std::list<Device> &list, DetectDevice detect)
{
    GUID guid;
    if(!HidD_GetHidGuid(&guid)){
        return;
    }

    auto hDevInfo=SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_INTERFACEDEVICE | DIGCF_PRESENT);
    if (hDevInfo==INVALID_HANDLE_VALUE){
        return;
    }

    SP_DEVICE_INTERFACE_DATA data;
	data.cbSize=sizeof(data);
    std::vector<BYTE> buf;
    for(DWORD i=0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, i, &data); ++i)
    {
        DWORD size=0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &data, NULL, 0, &size, NULL);
        if(size==0){
            // error ?
            continue;
        }

        buf.resize(size);
        auto detail = (SP_DEVICE_INTERFACE_DETAIL_DATA*) &buf[0];
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if(!SetupDiGetDeviceInterfaceDetail(hDevInfo, &data, detail, size, &size, NULL)){
            return;
        }

        auto handle = CreateFile(detail->DevicePath, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
		if(handle == INVALID_HANDLE_VALUE) {
            return;
        }
        processHandle(list, detect, handle);
        CloseHandle(handle);
    }
}


}
