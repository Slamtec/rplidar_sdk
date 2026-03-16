/*
 * RPLIDAR SDK - Broadcast Driver Implementation
 *
 * UDP broadcast-based device discovery and configuration
 */

#include "sdkcommon.h"
#include "rplidar_driver_broadcast.h"
#include "rplidar_protocol.h"
#include "hal/locker.h"
#include "hal/socket.h"
#include <cstring>
#include <algorithm>

namespace rp { namespace standalone { namespace rplidar {

RPLidarBroadcastDriver::RPLidarBroadcastDriver()
    : _rxSocket(nullptr)
    , _isInitialized(false)
{
}

RPLidarBroadcastDriver::~RPLidarBroadcastDriver()
{
    shutdown();
}

u_result RPLidarBroadcastDriver::init(const std::vector<std::string>& adapterIPs)
{
    if (_isInitialized) {
        return RESULT_ALREADY_DONE;
    }

    _adapterIPs = adapterIPs;

    // Create RX socket (bind to 0.0.0.0:BROADCAST_RX_PORT)
    _rxSocket = rp::net::DGramSocket::CreateSocket();
    if (!_rxSocket) {
        return RESULT_OPERATION_FAIL;
    }

    rp::net::SocketAddress rxAddr("0.0.0.0", BROADCAST_RX_PORT);
    u_result ans = _rxSocket->bind(rxAddr);
    if (IS_FAIL(ans)) {
        _rxSocket->dispose();
        _rxSocket = nullptr;
        return ans;
    }

    // Create TX sockets (one per adapter)
    _txSockets.clear();
    for (const auto& ip : adapterIPs) {
        rp::net::DGramSocket* txSocket = rp::net::DGramSocket::CreateSocket();
        if (!txSocket) {
            continue;
        }

        // Bind to adapter IP with fixed source port
        rp::net::SocketAddress txAddr(ip.c_str(), BROADCAST_RX_PORT);
        ans = txSocket->bind(txAddr);
        if (IS_FAIL(ans)) {
            txSocket->dispose();
            continue;
        }

        _txSockets.push_back(txSocket);
    }

    if (_txSockets.empty()) {
        _rxSocket->dispose();
        _rxSocket = nullptr;
        return RESULT_OPERATION_FAIL;
    }

    // Bind channel
    ans = _channel.bind(_rxSocket, _txSockets, "255.255.255.255", BROADCAST_TX_PORT);
    if (IS_FAIL(ans)) {
        shutdown();
        return ans;
    }

    _isInitialized = true;
    return RESULT_OK;
}

void RPLidarBroadcastDriver::shutdown()
{
    if (!_isInitialized) {
        return;
    }

    _channel.close();

    if (_rxSocket) {
        _rxSocket->dispose();
        _rxSocket = nullptr;
    }

    for (auto* sock : _txSockets) {
        if (sock) {
            sock->dispose();
        }
    }
    _txSockets.clear();

    _isInitialized = false;
}

u_result RPLidarBroadcastDriver::getDeviceInfoViaBroadcast(
    std::vector<rplidar_response_broadcast_device_info_t>& devices,
    _u32 timeout_ms)
{
    if (!_isInitialized) {
        return RESULT_OPERATION_FAIL;
    }

    devices.clear();

    // Send one broadcast and collect responses within timeout
    std::vector<std::vector<_u8>> responses;
    u_result ans = sendCommandAndCollectResponses(
        RPLIDAR_CMD_GET_BROADCAST_DEVICE_INFO,
        nullptr,
        0,
        responses,
        timeout_ms);

    if (IS_OK(ans)) {
        std::unordered_map<std::string, rplidar_response_broadcast_device_info_t> deviceMap;

        for (const auto& response : responses) {
            if (response.size() < sizeof(rplidar_response_broadcast_device_info_t)) {
                continue;
            }

            rplidar_response_broadcast_device_info_t info;
            memcpy(&info, response.data(), sizeof(info));

            std::string key((char*)info.dev_info.serialnum, 16);
            deviceMap[key] = info;
        }

        for (const auto& pair : deviceMap) {
            devices.push_back(pair.second);
        }
    }

    if (devices.empty()) {
        return RESULT_OPERATION_TIMEOUT;
    }

    return RESULT_OK;
}

u_result RPLidarBroadcastDriver::setDeviceIpViaBroadcast(
    const rplidar_payload_ip_set_related_t& ipConfig,
    const _u8 serialNum[16],
    _u32 timeout_ms)
{
    if (!_isInitialized) {
        return RESULT_OPERATION_FAIL;
    }

    // Build payload: serial number + IP config
    struct {
        _u8 sn[16];
        rplidar_payload_ip_set_related_t ip;
    } payload;

    memcpy(payload.sn, serialNum, 16);
    payload.ip = ipConfig;

    std::vector<std::vector<_u8>> responses;
    u_result ans = sendCommandAndCollectResponses(
        RPLIDAR_CMD_SET_BROADCAST_DEVICE_ETH_INFO,
        &payload,
        sizeof(payload),
        responses,
        timeout_ms);

    return ans;
}

u_result RPLidarBroadcastDriver::sendCommandAndCollectResponses(
    _u8 cmd,
    const void* payload,
    size_t payloadSize,
    std::vector<std::vector<_u8>>& responses,
    _u32 timeout_ms)
{
    responses.clear();

    // Build command packet
    std::vector<_u8> cmdPacket;
    buildCommandPacket(cmd, payload, payloadSize, cmdPacket);

    // Send broadcast
    u_result ans = _channel.sendData(cmdPacket.data(), cmdPacket.size());
    if (IS_FAIL(ans)) {
        return ans;
    }

    // Collect responses until timeout
    DWORD startTime = GetTickCount();

    while (true) {
        DWORD now = GetTickCount();
        DWORD elapsed = now - startTime;
        if (elapsed >= timeout_ms) {
            break;
        }

        _u8 buffer[2048];
        size_t actualSize = 0;

        _u32 remainingTime = timeout_ms - elapsed;
        ans = _channel.recvData(buffer, sizeof(buffer), actualSize, remainingTime);

        if (ans == RESULT_OPERATION_TIMEOUT) {
            break;  // Timeout, return collected responses
        }

        if (IS_FAIL(ans)) {
            continue;  // Try next packet
        }

        // Parse response
        std::vector<_u8> payloadData;
        if (parseResponsePacket(buffer, actualSize, RPLIDAR_ANS_TYPE_BROADCAST_DEVICE_INFO, payloadData)) {
            responses.push_back(payloadData);
        }
    }

    return responses.empty() ? RESULT_OPERATION_TIMEOUT : RESULT_OK;
}

void RPLidarBroadcastDriver::buildCommandPacket(
    _u8 cmd,
    const void* payload,
    size_t payloadSize,
    std::vector<_u8>& packet)
{
    packet.clear();

    //magic byte, broadcast port
    packet.push_back(0x52);//R
    packet.push_back(0x50);//P

    // Sync byte
    packet.push_back(RPLIDAR_CMD_SYNC_BYTE);

    // Command flag
    _u8 cmdFlag = cmd;
    if (payloadSize > 0) {
        cmdFlag |= RPLIDAR_CMDFLAG_HAS_PAYLOAD;
    }
    packet.push_back(cmdFlag);

    // If has payload, add size, payload data, and checksum
    if (payloadSize > 0 && payload) {
        // Size byte
        packet.push_back((_u8)payloadSize);

        // Payload data
        const _u8* payloadBytes = (const _u8*)payload;
        packet.insert(packet.end(), payloadBytes, payloadBytes + payloadSize);

        // Calculate checksum (XOR of sync, cmd_flag, size, and all payload bytes)
        _u8 checksum = 0;
        for (size_t i = 0; i < packet.size(); i++) {
            checksum ^= packet[i];
        }
        packet.push_back(checksum);
    }

    packet.push_back(0x0D);
    packet.push_back(0x0A);

}

bool RPLidarBroadcastDriver::parseResponsePacket(
    const _u8* buffer,
    size_t size,
    _u8 expectedType,
    std::vector<_u8>& payload)
{
    payload.clear();

    const size_t headerSize = sizeof(rplidar_ans_header_t);

    // Find sync bytes (RPLIDAR_ANS_SYNC_BYTE1 RPLIDAR_ANS_SYNC_BYTE2) in buffer
    size_t offset = 0;
    bool found = false;
    while (offset + headerSize <= size) {
        if (buffer[offset] == RPLIDAR_ANS_SYNC_BYTE1 && buffer[offset + 1] == RPLIDAR_ANS_SYNC_BYTE2) {
            found = true;
            break;
        }
        offset++;
    }

    if (!found) {
        return false;
    }

    size_t remaining = size - offset;

    if (remaining < headerSize) {
        return false;
    }

    // Parse header
    const rplidar_ans_header_t* header = reinterpret_cast<const rplidar_ans_header_t*>(buffer + offset);

    _u32 payloadSize = header->size_q30_subtype & RPLIDAR_ANS_HEADER_SIZE_MASK;

    // Check type
    if (header->type != expectedType) {
        return false;
    }

    // Check size
    if (remaining < headerSize + payloadSize) {
        return false;
    }

    // Extract payload
    payload.resize(payloadSize);
    memcpy(payload.data(), buffer + offset + headerSize, payloadSize);

    return true;
}

void RPLidarBroadcastDriver::deduplicateDevices(
    std::vector<rplidar_response_broadcast_device_info_t>& devices)
{
    std::unordered_map<std::string, rplidar_response_broadcast_device_info_t> deviceMap;

    for (const auto& device : devices) {
        std::string key((char*)device.dev_info.serialnum, 16);
        deviceMap[key] = device;
    }

    devices.clear();
    for (const auto& pair : deviceMap) {
        devices.push_back(pair.second);
    }
}

}}} // namespace rp::standalone::rplidar
