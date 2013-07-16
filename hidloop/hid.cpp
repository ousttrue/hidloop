#define EXTERN_HID_IMPL
#include "hid.h"

#define GET_PROC(name, type){ \
    name = (type*)GetProcAddress(hModule, #name); \
    if(!name){ \
        return false; \
    } \
} \


bool load_hid_funcs()
{
    HMODULE hModule= LoadLibrary( "HID.DLL" );
    GET_PROC(HidD_GetHidGuid, HIDD_GETHIDGUID);
    GET_PROC(HidD_GetAttributes, HIDD_GETATTRIBUTES);
	GET_PROC(HidD_GetPreparsedData, HIDD_GETPREPARSEDDATA);
    GET_PROC(HidD_FreePreparsedData, HIDD_FREEPREPARSEDDATA);
    GET_PROC(HidP_GetCaps, HIDP_GETCAPS);
}

