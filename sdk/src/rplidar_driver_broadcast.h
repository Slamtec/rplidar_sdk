/*
 * RPLIDAR SDK - Broadcast Driver
 *
 * UDP broadcast-based device discovery and configuration
 */

#pragma once

#include "hal/types.h"
#include "rplidar_cmd.h"
#include "net/udp_broadcast_channel.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace rp { namespace standalone { namespace rplidar {

class RPLidarBroadcastDriver
{
public:
    RPLidarBroadcastDriver();
    ~RPLidarBroadcastDriver();

    // Initialize broadcast driver with network adapter IPs
    u_result init(const std::vector<std::string>& adapterIPs);

    // Shutdown and cleanup
    void shutdown();

    // Get device information via broadcast
    // Returns list of discovered devices (deduplicated by serial number)
    u_result getDeviceInfoViaBroadcast(
        std::vector<rplidar_response_broadcast_device_info_t>& devices,
        _u32 timeout_ms = 1000);

    // Set device IP configuration via broadcast
    // Uses serial number to target specific device
    u_result setDeviceIpViaBroadcast(
        const rplidar_payload_ip_set_related_t& ipConfig,
        const _u8 serialNum[16],
        _u32 timeout_ms = 2000);

    // Check if initialized
    bool isInitialized() const { return _isInitialized; }

protected:
    // Send command and wait for responses
    u_result sendCommandAndCollectResponses(
        _u8 cmd,
        const void* payload,
        size_t payloadSize,
        std::vector<std::vector<_u8>>& responses,
        _u32 timeout_ms);

    // Build command packet
    void buildCommandPacket(
        _u8 cmd,
        const void* payload,
        size_t payloadSize,
        std::vector<_u8>& packet);

    // Parse response packet
    bool parseResponsePacket(
        const _u8* buffer,
        size_t size,
        _u8 expectedType,
        std::vector<_u8>& payload);

    // Deduplicate devices by serial number
    void deduplicateDevices(
        std::vector<rplidar_response_broadcast_device_info_t>& devices);

protected:
    rp::net::UDPBroadcastChannel _channel;
    rp::net::DGramSocket*        _rxSocket;
    std::vector<rp::net::DGramSocket*> _txSockets;

    std::vector<std::string>     _adapterIPs;
    bool                         _isInitialized;

    static const int BROADCAST_RX_PORT = 0x534C;
    static const int BROADCAST_TX_PORT = 0x5250;
};

}}} // namespace rp::standalone::rplidar
