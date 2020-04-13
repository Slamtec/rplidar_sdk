#pragma once

#include "lidar_channel.h"
#include "sick_tcp_client.h"
#include "sick_protocol.h"

namespace rpos { namespace net { namespace sickLidar {

using namespace rpos::net::hokuyoLidar;

class SickChannel : public LidarChannel
{
public:
    SickChannel();
    ~SickChannel();

public:
    virtual void close();
    virtual bool isConnected();
    virtual bool connect(const std::string& addr, int port);
    virtual void asyncRequest(const std::vector<unsigned char>& request);

protected:
    virtual void onLostConnect();

private:
    boost::shared_ptr<SickTcpClient> _client;
    bool _connected;
};

} } }
