#pragma once
#include "icallback.h"
#include "wiimote_state.h"
#include <vector>
#include <functional>
#include <list>

namespace hid {

class Wiimote: public ICallback
{
    wiimote_state m_state;
    unsigned int m_lastTime;
    std::function<void(const wiimote_state &)> m_onStatus;
    bool bMotionPlusDetected;

public:
    static bool detect(unsigned short vendor_id, unsigned short product_id);
    Wiimote();
    ~Wiimote();
    const wiimote_state &getState()const{ return m_state; }
    void onConnect(Device *device)override;
    void onRead(Device *device, const unsigned char *data, size_t size)override;
    void onDestroy(Device *device)override;
    void setOnStatus(std::function<void(const wiimote_state &)> onStatus){ m_onStatus=onStatus; }

private:
    int ParseStatus(Device *device, const unsigned char *buff);
    int ParseExtension(const unsigned char *buff, unsigned offset);
    int ParseReadAddress(Device *device, const unsigned char* buff);
    void ParseButtons(const unsigned char* buff);
    void ParseAccel(const unsigned char* buff);

	// memory read/write
	enum MEMORY_TYPE {
		MEMORY_NONE,
		MEMORY_READ,
		MEMORY_WRITE,
	};
	struct MemoryData
	{

		MEMORY_TYPE type;
		int address;
		unsigned char data;

		MemoryData(MEMORY_TYPE _type, int _address, unsigned char _data)
			: type(_type), address(_address), data(_data)
		{}
	};
	std::list<MemoryData> m_memoryQueue;
	MEMORY_TYPE m_currentMemory;
	void readMemory(Device *device, int address, unsigned char data);
	void writeMemory(Device *device, int address, unsigned char data);
	void dequeue(Device *device);
	void onReadMemory(Device *device);
	void onWriteMemory(Device *device);
    void initializeExtension(hid::Device *device);
};

}
