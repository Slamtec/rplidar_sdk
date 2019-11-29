#pragma once
#include "lidar_common_types.h"
#include <vector>
namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            class Lidar:public rpos::ev::MessageDispatcher<lidar_message_autoptr_t&>
            {
            public:
                typedef enum 
                {
                    Distance = 0,           
                    Distance_intensity,
                    Multiecho,          
                    Multiecho_intensity, 
                }measurement_type_t;

                typedef enum 
                {
                    Serial,
                    Ethernet,
                } connection_type_t;
                Lidar(){}
                virtual ~Lidar()
                {
                }
                virtual bool open(const std::string& device_name, long baudrate,connection_type_t type) = 0;
                virtual void close(void) = 0;
                virtual bool is_open(void) const = 0;

                virtual bool laser_on(void) = 0;
                virtual bool laser_off(void) = 0;

                virtual void reboot(void) = 0;

                virtual void sleep(void) = 0;
                virtual void wakeup(void) = 0;

                //scan_times<99,skip_scan<9
                virtual bool start_measurement(measurement_type_t type, unsigned char scan_times = 0, unsigned char skip_scan =0) = 0;
                virtual bool set_scanning_parameter(int first_step, int last_step,int skip_step) = 0;
                virtual bool get_distance(std::vector<int>& data,int& time_stamp,unsigned int time_out) = 0;
                virtual bool get_distance_intensity(std::vector<int>& data,std::vector<int>& intensity,int& time_stamp) = 0;
                virtual bool get_multiecho(std::vector<int>& data_multi,int& time_stamp) = 0;
                virtual bool get_multiecho_intensity(std::vector<int>& data_multiecho,std::vector<int>& intensity_multiecho,int& time_stamp) = 0;
                virtual void stop_measurement(void) = 0;
                virtual double index2rad(int index) const = 0;
                virtual double index2deg(int index) const = 0;
                virtual int rad2index(double radian) const = 0;
                virtual int deg2index(double degree) const = 0;
                virtual int rad2step(double radian) const = 0;
                virtual int deg2step(double degree) const = 0;
                virtual double step2rad(int step) const = 0;
                virtual double step2deg(int step) const = 0;
                virtual int step2index(int step) const = 0;

                virtual int min_step(void) const = 0;
                virtual int max_step(void) const = 0;
                virtual long min_distance(void) const = 0;
                virtual long max_distance(void) const = 0;
                virtual long scan_usec(void) const = 0;
                virtual int max_data_size(void) const = 0;
                virtual int max_echo_size(void) const = 0;

                virtual std::string product_type(void) = 0;
                virtual std::string firmware_version(void) = 0;
                virtual std::string serial_id(void) = 0;
                virtual std::string status(void) = 0;
                virtual std::string state(void) = 0;
                virtual void _addAllChannelListeners() = 0;
                virtual void _removeAllChannelListeners() = 0;
            };
        }
    }
}