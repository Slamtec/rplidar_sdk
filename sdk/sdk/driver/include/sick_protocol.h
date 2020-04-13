#pragma once

#include "../include/lidar_protocol.h"

namespace rpos { namespace net { namespace sickLidar {

using namespace rpos::net::hokuyoLidar;

class SickProtocol : public lidarProtocol
{
private:
    enum
    {
        CMD_INVALID,
        CMD_START_SCANNER,
        CMD_GET_DATAGRAM,
        CMD_DEVICE_IDENTITY,
        CMD_SERIAL_NUMBER,
        CMD_FIRMWARE_VERSION,
    };

    struct LidarCmdHeader
    {
        int cmdType;
        std::string cmdHeader;
    };

public:
    SickProtocol();
    ~SickProtocol();

    bool serialize(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping, const std::string& defineInfo, std::vector<unsigned char>& buffer);
    bool serialize(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping, unsigned char skips, unsigned short scans, const std::string& defineInfo, std::vector<unsigned char>& buffer);
    bool serialize(const std::string& code, const std::string& defineInfo, std::vector<unsigned char>& buffer);
    bool serialize(const std::string& code, unsigned char control, const std::string& defineInfo, std::vector<unsigned char>& buffer);

    void decodeData(std::vector<unsigned char>& buffer);

private:
    int truncDatagram(std::vector<unsigned char>& buffer, const unsigned char** data, size_t* size);
    int decodeDatagramHeader(const unsigned char* buffer, size_t size);
    bool parseDatagram(const unsigned char* datagram, int datagram_length, std::vector<int>& distances);
    bool parseDeviceInfo(const unsigned char* buffer, int buffer_length, std::string& str);

    void dispatchCurrentMessage(int eventId);
    void reset();

private:
    std::vector<unsigned char> lastBuffer_;
    lidar_message_autoptr_t message_;
};

} } }
