/*
 *  SLAMTEC LIDAR
 *  Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2020 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sl_lidar.h" 
#include "sl_lidar_driver.h"

#include <fstream>
#include <cmath>       
#include <sstream>
#include <vector>       
#include <chrono>       

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

using namespace sl;

// Struct to hold force vectors
// forward = +x, left = +y
struct ForceVector {
    double x = 0.0;
    double y = 0.0;
};


// Calculate attractive force
ForceVector attractive_force(ForceVector user_pos, double k_attr = 0.5) {
    ForceVector f_attr;
    // Scaled vector pointing to the user's location
    f_attr.x = k_attr * user_pos.x;
    f_attr.y = k_attr * user_pos.y;
    return f_attr;
}


// Calculate repulsive force
ForceVector repulsive_force(sl_lidar_response_measurement_node_hq_t* nodes, size_t count) {
    const double k_rep = 2000.0; // Repulsive force gain
    const double radius = 3000.0;  // Radius of detecting objects

    ForceVector total_rep_force;

    for (int pos = 0; pos < (int)count; ++pos) {
        double distance_mm = nodes[pos].dist_mm_q2 / 4.0f;

        if (distance_mm >= radius || distance_mm < 1.0) {
            continue;
        }
        
        double angle_deg = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
        double angle_rad = angle_deg * (M_PI / 180.0);

        // Calculate magnitude of repulsive force
        double magnitude = k_rep * (1.0 / distance_mm - 1.0 / radius);
        
        // Calculate force vector for point
        double force_x = magnitude * std::cos(angle_rad + M_PI);
        double force_y = magnitude * std::sin(angle_rad + M_PI);

        // Add to total
        total_rep_force.x += force_x;
        total_rep_force.y += force_y;
    }

    return total_rep_force;
}


ForceVector get_user_position_mm() {
    // user's position relative to cart
    // ex. cart is 2m in front of cart
    ForceVector temp;
    temp.x = 2000.0;
    temp.y = 0.0;
    return temp;
}


void send_motor_commands(ForceVector f_net) {
    // forward/backward speed is x
    // turning speed is y
    const double linear_gain = 0.001; 
    const double angular_gain = 0.005;

    double linear_velocity = f_net.x * linear_gain;
    double angular_velocity = f_net.y * angular_gain; // + angular is left

    printf("Net Force: (x=%.2f, y=%.2f) ==> Commands: (LinVel=%.4f, AngVel=%.4f)\n", 
            f_net.x, f_net.y, linear_velocity, angular_velocity);
}


sl_result process_scan(ILidarDriver * drv)
{
    sl_result ans;
    
	sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);

    ans = drv->grabScanDataHq(nodes, count, 0);

    if (SL_IS_OK(ans)) {
        drv->ascendScanData(nodes, count);

        std::string scan_line = "SCAN:";
        for (int pos = 0; pos < (int)count; ++pos) {
            double angle_deg = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
            double distance_mm = nodes[pos].dist_mm_q2 / 4.0f;
            
            if (distance_mm > 0.0) {
                scan_line += std::to_string(angle_deg) + "," + std::to_string(distance_mm) + ";";
            }
        }
        printf("%s\n", scan_line.c_str());

        // Get UWB data
        ForceVector user_pos = get_user_position_mm();

        // Calculate Attractive Force
        ForceVector f_attr = attractive_force(user_pos, 0.5);

        // Calculate Repulsive Force
        ForceVector f_rep = repulsive_force(nodes, count);

        // Add forces
        ForceVector f_net;
        f_net.x = f_attr.x + f_rep.x;
        f_net.y = f_attr.y + f_rep.y;

        // Send to Motors
        send_motor_commands(f_net);
        
    } else {
        printf("error code: %x\n", ans);
    }

    return ans;
}


void print_usage(int argc, const char * argv[]) {
           printf("Simple LIDAR data grabber for SLAMTEC LIDAR.\n"
           "Version:  %s \n"
           "Usage:\n"
           " For serial channel %s --channel --serial <com port> [baudrate]\n"
           " The baudrate used by different models is as follows:\n"
           "  A1(115200),A2M7(256000),A2M8(115200),A2M12(256000),"
           "A3(256000),S1(256000),S2(1000000),S3(1000000)\n"
		   " For udp channel %s --channel --udp <ipaddr> [port NO.]\n"
           "The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089. Please refer to the datasheet for details.\n"
           , SL_LIDAR_SDK_VERSION,  argv[0], argv[0]);
}


int main(int argc, const char * argv[]) {
	const char *opt_channel = NULL;
    const char *opt_channel_param_first = NULL;
    sl_u32      opt_channel_param_second = 0;
    sl_result   op_result;
	int         opt_channel_type = CHANNEL_TYPE_SERIALPORT;

    IChannel* _channel;

    if (argc < 5) {
        print_usage(argc, argv);
        return -1;
    }

	const char * opt_is_channel = argv[1];
	if(strcmp(opt_is_channel, "--channel")==0)
	{
		opt_channel = argv[2];
		if(strcmp(opt_channel, "-s")==0||strcmp(opt_channel, "--serial")==0)
		{
			opt_channel_param_first = argv[3];
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);
		}
		else if(strcmp(opt_channel, "-u")==0||strcmp(opt_channel, "--udp")==0)
		{
			opt_channel_param_first = argv[3];
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);
			opt_channel_type = CHANNEL_TYPE_UDP;
		}
		else
		{
			print_usage(argc, argv);
			return -1;
		}
	}
    else
	{
		print_usage(argc, argv);
		return -1;
	}

	ILidarDriver * drv = *createLidarDriver();
    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    sl_lidar_response_device_health_t healthinfo;
    sl_lidar_response_device_info_t devinfo;

    if (opt_channel_type == CHANNEL_TYPE_SERIALPORT) {
        _channel = (*createSerialPortChannel(opt_channel_param_first, opt_channel_param_second));
    }
    else if (opt_channel_type == CHANNEL_TYPE_UDP) {
        _channel = *createUdpChannel(opt_channel_param_first, opt_channel_param_second);
    }
    
    if (SL_IS_FAIL((drv)->connect(_channel))) {
        switch (opt_channel_type) {	
            case CHANNEL_TYPE_SERIALPORT:
                fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
                    , opt_channel_param_first);
                break;
            case CHANNEL_TYPE_UDP:
                fprintf(stderr, "Error, cannot connect to the ip addr %s with the udp port %u.\n"
                    , opt_channel_param_first, opt_channel_param_second);
                break;
        }
        if(drv) {
            delete drv;
            drv = NULL;
        }
        return -1;
    }

    op_result = drv->getDeviceInfo(devinfo);
    if (SL_IS_FAIL(op_result)) {
        fprintf(stderr, "Error, operation time out or unexpected error: %x\n", op_result);
        if(drv) {
            delete drv;
            drv = NULL;
        }
        return -1;
    }

    printf("SLAMTEC LIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }
    printf("\n"
            "Version:  %s \n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , "SL_LIDAR_SDK_VERSION"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);

    op_result = drv->getHealth(healthinfo);
    if (SL_IS_OK(op_result)) {
        printf("Lidar health status : ");
        switch (healthinfo.status) {
            case SL_LIDAR_STATUS_OK: printf("OK."); break;
            case SL_LIDAR_STATUS_WARNING: printf("Warning."); break;
            case SL_LIDAR_STATUS_ERROR: printf("Error."); break;
        }
        printf(" (errorcode: %d)\n", healthinfo.error_code);
    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        if(drv) {
            delete drv;
            drv = NULL;
        }
        return -1;
    }

    if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
        fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
        if(drv) {
            delete drv;
            drv = NULL;
        }
        return -1;
    }

    switch (opt_channel_type) {	
        case CHANNEL_TYPE_SERIALPORT:
            drv->setMotorSpeed();
        break;
    }

    if (SL_IS_FAIL(drv->startScan( 0,1 ))) {
        fprintf(stderr, "Error, cannot start the scan operation.\n");
        if(drv) {
            delete drv;
            drv = NULL;
        }
        return -1;
    }

    printf("Waiting for LiDAR to spin up...\n");
    delay(2000);

    printf("Starting potential field navigation... Press Ctrl+C to stop.\n");
    setbuf(stdout, NULL); // REMOVE IF NOT VISUALIZING
    while (true) {
        if (SL_IS_FAIL(process_scan(drv))) {
            fprintf(stderr, "Error, cannot grab scan data.\l\n");
        }
        delay(200); 
    }

    drv->stop();
    switch (opt_channel_type) {	
        case CHANNEL_TYPE_SERIALPORT:
            delay(20);
            drv->setMotorSpeed(0);
        break;
    }
    if(drv) {
        delete drv;
        drv = NULL;
}
    return 0;
}
