#include "devicemanager.h"
#include "device.h"

#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")

#define GET_PROC(name, type){ \
    name = (type*)GetProcAddress(hModule, #name); \
    if(!name){ \
        return false; \
    } \
} \

#include <vector>
#include <list>


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


static std::shared_ptr<Device> processHandle(DetectDevice detect, HANDLE handle)
{
    HIDD_ATTRIBUTES attrib;
    attrib.Size = sizeof(attrib);
    if(!HidD_GetAttributes(handle, &attrib))
    {
        return 0;
    }

    if(!detect(attrib.VendorID, attrib.ProductID)){
        return 0;
    }

    return std::make_shared<Device>(attrib.VendorID, attrib.ProductID);
}


class DeviceManagerImpl
{
    std::vector<std::shared_ptr<Device>> m_devices;

public:
    DeviceManagerImpl()
    {
        initialize();
    }

    ~DeviceManagerImpl()
    {
        // FreeLibrary();
    }

    std::shared_ptr<Device> getDevice(size_t index)
    {
        if(index<m_devices.size()){
            return m_devices[index];
        }
        return 0;
    }

    void search(DetectDevice detect)
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
            auto device=processHandle(detect, handle);
            if(device){
                device->setPath(detail->DevicePath);
                m_devices.push_back(device);
            }
            CloseHandle(handle);
        }
    }

private:
    bool initialize()
    {
        HMODULE hModule= LoadLibrary( "HID.DLL" );
        GET_PROC(HidD_GetHidGuid, HIDD_GETHIDGUID);
        GET_PROC(HidD_GetAttributes, HIDD_GETATTRIBUTES);

		return true;
    }
};


//////////////////////////////////////////////////////////////////////////////
// DeviceManager
//////////////////////////////////////////////////////////////////////////////
DeviceManager::DeviceManager()
    : m_impl(new DeviceManagerImpl)
{
}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::search(DetectDevice detect)
{
    m_impl->search(detect);
}

std::shared_ptr<Device> DeviceManager::getDevice(size_t index)
{
    return m_impl->getDevice(index);
}

}
