#pragma once 
#include <string>
#include <vector>

namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            struct SensorParameters 
            {
                std::string model;
                int min_distance;
                int max_distance;
                int area_resolution;
                int first_data_index;
                int last_data_index;
                int front_data_index; 
                int scan_usec;
                SensorParameters()
                {
                    min_distance = 0;
                    max_distance = 0;
                    area_resolution = 0;
                    first_data_index = 0;
                    last_data_index = 0;
                    front_data_index = 0;
                    scan_usec = 0;
                }
            };

            struct SensorVersion
            {
                std::string vendorInfo;
                std::string productInfo;
                std::string firmwareVersion;
                std::string protocolVersion;
                std::string serialNumber;
            };

            struct SensorState
            {
                std::string model;
                std::string lasterStatus;
                std::string currentScanSpeed;
                std::string state;
                std::string communicationSpeed;
                std::string time;
                std::string sensorStaus;
            };

            struct distanceInstenisty
            {
                int distance;
                int instensity;
                distanceInstenisty()
                {
                    distance = 0;
                    instensity = 0;
                }
            };

            struct  UrgMessage
            {
                int type; //消息类型
                int status_code; //状态吗
                bool error_occurred; //是否出错
                SensorParameters SensorPara;
                SensorVersion sensorVer;
                SensorState sensorState;
                int received_first_index; //start
                int received_last_index; //end
                unsigned char received_skip_step;
                unsigned char scan_times;
                int times_smap; //时间戳
                std::vector<int> distances;
                std::vector<int> multiecho;
                std::vector<distanceInstenisty> distance_intensity;
                std::vector<distanceInstenisty> multiecho_intensity;
                UrgMessage() 
                {
                    error_occurred = false;
                    received_first_index = 0;
                    received_last_index = 0;
                    scan_times = 0;
                    received_skip_step = 0;
                    times_smap = 0;
                };
            };
        }
    }
}