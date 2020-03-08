#pragma once

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "lidar.h"
#include "sick_channel.h"

namespace rpos { namespace net { namespace sickLidar {

using namespace rpos::net::hokuyoLidar;

class Sick_driver : public Lidar
{
private:
    typedef enum
    {
        COMMUNICATION_3_BYTE,
        COMMUNICATION_2_BYTE,
    } range_data;

    struct  scan_parameter
    {
        int scanning_first_step;
        int scanning_last_step;
        int scanning_skip_step;
        int scanning_skip_scan;

        int received_first_index;
        int received_last_index;
        int received_skip_step;
        int time_stamp;
        int timeout;
        int specified_scan_times;
        int scanning_remain_times;
        bool is_laser_on;
        int range_data_byte;
        int measurement_type;
        int last_errno;
        int time_stamp_offset;

        scan_parameter()
        {
            scanning_first_step = 0;
            scanning_last_step = 0;
            scanning_skip_step = 0;
            scanning_skip_scan = 0;
            received_first_index = 0;
            received_last_index = 0;
            received_skip_step = 0;
            timeout = 0;
            specified_scan_times = 0;
            scanning_remain_times = 0;
            is_laser_on = true;
            range_data_byte = COMMUNICATION_3_BYTE;
            measurement_type = Distance;
            last_errno = 0;
            time_stamp_offset = 0;
        }
    };

    static const long default_port = 2112;

public:
    Sick_driver();
    ~Sick_driver(void);

    bool open(const std::string& device_name, long baudrate = Sick_driver::default_port, Lidar::connection_type_t type = Lidar::Serial);
    void close(void);
    bool is_open(void) const;

    bool laser_on(void);
    bool laser_off(void);

    void reboot(void);
    void sleep(void);
    void wakeup(void);

    bool start_measurement(Lidar::measurement_type_t type, unsigned char scan_times = 0, unsigned char skip_scan = 0);
    void stop_measurement();
    bool set_scanning_parameter(int first_step, int last_step, int skip_step = 1);
    //void set_measurement_type(measurement_type_t type);

    virtual bool get_distance(std::vector<int>& data, int& time_stamp, unsigned int time_out = 0);
    virtual bool get_distance_intensity(std::vector<int>& data, std::vector<int>& intensity, int& time_stamp);
    virtual bool get_multiecho(std::vector<int>& data_multi, int& time_stamp);
    virtual bool get_multiecho_intensity(std::vector<int>& data_multiecho, std::vector<int>& intensity_multiecho, int& time_stamp);

    double index2rad(int index) const;
    double index2deg(int index) const;
    int rad2index(double radian) const;
    int deg2index(double degree) const;
    int rad2step(double radian) const;
    int deg2step(double degree) const;
    double step2rad(int step) const;
    double step2deg(int step) const;
    int step2index(int step) const;

    int min_step(void) const;
    int max_step(void) const;
    long min_distance(void) const;
    long max_distance(void) const;
    long scan_usec(void) const;
    int max_data_size(void) const;
    int max_echo_size(void) const;

    std::string product_type();
    std::string firmware_version();
    std::string serial_id();
    std::string status();
    std::string state();

protected:
    virtual void _addAllChannelListeners();
    virtual void _removeAllChannelListeners();

protected:
    virtual void _handlerChannelEvent(unsigned int eventId, evtDispatcher_t & channel, lidar_message_autoptr_t& message);
    void _handleAnsPkt(unsigned int ansResult, evtDispatcher_t & channel, lidar_message_autoptr_t& message);

protected:
    std::list<evtHandler_t *> handlers;

private:
    boost::shared_ptr<SickChannel> channel_;
    bool active_;

    boost::mutex _msgLock;
    CWaiter<bool> _startMeasuremen_waiter;
    CWaiter<bool> _laserCtrol_waiter;
    CEvent _scanData_waiter;

    struct
    {
        SensorParameters sensorPara;
        SensorVersion sensorVer;
        scan_parameter scan_info;
        SensorState sensorState;
        std::vector<int> distance_data;
        std::vector<int> multiecho_data;
        struct
        {
            std::vector<int> data;
            std::vector<int> intensity;
        } distance_intensity, multiecho_intensity;
    } _lastAnsPkt;
};

} } }