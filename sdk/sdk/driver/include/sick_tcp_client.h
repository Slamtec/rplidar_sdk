#pragma once

#include <rpos/system/util/tcp_client.h>
#include <rpos/system/signal.h>
#include "cwaiter.h"

namespace rpos { namespace net { namespace sickLidar {

using namespace rpos::net::hokuyoLidar;

const int RxBufferSize = 0x2000;

class SicklidarTcpClientHandler : public rpos::system::util::TcpClient<SicklidarTcpClientHandler, RxBufferSize>::EmptyTcpClientHandler
{
public:
    typedef rpos::system::util::TcpClient<SicklidarTcpClientHandler, RxBufferSize>::Pointer Pointer;

public:
    virtual void onHostResolveFailure(Pointer client, const boost::system::error_code& ec);
    virtual void onConnected(Pointer client, boost::asio::ip::basic_resolver_iterator<boost::asio::ip::tcp> connectedEntry);
    virtual void onConnectionFailure(Pointer client, const boost::system::error_code& ec);
    virtual void onSendError(Pointer client, const boost::system::error_code& ec);
    virtual void onSendComplete(Pointer client);
    virtual void onReceiveError(Pointer client, const boost::system::error_code& ec);
    virtual void onReceiveComplete(Pointer client, const unsigned char* buffer, size_t readBytes);
    virtual void onConnectionClosed(Pointer client);
};

class SickTcpClient : public rpos::system::util::TcpClient<SicklidarTcpClientHandler, RxBufferSize>
{
public:
    SickTcpClient();
    ~SickTcpClient();

public:
    bool establishConnection(const std::string& addr, unsigned int port, unsigned int timeOut = 3000);
    void setReceivedMsgHandler(const boost::function<void(const unsigned char*, size_t readBytes)>& handler);
    void setLostConnectionHandler(const boost::function<void()>& handler);

private:
    void onConnected(bool success);
    void onClosed();
    void onSend(bool succsss);
    void onReceived(bool success, const unsigned char* buffer = NULL, size_t readBytes = 0);

public:
    void startScanner();
    void stopScanner();
    void getDeviceIdentity();
    void getSerialNumber();
    void getFirmwareVersion();
    bool isCompatibleDevice(const std::string identStr) const;

private:
    void sendCommand(const char* request, const size_t length);

private:
    friend class SicklidarTcpClientHandler;

private:
    rpos::system::Signal<void(const unsigned char*, size_t readBytes)> signalMessageArrived_;
    rpos::system::Signal<void()> signalLostConnection_;
    CWaiter<bool> _connect_waiter;
};

} } }