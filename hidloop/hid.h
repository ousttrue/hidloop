#pragma once

#include <windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")


#ifdef EXTERN_HID_IMPL
#define EXTERN_HID
#else
#define EXTERN_HID extern
#endif


typedef struct _HIDD_ATTRIBUTES {
    ULONG   Size;            // = sizeof (struct _HIDD_ATTRIBUTES)
    USHORT  VendorID;
    USHORT  ProductID;
    USHORT  VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

typedef BOOL __stdcall HIDD_GETHIDGUID(GUID *pGuid);
EXTERN_HID HIDD_GETHIDGUID *HidD_GetHidGuid;

typedef BOOL __stdcall HIDD_GETATTRIBUTES(HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attributes);
EXTERN_HID HIDD_GETATTRIBUTES *HidD_GetAttributes;

typedef struct _HIDP_PREPARSED_DATA HIDP_PREPARSED_DATA, * PHIDP_PREPARSED_DATA;
typedef BOOL __stdcall HIDD_GETPREPARSEDDATA(HANDLE HidDeviceObject, PHIDP_PREPARSED_DATA *PreparsedData);
EXTERN_HID HIDD_GETPREPARSEDDATA *HidD_GetPreparsedData;

typedef BOOL __stdcall HIDD_FREEPREPARSEDDATA(PHIDP_PREPARSED_DATA PreparsedData);
EXTERN_HID HIDD_FREEPREPARSEDDATA *HidD_FreePreparsedData;

#ifndef FACILITY_HID_ERROR_CODE
#define FACILITY_HID_ERROR_CODE 0x11
#endif

#ifndef HIDP_ERROR_CODES
#define HIDP_ERROR_CODES(SEV, CODE) ((NTSTATUS) (((SEV) << 28) | (FACILITY_HID_ERROR_CODE << 16) | (CODE)))
#endif

#define HIDP_STATUS_SUCCESS                  (HIDP_ERROR_CODES(0x0,0))

typedef USHORT USAGE, *PUSAGE;

typedef struct _HIDP_CAPS
{
    USAGE    Usage;
    USAGE    UsagePage;
    USHORT   InputReportByteLength;
    USHORT   OutputReportByteLength;
    USHORT   FeatureReportByteLength;
    USHORT   Reserved[17];
    USHORT   NumberLinkCollectionNodes;
    USHORT   NumberInputButtonCaps;
    USHORT   NumberInputValueCaps;
    USHORT   NumberInputDataIndices;
    USHORT   NumberOutputButtonCaps;
    USHORT   NumberOutputValueCaps;
    USHORT   NumberOutputDataIndices;
    USHORT   NumberFeatureButtonCaps;
    USHORT   NumberFeatureValueCaps;
    USHORT   NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;

typedef long NTSTATUS;
typedef NTSTATUS __stdcall HIDP_GETCAPS(PHIDP_PREPARSED_DATA PreparsedData, PHIDP_CAPS Capabilities);
EXTERN_HID HIDP_GETCAPS *HidP_GetCaps;

bool load_hid_funcs();

