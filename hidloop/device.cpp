#include "device.h"


namespace hid {

bool Device::open(boost::asio::io_service &io_service, std::shared_ptr<IOnRead> callback)
{
    auto handle = CreateFile(m_path.c_str(), GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED, NULL);
    if(handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    m_stream=std::make_shared<boost::asio::windows::stream_handle>(io_service, handle);

    beginRead(callback);

    return true;
}

void Device::write(std::vector<unsigned char> &data)
{
    m_stream->async_write_some(boost::asio::buffer(&data[0], data.size()), [](
                const boost::system::error_code& error,
                size_t bytes_transferred
                ){

            if(error){
            std::cout  << error.message() << std::endl;
            return;
            }

            std::cout << "write " << bytes_transferred << " bytes" << std::endl;

            });
}

void Device::beginRead(std::shared_ptr<IOnRead> callback)
{
    auto device=this;
    m_stream->async_read_some(boost::asio::buffer(m_buf, sizeof(m_buf)),
            [callback, device](
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
            device->beginRead(callback);

            // callback
            if(!tmp.empty()){
            callback->onRead(&tmp[0], tmp.size());
            }
            }
    );
}

}
