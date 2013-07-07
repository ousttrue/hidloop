#pragma once

enum IN_TYPE
{
    IN_STATUS = 0x20,
    IN_READADDRESS = 0x21,
    IN_ACK = 0x22,
    IN_BUTTONS				 = 0x30,
    IN_BUTTONS_ACCEL		 = 0x31,
    IN_BUTTONS_ACCEL_IR		 = 0x33,
    IN_BUTTONS_ACCEL_EXT	 = 0x35,
    IN_BUTTONS_ACCEL_IR_EXT	 = 0x37,
    IN_BUTTONS_BALANCE_BOARD = 0x32,
};

enum OUT_TYPE
{
    OUT_NONE			= 0x00,
    OUT_LEDs			= 0x11,
    OUT_TYPE			= 0x12,
    OUT_IR				= 0x13,
    OUT_SPEAKER_ENABLE	= 0x14,
    OUT_STATUS			= 0x15,
    OUT_WRITEMEMORY		= 0x16,
    OUT_READMEMORY		= 0x17,
    OUT_SPEAKER_DATA	= 0x18,
    OUT_SPEAKER_MUTE	= 0x19,
    OUT_IR2				= 0x1a,
};

enum {
    REPORT_LENGTH=22
};

