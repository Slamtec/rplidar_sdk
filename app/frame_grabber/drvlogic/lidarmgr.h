/*
 *  SLAMTEC LIDAR
 *  Win32 Demo Application
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

#pragma once
#include "common.h"
#include "..\..\sdk\src\hal\locker.h"
#include "..\..\sdk\src\rplidar_driver_broadcast.h"
#include <vector>
#include <string>

class LidarMgr {

public:
    static LidarMgr & GetInstance();

    LidarMgr();
    ~LidarMgr();
    bool onConnect(const char * port, int baudrate);
    bool onConnectTcp(const char * ipStr, sl_u32 port, sl_u32 flag = 0);
	bool onConnectUdp(const char * ipStr, sl_u32 port, sl_u32 flag = 0);
    bool isConnected() const { return _isConnected; }
    void onDisconnect();
    bool checkDeviceHealth(int * errorCode = NULL);

    // Broadcast discovery support
    bool initBroadcastDriver();
    void shutdownBroadcastDriver();
    bool isBroadcastDriverInitialized() const { return _broadcastDriver != NULL; }

    sl_result discoverDevicesViaBroadcast(
        std::vector<rplidar_response_broadcast_device_info_t>& devices,
        sl_u32 timeout_ms = 1000);

    sl_result setDeviceIpViaBroadcast(
        const rplidar_payload_ip_set_related_t& ipConfig,
        const sl_u8 serialNum[16],
        sl_u32 timeout_ms = 2000);

public:
    sl_lidar_response_device_info_t devinfo;
    static ILidarDriver * lidar_drv;
    IChannel* _channel;
protected:
    static LidarMgr * g_instance;
    static rp::hal::Locker g_oplocker;
    bool  _isConnected;

    // Broadcast driver
    rp::standalone::rplidar::RPLidarBroadcastDriver* _broadcastDriver;
};