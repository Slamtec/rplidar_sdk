#pragma once
#ifdef WIN32
#include <Windows.h>
#endif
#include "../include/lidar_protocol.h"

namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            class HokuyoProtocol;

            typedef void (HokuyoProtocol::*ActionHandlerT)(std::vector<unsigned char>&,size_t);
            // #define ACFunc(FuncName) (ActionHandlerT)(&HokuyoProtocol::##FuncName)

            struct LidarCmdInfo
            {
                unsigned int funtionId;
                ActionHandlerT onActionHandler;
            };

            enum 
            {
                RP_STAGE_ECHO = 0x00, //Flag
                RP_STAGE_STATE,
                RP_STAGE_TIME, 
                RP_STAGE_CURRENT_SENSOR,
                RP_STAGE_DISTANCE,
                RP_STAGE_DISTANCE_INTENSITY,
                RP_STAGE_MULITI_DISTANCE,
                RP_STAGE_MULTI_DISTANCE_INTENSITY,
                RP_STAGE_VERSION,
                RP_STAGE_SENSOR_PARAM,
                RP_STAGE_SENSOR_STATE
            };

            class HokuyoProtocol:public lidarProtocol
            {
            public:
                HokuyoProtocol():
                  previousIsError_(false),
                      check_error_(false),
                      state_(RP_STAGE_ECHO)
                  {
                      message_ = boost::shared_ptr<UrgMessage>(new UrgMessage);
                      reset();
                  }
                  bool serialize(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping, const std::string& defineInfo,std::vector<unsigned char>& buffer);
                  bool serialize(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping,unsigned char skips,unsigned short scans,const std::string& defineInfo,std::vector<unsigned char>& buffer);
                  bool serialize(const std::string& code, const std::string& defineInfo,std::vector<unsigned char>& buffer);
                  bool serialize(const std::string& code, unsigned char control,const std::string& defineInfo,std::vector<unsigned char>& buffer);

                  void decodeData(std::vector<unsigned char>& buffer);
            public:
                unsigned char scipChecksum(const std::vector<unsigned char>& buffer,unsigned int begin,unsigned int end);
                unsigned int scipDecode(const std::vector<unsigned char>&,unsigned int begin,unsigned int end);

                void decodeTime(std::vector<unsigned char>& timeData,size_t end);
                bool isMeasurementCmd(const std::string& strCmd);

                void decodeEchoBacMsg(std::vector<unsigned char>& buffer,size_t end);
                void decodeStatus(std::vector<unsigned char>& buffer,size_t end);
                void decodeDistance(std::vector<unsigned char>& buffer,size_t end);
                void decodeMultieDistance(std::vector<unsigned char>& buffer,size_t end);
                void decodeMultieDistanceIntensity(std::vector<unsigned char>& buffer,size_t end);
                void decodeParameter(std::vector<unsigned char>& buffer,size_t end);
                void decodeSensorState(std::vector<unsigned char>& buffer,size_t end);
                void decodeSensorVersion(std::vector<unsigned char>& buffer,size_t end);
                void decode_distance_intensity(std::vector<unsigned char>& buffer,size_t end);
                void decodeCurrentSensorState(std::vector<unsigned char>& buffer,size_t end);
                void reset();
                unsigned int calculateRestsize(std::vector<unsigned char>& buffer,size_t begin,size_t size,unsigned int blockSize);
            protected:
                void dispatchCurrentMessage(int eventId);
            private:
                LidarCmdInfo* FindCmdInfo(unsigned int cmdId);
                bool error_occurred(const std::string& strCode,int code);
            private:
                std::vector<unsigned char> transferingBuffer_;
                std::vector<unsigned char> restBlockBuffer_;
                bool previousIsError_;
                unsigned char state_;
                std::string strCmd_;
                bool check_error_;
                unsigned int currentSensorState_;
                lidar_message_autoptr_t message_;
            };
        }
    }
}