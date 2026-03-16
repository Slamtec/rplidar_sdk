/*
 * RPLIDAR SDK - UDP Broadcast Channel
 *
 * Simplified version for device discovery (without mDNS)
 */

#ifdef _WIN32
#define _WINSOCKAPI_
#endif

#include "sdkcommon.h"
#include "net/udp_broadcast_channel.h"
#include "hal/locker.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

namespace rp { namespace net {

UDPBroadcastChannel::UDPBroadcastChannel()
    : _bindedRxSocket(nullptr)
    , _isOpen(false)
{
}

UDPBroadcastChannel::~UDPBroadcastChannel()
{
    rp::hal::AutoLocker l(_opLocker);
    close();
}

u_result UDPBroadcastChannel::bind(
    rp::net::DGramSocket* rxSocket,
    std::vector<rp::net::DGramSocket*> txSockets,
    const std::string& broadcastAddr,
    int broadcastPort,
    udpchannel_flag_t flag)
{
    rp::hal::AutoLocker l(_opLocker);

    if (_isOpen) {
        return RESULT_ALREADY_DONE;
    }

    _bindedRxSocket = rxSocket;
    _bindedTxSockets = txSockets;
    _broadcastAddr = SocketAddress(broadcastAddr.c_str(), broadcastPort);
    _isOpen = true;

    return RESULT_OK;
}

u_result UDPBroadcastChannel::sendData(_u8* buffer, size_t size)
{
    rp::hal::AutoLocker l(_opLocker);

    if (!_isOpen || _bindedTxSockets.empty()) {
        return RESULT_OPERATION_FAIL;
    }

    u_result ans = RESULT_OK;

    // Send broadcast through all TX sockets (one per network adapter)
    for (size_t i = 0; i < _bindedTxSockets.size(); i++) {
        ans = _bindedTxSockets[i]->sendTo(&_broadcastAddr, buffer, size);
        if (IS_FAIL(ans)) {
            return ans;
        }
    }

    return ans;
}

u_result UDPBroadcastChannel::recvData(_u8* buffer, size_t bufferSize,
                                        size_t& actualSize, _u32 timeout_ms)
{
    if (!_isOpen || !_bindedRxSocket) {
        return RESULT_OPERATION_FAIL;
    }

    // Set timeout
    if (timeout_ms > 0) {
        _bindedRxSocket->setTimeout(timeout_ms, SocketBase::SOCKET_DIR_RD);
    }

    SocketAddress fromAddr("0.0.0.0", 0);
    u_result ans = _bindedRxSocket->recvFrom(buffer, bufferSize, actualSize, &fromAddr);

    return ans;
}

u_result UDPBroadcastChannel::waitForData(_u32 timeout_ms)
{
    if (!_isOpen || !_bindedRxSocket) {
        return RESULT_OPERATION_FAIL;
    }

    return _bindedRxSocket->waitforData(timeout_ms);
}

void UDPBroadcastChannel::close()
{
    if (_isOpen) {
        _bindedRxSocket = nullptr;
        _bindedTxSockets.clear();
        _isOpen = false;
    }
}

std::set<std::string> UDPBroadcastChannel::getLocalEthernetIPs()
{
    std::set<std::string> ipAddresses;

#ifdef _WIN32
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufferSize);

    if (pAddresses == nullptr) {
        return ipAddresses;
    }

    DWORD result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufferSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufferSize);
        if (pAddresses == nullptr) {
            return ipAddresses;
        }
        result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufferSize);
    }

    if (result == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next) {
            // Only process wired Ethernet adapters that are UP
            if (pCurr->IfType == IF_TYPE_ETHERNET_CSMACD && pCurr->OperStatus == IfOperStatusUp) {
                for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                     pUnicast != nullptr; pUnicast = pUnicast->Next) {
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        struct sockaddr_in* pAddr = (struct sockaddr_in*)pUnicast->Address.lpSockaddr;

                        // Use inet_ntoa for IP to string conversion
                        char* ipStr = inet_ntoa(pAddr->sin_addr);
                        if (ipStr != nullptr) {
                            ipAddresses.insert(ipStr);
                        }
                    }
                }
            }
        }
    }

    free(pAddresses);
#else
    // Linux implementation would go here
    // For now, Windows-only as per requirements
#endif

    return ipAddresses;
}

}} // namespace rp::net
