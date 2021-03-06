#include "device.h"
#include "icallback.h"
#include "hid.h"


namespace hid {

Device::Device(unsigned short vendorID, unsigned short productID)
: m_vendorID(vendorID), m_productID(productID)
{
}

Device::~Device()
{
    if(m_callback){
        m_callback->onDestroy(this);
    }
}

bool Device::open(boost::asio::io_service &io_service, std::shared_ptr<ICallback> callback)
{
    auto handle = CreateFile(m_path.c_str(), GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED, NULL);
    if(handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    m_handle=std::shared_ptr<void>(handle, &::CloseHandle);

    // get hid caps
    {
        HIDP_PREPARSED_DATA* preparsedData=0;
        if (!HidD_GetPreparsedData(handle, &preparsedData)){
            return false;
        }
        std::shared_ptr<HIDP_PREPARSED_DATA> parsed(
                preparsedData, HidD_FreePreparsedData);

        HIDP_CAPS caps;
        if (HidP_GetCaps(preparsedData, &caps) != HIDP_STATUS_SUCCESS)
        {
            return false;
        }
        std::cout << "Usage: " << caps.Usage << std::endl;
        std::cout << "UsagePage: " <<  caps.UsagePage << std::endl;
        std::cout << "InputReportByteLength: " 
            << caps.InputReportByteLength << std::endl;
        std::cout << "OutputReportByteLength: " 
            <<  caps.OutputReportByteLength << std::endl;
        std::cout << "FeatureReportByteLength: "
            <<  caps.FeatureReportByteLength << std::endl;
    }

    // setup stream
    m_stream=std::make_shared<boost::asio::windows::stream_handle>(io_service, handle);
    m_callback=callback;

    beginRead();
    m_callback->onConnect(this);

    return true;
}

void Device::write(std::vector<unsigned char> &data)
{
    auto on_write=[](
            const boost::system::error_code& error,
            size_t bytes_transferred
            ){

        if(error){
            std::cout  << error.message() << std::endl;
            return;
        }
        //std::cout << "write " << bytes_transferred << " bytes" << std::endl;
    };
    m_stream->async_write_some(
            boost::asio::buffer(&data[0], data.size()), 
            on_write);
}

void Device::beginRead()
{
    auto device=shared_from_this();
    auto on_read=[device](
            const boost::system::error_code& error,
            size_t bytes_transferred
            ){

        if(error){
            std::cout  << error.message() << std::endl;
            return;
        }

        // copy
        std::vector<unsigned char> tmp(device->m_buf, device->m_buf+bytes_transferred);

        // next
        device->beginRead();

        // callback
        if(!tmp.empty()){
            device->m_callback->onRead(device.get(), tmp);
        }
    };
    m_stream->async_read_some(
            boost::asio::buffer(m_buf, sizeof(m_buf)),
            on_read
    );
}

}
