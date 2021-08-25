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

#include "stdafx.h"
#include "lidarmgr.h"
#include "sl_lidar_driver.h"
#ifdef _DEBUG
#include <assert.h>
#else
#define assert(_Expression) void(_Expression)
#endif

LidarMgr * LidarMgr::g_instance = NULL;
rp::hal::Locker LidarMgr::g_oplocker;
ILidarDriver * LidarMgr::lidar_drv = NULL;

LidarMgr & LidarMgr::GetInstance()
{
    if (g_instance) return *g_instance;
    rp::hal::AutoLocker l(g_oplocker);

    if (g_instance) return *g_instance;
    g_instance = new LidarMgr();
    return *g_instance;
}

LidarMgr::LidarMgr()
    : _isConnected(false)
{

}

LidarMgr::~LidarMgr()
{
    rp::hal::AutoLocker l(g_oplocker);
    onDisconnect();
    delete g_instance;
    g_instance = NULL;
    delete lidar_drv;
    lidar_drv = NULL;
}

void LidarMgr::onDisconnect()
{
    if (_isConnected) {
        lidar_drv->stop();
    }
    _isConnected = false;
    delete lidar_drv;
    lidar_drv = NULL;
}

bool  LidarMgr::checkDeviceHealth(int * errorCode)
{
  
    int errcode = 0;
    bool ans = false;

    do {
        if (!_isConnected) {
            errcode = SL_RESULT_OPERATION_FAIL;
            break;
        }

        sl_lidar_response_device_health_t healthinfo;
        if (IS_FAIL(lidar_drv->getHealth(healthinfo))) {
            errcode = SL_RESULT_OPERATION_FAIL;
            break;
        }

        if (healthinfo.status != SL_LIDAR_STATUS_OK) {
            errcode = healthinfo.error_code;
            break;
        }

        ans = true;
    } while(0);

    if (errorCode) *errorCode = errcode;
    return ans;
}

bool LidarMgr::onConnect(const char * port, int baudrate)
{
    if (_isConnected) return true;

    _channel = (*createSerialPortChannel(port, baudrate));

    if (!lidar_drv)
        lidar_drv = *createLidarDriver();

    if (!(bool)lidar_drv) return SL_RESULT_OPERATION_FAIL;

    sl_result ans =(lidar_drv)->connect(_channel);
    
    if (SL_IS_FAIL(ans)) {
        return false;
    }
    // retrieve the devinfo
    ans = lidar_drv->getDeviceInfo(devinfo);

    if (SL_IS_FAIL(ans)) {
        return false;
    }

    _isConnected = true;
    return true;
}

bool LidarMgr::onConnectTcp(const char * ipStr, sl_u32 port, sl_u32 flag)
{
    if (_isConnected) return true;

    _channel = *createTcpChannel(ipStr, port);

    if (!lidar_drv)
        lidar_drv = *createLidarDriver();

    if (!(bool)lidar_drv) return SL_RESULT_OPERATION_FAIL;
    
    sl_result ans =(lidar_drv)->connect(_channel);

    if (SL_IS_FAIL(ans)) {
        return false;
    }
     // retrieve the devinfo
    ans = lidar_drv->getDeviceInfo(devinfo);

    if (SL_IS_FAIL(ans)) {
        return false;
    }

    _isConnected = true;
    return true;
}

bool LidarMgr::onConnectUdp(const char * ipStr, sl_u32 port, sl_u32 flag)
{
	if (_isConnected) return true;

    _channel = *createUdpChannel(ipStr, port);

    if (!lidar_drv)
        lidar_drv = *createLidarDriver();

    if (!(bool)lidar_drv) return SL_RESULT_OPERATION_FAIL;
    
    sl_result ans =(lidar_drv)->connect(_channel);

    if (SL_IS_FAIL(ans)) {
        return false;
    }
     // retrieve the devinfo
    ans = lidar_drv->getDeviceInfo(devinfo);

    if (SL_IS_FAIL(ans)) {
        return false;
    }

    _isConnected = true;
    return true;
}