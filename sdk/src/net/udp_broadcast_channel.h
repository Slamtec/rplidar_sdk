/*
 * RPLIDAR SDK - UDP Broadcast Channel
 *
 * Simplified version for device discovery (without mDNS)
 */

#pragma once

#include "hal/types.h"
#include "hal/locker.h"
#include "hal/socket.h"
#include <vector>
#include <string>
#include <set>

namespace rp { namespace net {

class UDPBroadcastChannel
{
public:
    enum udpchannel_flag_t {
        UDPCHANNEL_FLAG_NONE    = 0,
        UDPCHANNEL_FLAG_RX_ONLY = 0x1L << 0,
        UDPCHANNEL_FLAG_TX_ONLY = 0x1L << 1,
    };

public:
    UDPBroadcastChannel();
    virtual ~UDPBroadcastChannel();

    // Bind channel with RX socket and multiple TX sockets (one per adapter)
    u_result bind(rp::net::DGramSocket* rxSocket,
                  std::vector<rp::net::DGramSocket*> txSockets,
                  const std::string& broadcastAddr = "255.255.255.255",
                  int broadcastPort = 0x534C/*'S''L'*/,
                  udpchannel_flag_t flag = UDPCHANNEL_FLAG_NONE);

    // Send data via broadcast (to all adapters)
    u_result sendData(_u8* buffer, size_t size);

    // Receive data with timeout
    u_result recvData(_u8* buffer, size_t bufferSize,
                      size_t& actualSize, _u32 timeout_ms);

    // Wait for incoming data
    u_result waitForData(_u32 timeout_ms);

    void close();

    // Get binded RX socket
    rp::net::DGramSocket* getBindedSocket() {
        return _bindedRxSocket;
    }

    // Get local wired Ethernet adapter IPs
    static std::set<std::string> getLocalEthernetIPs();

protected:
    rp::net::DGramSocket*              _bindedRxSocket;
    std::vector<rp::net::DGramSocket*> _bindedTxSockets;
    SocketAddress                      _broadcastAddr;
    rp::hal::Locker                    _opLocker;
    bool                               _isOpen;
};

}} // namespace rp::net
