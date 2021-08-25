/*
 *  SLAMTEC LIDAR
 *  Demo for Changing the  Baudrate of a Serial Port Communication based LIDAR
 *
 *  Notice:
 *  1. This demo requires the LIDAR  to support the baudrate negotiation mechanism to work
 *    For A-series LIDARs, the firmware version must be higher or equal to 1.30
 *
 *  2. The USB Adaptor Board included in the development kit can NOT be used unless the target baudrate will be 115200bps or 256000bps.
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
#include <signal.h>
#include <string.h>

#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(sl_word_size_t ms) {
    while (ms >= 1000) {
        usleep(1000 * 1000);
        ms -= 1000;
    };
    if (ms != 0)
        usleep(ms * 1000);
}
#endif

using namespace sl;

void print_usage(int argc, const char* argv[])
{
    printf("RPLIDAR Baudrate Negotiation Demo.\n"
        "Version: %s \n"
        "Usage:\n"
        " %s <com port> [baudrate]\n"
        " The baudrate can be ANY possible values between 115200 to 512000.\n"

        , "SL_LIDAR_SDK_VERSION", argv[0], argv[0]);
}


int main(int argc, const char* argv[]) {

    const char* opt_port_dev;
    sl_u32         opt_required_baudrate = 460000;
    sl_result     op_result;


    bool useArgcBaudrate = false;

    IChannel* _channel;

    printf("Baudrate negotiation demo for SLAMTEC LIDAR.\n");


    if (argc > 1)
    {
        opt_port_dev = argv[1];
    }
    else
    {
        print_usage(argc, argv);
        return -1;
    }

    if (argc > 2)
    {
        opt_required_baudrate = strtoul(argv[2], NULL, 10);
        if (opt_required_baudrate < 115200 || opt_required_baudrate > 512000)
        {
            fprintf(stderr, "The baudrate specified is out of the range of [115200, 512000] ");
            return -1;
        }
    }

    // create the driver instance
    ILidarDriver* drv = *createLidarDriver();

    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }

    printf("Try to establish communication to the LIDAR using the baudrate at %s@%d ...\n", opt_port_dev, opt_required_baudrate);

    _channel = (*createSerialPortChannel(opt_port_dev, opt_required_baudrate));

    int result = 0;
    do {

        if (SL_IS_FAIL((drv)->connect(_channel))) {
            fprintf(stderr, "Error, failed to bind to the serial port.\n");
            result = -2;
            break;
        }


        // the same baudrate value must be used here 
        sl_u32 baudrateDetected;
        if (SL_IS_FAIL((drv)->negotiateSerialBaudRate(opt_required_baudrate, &baudrateDetected)))
        {
            fprintf(stderr, "Error, cannot perform baudrate negotiation.\n");
            result = -3;
            break;
        }

        float bpsError = abs((int)baudrateDetected - (int)opt_required_baudrate) * 100.0f / opt_required_baudrate;
        printf("The baudrate detected by the pair is %d bps. Error: %.3f %%\n", baudrateDetected, bpsError);

        if (bpsError > 5.0) {
            printf("Warning, actual BPS error is too large. Communication based on this baudrate may fail.\n");

        }


        if (SL_IS_FAIL((drv)->connect(_channel))) { //reconnect, otherwise, corrupted data will be retrieved.
            fprintf(stderr, "Error, failed to bind to the serial port.\n");
            result = -2;
            break;
        }

        // try to perform an actual communication
        {
            sl_lidar_response_device_info_t devinfo;

            op_result = (drv)->getDeviceInfo(devinfo);

            if (SL_IS_FAIL(op_result)) {
                fprintf(stderr, "Neogiation failure, the new baudrate cannot be used.\n");
                result = -4;
                break;
            }

            // print out the device serial number, firmware and hardware version number..
            printf("Wow, we just communicate with the LIDAR using non-standard baudrate : %d!.\n", opt_required_baudrate);

            printf("SLAMTEC LIDAR S/N: ");
            for (int pos = 0; pos < 16; ++pos) {
                printf("%02X", devinfo.serialnum[pos]);
            }

            printf("\n"
                "Firmware Ver: %d.%02d\n"
                "Hardware Rev: %d\n"
                , devinfo.firmware_version >> 8
                , devinfo.firmware_version & 0xFF
                , (int)devinfo.hardware_version);

        }

    } while (0);

    delete drv;
    delete _channel;
    return 0;
}

