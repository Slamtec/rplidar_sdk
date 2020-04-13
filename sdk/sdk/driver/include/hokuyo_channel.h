#pragma once
#include "lidar_channel.h"
#include "rplidar_tcp_client.h"
#include "hokuyo_rplidar_protocol.h"
#include <boost/shared_ptr.hpp>
#include "cwaiter.h"

namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            class HokuyoChannel:public hokuyoLidar::LidarChannel
            {
            public:
                HokuyoChannel();
                ~HokuyoChannel();
            public:
                virtual void close();
                virtual bool isConnected();
                virtual bool connect(const std::string& device_name, int baudrate);
                virtual void asyncRequest(const std::vector<unsigned char>& request);
            protected:
                virtual void onLostConect();
            private:
                boost::shared_ptr<RplidarTcpClient> _client;
                bool _connected;
            };
        }
    }
}