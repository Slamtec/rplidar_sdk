#pragma once
#include "lidar_common_types.h"
#include "message_dispatcher.h"

namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            class lidarProtocol:public rpos::ev::MessageDispatcher<rpos::net::hokuyoLidar::lidar_message_autoptr_t &>
            {
            public:
                enum 
                {
                    EVENT_MSG_ARRIVED = 0,
                    EVENT_MSG_ERROR_DETECTED,
                    EVENT_MSG_ERROR_COMMAND,
                    EVENT_MSG_LOSTCONNECT,
                };
            public:
                lidarProtocol(){}
                virtual ~lidarProtocol(){}
            public:
                virtual bool serialize(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping, const std::string& defineInfo,std::vector<unsigned char>& buffer) = 0;
                virtual bool serialize(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping,unsigned char skips,unsigned short scans,const std::string& defineInfo,std::vector<unsigned char>& buffer) = 0;
                virtual bool serialize(const std::string& code, const std::string& defineInfo,std::vector<unsigned char>& buffer) = 0;
                virtual bool serialize(const std::string& code, unsigned char control,const std::string& defineInfo,std::vector<unsigned char>& buffer) = 0;
                virtual void decodeData(std::vector<unsigned char>& buffer) =0;
            };
        }
    }
}