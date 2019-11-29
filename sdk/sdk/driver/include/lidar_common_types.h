#pragma once
#include "message_dispatcher.h"
#include "message_handler.h"
#include "sensor.h"
#include <boost/shared_ptr.hpp>

namespace rpos 
{ 
    namespace net
    {
        namespace hokuyoLidar
        {
            typedef enum
            {
                Scanning_Param,
                Distance_Data,            
                Distance_Intensity_Data,  
                Multiecho_Data,          
                Multiecho_Intensity_Data,
                Sensor_Parameters,
                Sensor_State,
                Sensor_Version,
                Time_Mode_On,
                Time_Request,
                Time_Mode_Off,
                Laser_On,
                Laser_Off,
                Reboot_Echo,
                Sleep_Echo,
                Echo_Back
            }messsage_type_t;

            typedef boost::shared_ptr<UrgMessage> lidar_message_autoptr_t;
            typedef rpos::ev::MessageDispatcher<lidar_message_autoptr_t &>  evtDispatcher_t;
            typedef rpos::ev::AbsMessageHandler<lidar_message_autoptr_t &>  evtHandler_t;
        }
    }
}