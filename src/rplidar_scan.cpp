#include "rplidar_scan.h"

#define LOCAL_BUFFER_SIZE 256

// Bunch o' dirty global variables - pls no blame me i hate it as much as you

// some cheeky globals for processing the scan
// sl_lidar_response_measurement_node_t      LOCAL_BUF[LOCAL_BUFFER_SIZE];
// sl_lidar_response_measurement_node_hq_t   LOCAL_SCAN[MAX_SCAN_NODES];

sl_lidar_response_measurement_node_hq_t          LOCAL_BUF_HQ[LOCAL_BUFFER_SIZE];
sl_lidar_response_measurement_node_hq_t          LOCAL_SCAN_HQ[MAX_SCAN_NODES];
size_t                                           LOCAL_COUNT = LOCAL_BUFFER_SIZE;
size_t                                           LOCAL_SCAN_COUNT = 0;

sl_lidar_response_capsule_measurement_nodes_t          capsule_node;
sl_lidar_response_hq_capsule_measurement_nodes_t       hq_node;
sl_lidar_response_ultra_capsule_measurement_nodes_t    ultra_capsule_node;


// cheeky globals for storing the scan results
sl_lidar_response_measurement_node_hq_t   _cached_scan_node_hq_buf[RPLIDAR_BUFFER_SIZE];
size_t                                    _cached_scan_node_hq_count;
sl_u8                                     _cached_capsule_flag;

// sl_lidar_response_measurement_node_hq_t   _cached_scan_node_hq_buf_for_interval_retrieve[RPLIDAR_BUFFER_SIZE];
// size_t                                    _cached_scan_node_hq_count_for_interval_retrieve;


sl_lidar_response_capsule_measurement_nodes_t       _cached_previous_capsuledata;
sl_lidar_response_dense_capsule_measurement_nodes_t _cached_previous_dense_capsuledata;
sl_lidar_response_ultra_capsule_measurement_nodes_t _cached_previous_ultracapsuledata;
sl_lidar_response_hq_capsule_measurement_nodes_t    _cached_previous_Hqdata;

bool                                         _is_previous_capsuledataRdy;
bool                                         _is_previous_HqdataRdy;

rplidar_scan_cache SCAN_CACHE_TYPE = CACHE_SCAN_DATA;

// yucky global variables controlling lidar scan and communication
HardwareSerial * _channel = NULL;
bool _isConnected = false;
bool _isScanning = false;

// internal sync of lidar buffer
bool _scan_node_synced = false;

// Task handle for rplidar_scan_task to allow for deletion and creation
TaskHandle_t RPLidarScanTaskHandle = NULL;
EventGroupHandle_t RPLidarScanEventGroup = NULL;
SemaphoreHandle_t lidarLock = NULL;

using namespace sl;


uint32_t getms() {
    return esp_timer_get_time() / 1000;
}

bool waitForData(HardwareSerial* serial, size_t size, sl_u32 timeoutInMs, size_t* actualReady) {
    sl_u32 startTs = getms();
    size_t bytesReady;
    
    // wait timeout (-1 for forever)
    while (timeoutInMs == -1 || (getms() - startTs) <= timeoutInMs) {
        // wait for total bytes to be ready
        if ((bytesReady = serial->available()) < size) {
            // Serial.printf("%d / %d bytes ready\n", bytesReady, size);
            delay(1);
            continue;
        }

        // check for disconnection
        if (!_isConnected) {
            return false;
        }
        // pls no null pointer schenanigans
        if (actualReady != nullptr) {
            *actualReady = bytesReady;
        }
        return true;
    }
    Serial.printf("waitForData - TIMED OUT - %d / %d bytes ready\n", bytesReady, size);
    return false;
}


static void convert(const sl_lidar_response_measurement_node_t& from, sl_lidar_response_measurement_node_hq_t& to)
{
    to.angle_z_q14 = (((from.angle_q6_checkbit) >> SL_LIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) << 8) / 90;  //transfer to q14 Z-angle
    to.dist_mm_q2 = from.distance_q2;
    to.flag = (from.sync_quality & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT);  // trasfer syncbit to HQ flag field
    to.quality = (from.sync_quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) << SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;  //remove the last two bits and then make quality from 0-63 to 0-255
}

static void convert(const sl_lidar_response_measurement_node_hq_t& from, sl_lidar_response_measurement_node_t& to)
{
    to.sync_quality = (from.flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT) | ((from.quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) << SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
    to.angle_q6_checkbit = 1 | (((from.angle_z_q14 * 90) >> 8) << SL_LIDAR_RESP_MEASUREMENT_ANGLE_SHIFT);
    to.distance_q2 = from.dist_mm_q2 > sl_u16(-1) ? sl_u16(0) : sl_u16(from.dist_mm_q2);
}

static sl_u32 _varbitscale_decode(sl_u32 scaled, sl_u32 & scaleLevel)
{
    static const sl_u32 VBS_SCALED_BASE[] = {
        SL_LIDAR_VARBITSCALE_X16_DEST_VAL,
        SL_LIDAR_VARBITSCALE_X8_DEST_VAL,
        SL_LIDAR_VARBITSCALE_X4_DEST_VAL,
        SL_LIDAR_VARBITSCALE_X2_DEST_VAL,
        0,
    };

    static const sl_u32 VBS_SCALED_LVL[] = {
        4,
        3,
        2,
        1,
        0,
    };

    static const sl_u32 VBS_TARGET_BASE[] = {
        (0x1 << SL_LIDAR_VARBITSCALE_X16_SRC_BIT),
        (0x1 << SL_LIDAR_VARBITSCALE_X8_SRC_BIT),
        (0x1 << SL_LIDAR_VARBITSCALE_X4_SRC_BIT),
        (0x1 << SL_LIDAR_VARBITSCALE_X2_SRC_BIT),
        0,
    };

    for (size_t i = 0; i < _countof(VBS_SCALED_BASE); ++i) {
        int remain = ((int)scaled - (int)VBS_SCALED_BASE[i]);
        if (remain >= 0) {
            scaleLevel = VBS_SCALED_LVL[i];
            return VBS_TARGET_BASE[i] + (remain << scaleLevel);
        }
    }
    return 0;
}


sl_result _waitNode(sl_lidar_response_measurement_node_t * node, sl_u32 timeout = DEFAULT_TIMEOUT)
{
    int  recvPos = 0;
    sl_u32 startTs = getms();
    sl_u8  recvBuffer[sizeof(sl_lidar_response_measurement_node_t)];
    sl_u8 *nodeBuffer = (sl_u8*)node;
    sl_u32 waitTime;

    while ((waitTime = getms() - startTs) <= timeout) {
        size_t remainSize = sizeof(sl_lidar_response_measurement_node_t) - recvPos;
        size_t recvSize;

        bool ans = waitForData(_channel, remainSize, timeout - waitTime, &recvSize);
        if (!ans) return SL_RESULT_OPERATION_FAIL;

        if (recvSize > remainSize) recvSize = remainSize;

        recvSize = _channel->readBytes(recvBuffer, recvSize);

        for (size_t pos = 0; pos < recvSize; ++pos) {
            sl_u8 currentByte = recvBuffer[pos];
            switch (recvPos) {
            case 0: // expect the sync bit and its reverse in this byte
            {
                sl_u8 tmp = (currentByte >> 1);
                if ((tmp ^ currentByte) & 0x1) {
                    // pass
                }
                else {
                    continue;
                }

            }
            break;
            case 1: // expect the highest bit to be 1
            {
                if (currentByte & SL_LIDAR_RESP_MEASUREMENT_CHECKBIT) {
                    // pass
                }
                else {
                    recvPos = 0;
                    continue;
                }
            }
            break;
            }
            nodeBuffer[recvPos++] = currentByte;

            if (recvPos == sizeof(sl_lidar_response_measurement_node_t)) {
                return SL_RESULT_OK;
            }
        }
    }

    return SL_RESULT_OPERATION_TIMEOUT;
}

sl_result _waitScanData(sl_lidar_response_measurement_node_t * nodebuffer, size_t & count, sl_u32 timeout = DEFAULT_TIMEOUT)
{
    if (!_isConnected) {
        count = 0;
        return SL_RESULT_OPERATION_FAIL;
    }

    size_t   recvNodeCount = 0;
    sl_u32     startTs = getms();
    sl_u32     waitTime;
    Result<nullptr_t> ans = SL_RESULT_OK;

    while ((waitTime = getms() - startTs) <= timeout && recvNodeCount < count) {
        sl_lidar_response_measurement_node_t node;
        ans = _waitNode(&node, timeout - waitTime);
        if (!ans) return ans;

        nodebuffer[recvNodeCount++] = node;

        if (recvNodeCount == count) return SL_RESULT_OK;
    }
    count = recvNodeCount;
    return SL_RESULT_OPERATION_TIMEOUT;
}

sl_result _cacheScanData()
{

    // sl_lidar_response_measurement_node_t      local_buf[LOCAL_BUFFER_SIZE];
    // size_t                                   count = LOCAL_BUFFER_SIZE;
    // sl_lidar_response_measurement_node_hq_t   local_scan[MAX_SCAN_NODES];
    // size_t                                   scan_count = 0;
    Result<nullptr_t>                        ans = SL_RESULT_OK;
    // memset(local_scan, 0, sizeof(local_scan));

    // _waitScanData(local_buf, count); // // always discard the first data since it may be incomplete

    // ans = _waitScanData(LOCAL_BUF, LOCAL_COUNT);

    // if (!ans) {
    //     if ((sl_result)ans != SL_RESULT_OPERATION_TIMEOUT) {
    //         return SL_RESULT_OPERATION_FAIL;
    //     }
    // }

    // for (size_t pos = 0; pos < LOCAL_COUNT; ++pos) {
    //     if (LOCAL_BUF[pos].sync_quality & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT) {
    //         // only publish the data when it contains a full 360 degree scan 

    //         if ((LOCAL_SCAN[0].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT)) {
    //             if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
    //                 memcpy(_cached_scan_node_hq_buf, LOCAL_SCAN, LOCAL_SCAN_COUNT * sizeof(sl_lidar_response_measurement_node_hq_t));
    //                 _cached_scan_node_hq_count = LOCAL_SCAN_COUNT;
    //                 xEventGroupSetBits(RPLidarScanEventGroup, EVT_RPLIDAR_SCAN_COMPLETE);
    //                 xSemaphoreGive(lidarLock);
    //             } else {
    //                 // timeout
    //                 ;
    //             }
    //         }
    //         LOCAL_SCAN_COUNT = 0;
    //     }

    //     sl_lidar_response_measurement_node_hq_t nodeHq;
    //     convert(LOCAL_BUF[pos], nodeHq);
    //     LOCAL_SCAN[LOCAL_SCAN_COUNT++] = nodeHq;

    //     if (LOCAL_SCAN_COUNT == _countof(LOCAL_SCAN)) {
    //         LOCAL_SCAN_COUNT -= 1; // prevent overflow
    //     }
    //     //for interval retrieve
    //     if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
    //         _cached_scan_node_hq_buf_for_interval_retrieve[_cached_scan_node_hq_count_for_interval_retrieve++] = nodeHq;
    //         if (_cached_scan_node_hq_count_for_interval_retrieve == _countof(_cached_scan_node_hq_buf_for_interval_retrieve)) _cached_scan_node_hq_count_for_interval_retrieve -= 1; // prevent overflow
    //         xSemaphoreGive(lidarLock);
    //     }
    // }
    
    return SL_RESULT_OK;
}

void _ultraCapsuleToNormal(const sl_lidar_response_ultra_capsule_measurement_nodes_t & capsule, sl_lidar_response_measurement_node_hq_t *nodebuffer, size_t &nodeCount)
{
    nodeCount = 0;
    if (_is_previous_capsuledataRdy) {
        int diffAngle_q8;
        int currentStartAngle_q8 = ((capsule.start_angle_sync_q6 & 0x7FFF) << 2);
        int prevStartAngle_q8 = ((_cached_previous_ultracapsuledata.start_angle_sync_q6 & 0x7FFF) << 2);

        diffAngle_q8 = (currentStartAngle_q8)-(prevStartAngle_q8);
        if (prevStartAngle_q8 > currentStartAngle_q8) {
            diffAngle_q8 += (360 << 8);
        }

        int angleInc_q16 = (diffAngle_q8 << 3) / 3;
        int currentAngle_raw_q16 = (prevStartAngle_q8 << 8);
        for (size_t pos = 0; pos < _countof(_cached_previous_ultracapsuledata.ultra_cabins); ++pos) {
            int dist_q2[3];
            int angle_q6[3];
            int syncBit[3];


            sl_u32 combined_x3 = _cached_previous_ultracapsuledata.ultra_cabins[pos].combined_x3;

            // unpack ...
            int dist_major = (combined_x3 & 0xFFF);

            // signed partical integer, using the magic shift here
            // DO NOT TOUCH

            int dist_predict1 = (((int)(combined_x3 << 10)) >> 22);
            int dist_predict2 = (((int)combined_x3) >> 22);

            int dist_major2;

            sl_u32 scalelvl1, scalelvl2;

            // prefetch next ...
            if (pos == _countof(_cached_previous_ultracapsuledata.ultra_cabins) - 1) {
                dist_major2 = (capsule.ultra_cabins[0].combined_x3 & 0xFFF);
            }
            else {
                dist_major2 = (_cached_previous_ultracapsuledata.ultra_cabins[pos + 1].combined_x3 & 0xFFF);
            }

            // decode with the var bit scale ...
            dist_major = _varbitscale_decode(dist_major, scalelvl1);
            dist_major2 = _varbitscale_decode(dist_major2, scalelvl2);


            int dist_base1 = dist_major;
            int dist_base2 = dist_major2;

            if ((!dist_major) && dist_major2) {
                dist_base1 = dist_major2;
                scalelvl1 = scalelvl2;
            }


            dist_q2[0] = (dist_major << 2);
            if ((dist_predict1 == 0xFFFFFE00) || (dist_predict1 == 0x1FF)) {
                dist_q2[1] = 0;
            }
            else {
                dist_predict1 = (dist_predict1 << scalelvl1);
                dist_q2[1] = (dist_predict1 + dist_base1) << 2;

            }

            if ((dist_predict2 == 0xFFFFFE00) || (dist_predict2 == 0x1FF)) {
                dist_q2[2] = 0;
            }
            else {
                dist_predict2 = (dist_predict2 << scalelvl2);
                dist_q2[2] = (dist_predict2 + dist_base2) << 2;
            }


            for (int cpos = 0; cpos < 3; ++cpos) {
                syncBit[cpos] = (((currentAngle_raw_q16 + angleInc_q16) % (360 << 16)) < angleInc_q16) ? 1 : 0;

                int offsetAngleMean_q16 = (int)(7.5 * 3.1415926535 * (1 << 16) / 180.0);

                if (dist_q2[cpos] >= (50 * 4))
                {
                    const int k1 = 98361;
                    const int k2 = int(k1 / dist_q2[cpos]);

                    offsetAngleMean_q16 = (int)(8 * 3.1415926535 * (1 << 16) / 180) - (k2 << 6) - (k2 * k2 * k2) / 98304;
                }

                angle_q6[cpos] = ((currentAngle_raw_q16 - int(offsetAngleMean_q16 * 180 / 3.14159265)) >> 10);
                currentAngle_raw_q16 += angleInc_q16;

                if (angle_q6[cpos] < 0) angle_q6[cpos] += (360 << 6);
                if (angle_q6[cpos] >= (360 << 6)) angle_q6[cpos] -= (360 << 6);

                sl_lidar_response_measurement_node_hq_t node;

                node.flag = (syncBit[cpos] | ((!syncBit[cpos]) << 1));
                node.quality = dist_q2[cpos] ? (0x2F << SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) : 0;
                node.angle_z_q14 = sl_u16((angle_q6[cpos] << 8) / 90);
                node.dist_mm_q2 = dist_q2[cpos];

                nodebuffer[nodeCount++] = node;
            }

        }
    }

    _cached_previous_ultracapsuledata = capsule;
    _is_previous_capsuledataRdy = true;
}

sl_result _waitCapsuledNode(sl_lidar_response_capsule_measurement_nodes_t & node, sl_u32 timeout = DEFAULT_TIMEOUT)
{
    int  recvPos = 0;
    sl_u32 startTs = getms();
    sl_u8  recvBuffer[sizeof(sl_lidar_response_capsule_measurement_nodes_t)];
    sl_u8 *nodeBuffer = (sl_u8*)&node;
    sl_u32 waitTime;
    while ((waitTime = getms() - startTs) <= timeout) {
        size_t remainSize = sizeof(sl_lidar_response_capsule_measurement_nodes_t) - recvPos;
        size_t recvSize;
        bool ans = waitForData(_channel, remainSize, timeout - waitTime, &recvSize);
        if (!ans) return SL_RESULT_OPERATION_TIMEOUT;

        if (recvSize > remainSize) recvSize = remainSize;
        recvSize = _channel->readBytes(recvBuffer, recvSize);

        for (size_t pos = 0; pos < recvSize; ++pos) {
            sl_u8 currentByte = recvBuffer[pos];

            switch (recvPos) {
            case 0: // expect the sync bit 1
            {
                sl_u8 tmp = (currentByte >> 4);
                if (tmp == SL_LIDAR_RESP_MEASUREMENT_EXP_SYNC_1) {
                    // pass
                }
                else {
                    _is_previous_capsuledataRdy = false;
                    continue;
                }

            }
            break;
            case 1: // expect the sync bit 2
            {
                sl_u8 tmp = (currentByte >> 4);
                if (tmp == SL_LIDAR_RESP_MEASUREMENT_EXP_SYNC_2) {
                    // pass
                }
                else {
                    recvPos = 0;
                    _is_previous_capsuledataRdy = false;
                    continue;
                }
            }
            break;
            }
            nodeBuffer[recvPos++] = currentByte;
            if (recvPos == sizeof(sl_lidar_response_capsule_measurement_nodes_t)) {
                // calc the checksum ...
                sl_u8 checksum = 0;
                sl_u8 recvChecksum = ((node.s_checksum_1 & 0xF) | (node.s_checksum_2 << 4));
                for (size_t cpos = offsetof(sl_lidar_response_capsule_measurement_nodes_t, start_angle_sync_q6);
                    cpos < sizeof(sl_lidar_response_capsule_measurement_nodes_t); ++cpos)
                {
                    checksum ^= nodeBuffer[cpos];
                }
                if (recvChecksum == checksum) {
                    // only consider vaild if the checksum matches...
                    if (node.start_angle_sync_q6 & SL_LIDAR_RESP_MEASUREMENT_EXP_SYNCBIT) {
                        // this is the first capsule frame in logic, discard the previous cached data...
                        _scan_node_synced = false;
                        _is_previous_capsuledataRdy = false;
                        return SL_RESULT_OK;
                    }
                    return SL_RESULT_OK;
                }
                _is_previous_capsuledataRdy = false;
                return SL_RESULT_INVALID_DATA;
            }
        }
    }
    _is_previous_capsuledataRdy = false;
    return SL_RESULT_OPERATION_TIMEOUT;
}
void _capsuleToNormal(const sl_lidar_response_capsule_measurement_nodes_t & capsule, sl_lidar_response_measurement_node_hq_t *nodebuffer, size_t &nodeCount)
{
    nodeCount = 0;
    if (_is_previous_capsuledataRdy) {
        int diffAngle_q8;
        int currentStartAngle_q8 = ((capsule.start_angle_sync_q6 & 0x7FFF) << 2);
        int prevStartAngle_q8 = ((_cached_previous_capsuledata.start_angle_sync_q6 & 0x7FFF) << 2);

        diffAngle_q8 = (currentStartAngle_q8)-(prevStartAngle_q8);
        if (prevStartAngle_q8 > currentStartAngle_q8) {
            diffAngle_q8 += (360 << 8);
        }

        int angleInc_q16 = (diffAngle_q8 << 3);
        int currentAngle_raw_q16 = (prevStartAngle_q8 << 8);
        for (size_t pos = 0; pos < _countof(_cached_previous_capsuledata.cabins); ++pos) {
            int dist_q2[2];
            int angle_q6[2];
            int syncBit[2];

            dist_q2[0] = (_cached_previous_capsuledata.cabins[pos].distance_angle_1 & 0xFFFC);
            dist_q2[1] = (_cached_previous_capsuledata.cabins[pos].distance_angle_2 & 0xFFFC);

            int angle_offset1_q3 = ((_cached_previous_capsuledata.cabins[pos].offset_angles_q3 & 0xF) | ((_cached_previous_capsuledata.cabins[pos].distance_angle_1 & 0x3) << 4));
            int angle_offset2_q3 = ((_cached_previous_capsuledata.cabins[pos].offset_angles_q3 >> 4) | ((_cached_previous_capsuledata.cabins[pos].distance_angle_2 & 0x3) << 4));

            angle_q6[0] = ((currentAngle_raw_q16 - (angle_offset1_q3 << 13)) >> 10);
            syncBit[0] = (((currentAngle_raw_q16 + angleInc_q16) % (360 << 16)) < angleInc_q16) ? 1 : 0;
            currentAngle_raw_q16 += angleInc_q16;


            angle_q6[1] = ((currentAngle_raw_q16 - (angle_offset2_q3 << 13)) >> 10);
            syncBit[1] = (((currentAngle_raw_q16 + angleInc_q16) % (360 << 16)) < angleInc_q16) ? 1 : 0;
            currentAngle_raw_q16 += angleInc_q16;

            for (int cpos = 0; cpos < 2; ++cpos) {

                if (angle_q6[cpos] < 0) angle_q6[cpos] += (360 << 6);
                if (angle_q6[cpos] >= (360 << 6)) angle_q6[cpos] -= (360 << 6);

                sl_lidar_response_measurement_node_hq_t node;

                node.angle_z_q14 = sl_u16((angle_q6[cpos] << 8) / 90);
                node.flag = (syncBit[cpos] | ((!syncBit[cpos]) << 1));
                node.quality = dist_q2[cpos] ? (0x2f << SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) : 0;
                node.dist_mm_q2 = dist_q2[cpos];

                nodebuffer[nodeCount++] = node;
            }

        }
    }

    _cached_previous_capsuledata = capsule;
    _is_previous_capsuledataRdy = true;
}

void _dense_capsuleToNormal(const sl_lidar_response_capsule_measurement_nodes_t & capsule, sl_lidar_response_measurement_node_hq_t *nodebuffer, size_t &nodeCount)
{
    static int lastNodeSyncBit = 0;
    const sl_lidar_response_dense_capsule_measurement_nodes_t *dense_capsule = reinterpret_cast<const sl_lidar_response_dense_capsule_measurement_nodes_t*>(&capsule);
    nodeCount = 0;
    if (_is_previous_capsuledataRdy) {
        int diffAngle_q8;
        int currentStartAngle_q8 = ((dense_capsule->start_angle_sync_q6 & 0x7FFF) << 2);
        int prevStartAngle_q8 = ((_cached_previous_dense_capsuledata.start_angle_sync_q6 & 0x7FFF) << 2);

        diffAngle_q8 = (currentStartAngle_q8)-(prevStartAngle_q8);
        if (prevStartAngle_q8 > currentStartAngle_q8) {
            diffAngle_q8 += (360 << 8);
        }

        int angleInc_q16 = (diffAngle_q8 << 8) / 40;
        int currentAngle_raw_q16 = (prevStartAngle_q8 << 8);
        for (size_t pos = 0; pos < _countof(_cached_previous_dense_capsuledata.cabins); ++pos) {
            int dist_q2;
            int angle_q6;
            int syncBit;
            const int dist = static_cast<const int>(_cached_previous_dense_capsuledata.cabins[pos].distance);
            dist_q2 = dist << 2;
            angle_q6 = (currentAngle_raw_q16 >> 10);

            syncBit = (((currentAngle_raw_q16 + angleInc_q16) % (360 << 16)) < (angleInc_q16<<1)) ? 1 : 0;
            syncBit = (syncBit^ lastNodeSyncBit)&syncBit;//Ensure that syncBit is exactly detected
            if (syncBit) {
                _scan_node_synced = true;
            }

            currentAngle_raw_q16 += angleInc_q16;

            if (angle_q6 < 0) angle_q6 += (360 << 6);
            if (angle_q6 >= (360 << 6)) angle_q6 -= (360 << 6);

            
            sl_lidar_response_measurement_node_hq_t node;

            node.angle_z_q14 = sl_u16((angle_q6 << 8) / 90);
            node.flag = (syncBit | ((!syncBit) << 1));
            node.quality = dist_q2 ? (0x2f << SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) : 0;
            node.dist_mm_q2 = dist_q2;
            if(_scan_node_synced)
                nodebuffer[nodeCount++] = node;
            lastNodeSyncBit = syncBit;
        }
    }
    else {
        _scan_node_synced = false;
    }

    _cached_previous_dense_capsuledata = *dense_capsule;
    _is_previous_capsuledataRdy = true;
}

sl_result _cacheCapsuledScanData()
{
    // sl_lidar_response_capsule_measurement_nodes_t    capsule_node;
    // sl_lidar_response_measurement_node_hq_t          LOCAL_BUF_HQ[LOCAL_BUFFER_SIZE];
    // size_t                                           count = LOCAL_BUFFER_SIZE;
    // sl_lidar_response_measurement_node_hq_t          LOCAL_SCAN_HQ[MAX_SCAN_NODES];
    // size_t                                           scan_count = 0;
    // memset(LOCAL_SCAN_HQ, 0, sizeof(LOCAL_SCAN_HQ));

    // _waitCapsuledNode(capsule_node); // // always discard the first data since it may be incomplete

    Result<nullptr_t> ans = SL_RESULT_OK;  
    ans = _waitCapsuledNode(capsule_node);
    if (!ans) {
        if ((sl_result)ans != SL_RESULT_OPERATION_TIMEOUT && (sl_result)ans != SL_RESULT_INVALID_DATA) {
            return SL_RESULT_OPERATION_FAIL;
        }
        else {
            // current data is invalid, do not use it.
            return SL_RESULT_OK;
        }
    }
    switch (_cached_capsule_flag) {
    case NORMAL_CAPSULE:
        _capsuleToNormal(capsule_node, LOCAL_BUF_HQ, LOCAL_COUNT);
        break;
    case DENSE_CAPSULE:
        _dense_capsuleToNormal(capsule_node, LOCAL_BUF_HQ, LOCAL_COUNT);
        break;
    }
    //

    for (size_t pos = 0; pos < LOCAL_COUNT; ++pos) {
        if (LOCAL_BUF_HQ[pos].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT) {
            // only publish the data when it contains a full 360 degree scan 

            if ((LOCAL_SCAN_HQ[0].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT)) {
                if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
                    memcpy(_cached_scan_node_hq_buf, LOCAL_SCAN_HQ, LOCAL_SCAN_COUNT * sizeof(sl_lidar_response_measurement_node_hq_t));
                    _cached_scan_node_hq_count = LOCAL_SCAN_COUNT;
                    xEventGroupSetBits(RPLidarScanEventGroup, EVT_RPLIDAR_SCAN_COMPLETE);

                    xSemaphoreGive(lidarLock);
                } else {
                    // timeout
                    ;
                }
            }
            LOCAL_SCAN_COUNT = 0;
        }
        LOCAL_SCAN_HQ[LOCAL_SCAN_COUNT++] = LOCAL_BUF_HQ[pos];
        if (LOCAL_SCAN_COUNT == _countof(LOCAL_SCAN_HQ)) {
            LOCAL_SCAN_COUNT -= 1; // prevent overflow
        }

        //for interval retrieve
        // if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
        //     _cached_scan_node_hq_buf_for_interval_retrieve[_cached_scan_node_hq_count_for_interval_retrieve++] = LOCAL_BUF_HQ[pos];
        //     if (_cached_scan_node_hq_count_for_interval_retrieve == _countof(_cached_scan_node_hq_buf_for_interval_retrieve)) {
        //         _cached_scan_node_hq_count_for_interval_retrieve -= 1; // prevent overflow
        //     }
        //     xSemaphoreGive(lidarLock);
        // }
    }
    return SL_RESULT_OK;
}

sl_result _waitHqNode(sl_lidar_response_hq_capsule_measurement_nodes_t & node, sl_u32 timeout = DEFAULT_TIMEOUT)
{
    if (!_isConnected) {
        return SL_RESULT_OPERATION_FAIL;
    }

    int  recvPos = 0;
    sl_u32 startTs = getms();
    sl_u8  recvBuffer[sizeof(sl_lidar_response_hq_capsule_measurement_nodes_t)];
    sl_u8 *nodeBuffer = (sl_u8*)&node;
    sl_u32 waitTime;

    while ((waitTime = getms() - startTs) <= timeout) {
        size_t remainSize = sizeof(sl_lidar_response_hq_capsule_measurement_nodes_t) - recvPos;
        size_t recvSize;

        bool ans = waitForData(_channel, remainSize, timeout - waitTime, &recvSize);
        if (!ans){
            return SL_RESULT_OPERATION_TIMEOUT;
        }
        if (recvSize > remainSize) recvSize = remainSize;

        recvSize = _channel->readBytes(recvBuffer, recvSize);

        for (size_t pos = 0; pos < recvSize; ++pos) {
            sl_u8 currentByte = recvBuffer[pos];
            switch (recvPos) {
            case 0: // expect the sync byte
            {
                sl_u8 tmp = (currentByte);
                if (tmp == SL_LIDAR_RESP_MEASUREMENT_HQ_SYNC) {
                    // pass
                }
                else {
                    recvPos = 0;
                    _is_previous_HqdataRdy = false;
                    continue;
                }
            }
            break;
            case sizeof(sl_lidar_response_hq_capsule_measurement_nodes_t) - 1 - 4:
            {

            }
            break;
            case sizeof(sl_lidar_response_hq_capsule_measurement_nodes_t) - 1:
            {

            }
            break;
            }
            nodeBuffer[recvPos++] = currentByte;
            if (recvPos == sizeof(sl_lidar_response_hq_capsule_measurement_nodes_t)) {
                sl_u32 crcCalc2 = crc32::getResult(nodeBuffer, sizeof(sl_lidar_response_hq_capsule_measurement_nodes_t) - 4);

                if (crcCalc2 == node.crc32) {
                    _is_previous_HqdataRdy = true;
                    return SL_RESULT_OK;
                }
                else {
                    _is_previous_HqdataRdy = false;
                    return SL_RESULT_INVALID_DATA;
                }

            }
        }
    }
    _is_previous_HqdataRdy = false;
    return SL_RESULT_OPERATION_TIMEOUT;
}

void _HqToNormal(const sl_lidar_response_hq_capsule_measurement_nodes_t & node_hq, sl_lidar_response_measurement_node_hq_t *nodebuffer, size_t &nodeCount)
{
    nodeCount = 0;
    if (_is_previous_HqdataRdy) {
        for (size_t pos = 0; pos < _countof(_cached_previous_Hqdata.node_hq); ++pos) {
            nodebuffer[nodeCount++] = node_hq.node_hq[pos];
        }
    }
    _cached_previous_Hqdata = node_hq;
    _is_previous_HqdataRdy = true;

}

sl_result _cacheHqScanData()
{
    // sl_lidar_response_hq_capsule_measurement_nodes_t    hq_node;
    // sl_lidar_response_measurement_node_hq_t   local_buf[LOCAL_BUFFER_SIZE];
    // size_t                                   count = LOCAL_BUFFER_SIZE;
    // sl_lidar_response_measurement_node_hq_t   local_scan[MAX_SCAN_NODES];
    // size_t                                   scan_count = 0;
    Result<nullptr_t> ans = SL_RESULT_OK;
    // memset(local_scan, 0, sizeof(local_scan));
    // _waitHqNode(hq_node);
    ans = _waitHqNode(hq_node);
    if (!ans) {
        if ((sl_result)ans != SL_RESULT_OPERATION_TIMEOUT && (sl_result)ans != SL_RESULT_INVALID_DATA) {
            return SL_RESULT_OPERATION_FAIL;
        }
        else {
            // current data is invalid, do not use it.
            return SL_RESULT_OK;
        }
    }

    _HqToNormal(hq_node, LOCAL_BUF_HQ, LOCAL_COUNT);
    for (size_t pos = 0; pos < LOCAL_COUNT; ++pos){
        if (LOCAL_BUF_HQ[pos].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT){
            // only publish the data when it contains a full 360 degree scan 
            if ((LOCAL_SCAN_HQ[0].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT)) {
                if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
                    memcpy(_cached_scan_node_hq_buf, LOCAL_SCAN_HQ, LOCAL_SCAN_COUNT * sizeof(sl_lidar_response_measurement_node_hq_t));
                    _cached_scan_node_hq_count = LOCAL_SCAN_COUNT;
                    xEventGroupSetBits(RPLidarScanEventGroup, EVT_RPLIDAR_SCAN_COMPLETE);

                    xSemaphoreGive(lidarLock);
                } else {
                    // timeout
                    ;
                }
            }
            LOCAL_SCAN_COUNT = 0;
        }
        LOCAL_SCAN_HQ[LOCAL_SCAN_COUNT++] = LOCAL_BUF_HQ[pos];
        if (LOCAL_SCAN_COUNT == _countof(LOCAL_SCAN_HQ)) {
            LOCAL_SCAN_COUNT -= 1; // prevent overflow
        }
                                                                    //for interval retrieve
        // if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
        //     _cached_scan_node_hq_buf_for_interval_retrieve[_cached_scan_node_hq_count_for_interval_retrieve++] = LOCAL_BUF_HQ[pos];
        //     // prevent overflow
        //     if (_cached_scan_node_hq_count_for_interval_retrieve == _countof(_cached_scan_node_hq_buf_for_interval_retrieve)) {
        //         _cached_scan_node_hq_count_for_interval_retrieve -= 1;
        //     }
        //     xSemaphoreGive(lidarLock);
        // }
    }
    return SL_RESULT_OK;
}

sl_result _waitUltraCapsuledNode(sl_lidar_response_ultra_capsule_measurement_nodes_t & node, sl_u32 timeout = DEFAULT_TIMEOUT)
{
    if (!_isConnected) {
        Serial.printf("Not connected\n");
        return SL_RESULT_OPERATION_FAIL;
    }

    int  recvPos = 0;
    sl_u32 startTs = getms();
    sl_u8  recvBuffer[sizeof(sl_lidar_response_ultra_capsule_measurement_nodes_t)];
    sl_u8 *nodeBuffer = (sl_u8*)&node;
    sl_u32 waitTime;

    while ((waitTime = getms() - startTs) <= timeout) {
        size_t remainSize = sizeof(sl_lidar_response_ultra_capsule_measurement_nodes_t) - recvPos;
        size_t recvSize;

        bool ans = waitForData(_channel, remainSize, timeout - waitTime, &recvSize);
        if (!ans) {
            Serial.printf("Scan timed out 1\n");
            return SL_RESULT_OPERATION_TIMEOUT;
        }
        if (recvSize > remainSize) recvSize = remainSize;

        recvSize = _channel->readBytes(recvBuffer, recvSize);

        for (size_t pos = 0; pos < recvSize; ++pos) {
            sl_u8 currentByte = recvBuffer[pos];
            switch (recvPos) {
            case 0: // expect the sync bit 1
            {
                sl_u8 tmp = (currentByte >> 4);
                if (tmp == SL_LIDAR_RESP_MEASUREMENT_EXP_SYNC_1) {
                    // pass
                }
                else {
                    _is_previous_capsuledataRdy = false;
                    continue;
                }
            }
            break;
            case 1: // expect the sync bit 2
            {
                sl_u8 tmp = (currentByte >> 4);
                if (tmp == SL_LIDAR_RESP_MEASUREMENT_EXP_SYNC_2) {
                    // pass
                }
                else {
                    recvPos = 0;
                    _is_previous_capsuledataRdy = false;
                    continue;
                }
            }
            break;
            }
            nodeBuffer[recvPos++] = currentByte;
            if (recvPos == sizeof(sl_lidar_response_ultra_capsule_measurement_nodes_t)) {
                // calc the checksum ...
                sl_u8 checksum = 0;
                sl_u8 recvChecksum = ((node.s_checksum_1 & 0xF) | (node.s_checksum_2 << 4));

                for (size_t cpos = offsetof(sl_lidar_response_ultra_capsule_measurement_nodes_t, start_angle_sync_q6);
                    cpos < sizeof(sl_lidar_response_ultra_capsule_measurement_nodes_t); ++cpos) 
                {
                    checksum ^= nodeBuffer[cpos];
                }

                if (recvChecksum == checksum) {
                    // only consider vaild if the checksum matches...
                    if (node.start_angle_sync_q6 & SL_LIDAR_RESP_MEASUREMENT_EXP_SYNCBIT) {
                        // this is the first capsule frame in logic, discard the previous cached data...
                        _is_previous_capsuledataRdy = false;
                        return SL_RESULT_OK;
                    }
                    return SL_RESULT_OK;
                }
                Serial.printf("Scan returned invalid data\n");
                _is_previous_capsuledataRdy = false;
                return SL_RESULT_INVALID_DATA;
            }
        }
    }
    Serial.printf("Scan timed ou 2\n");
    _is_previous_capsuledataRdy = false;
    return SL_RESULT_OPERATION_TIMEOUT;
}

sl_result _cacheUltraCapsuledScanData()
{
    Result<nullptr_t> ans = SL_RESULT_OK;
    ans = _waitUltraCapsuledNode(ultra_capsule_node);
    if (!ans) {
        Serial.printf("Bad data acquired by scan\n");
        if ((sl_result)ans != SL_RESULT_OPERATION_TIMEOUT && (sl_result)ans != SL_RESULT_INVALID_DATA) {
            Serial.printf("Lidar failed\n");
            return SL_RESULT_OPERATION_FAIL;
        }
        else {
            // current data is invalid, do not use it.
            Serial.printf("Data is invalid with err %d\n", (sl_result)ans);
            return SL_RESULT_OK;
        }
    }

    _ultraCapsuleToNormal(ultra_capsule_node, LOCAL_BUF_HQ, LOCAL_COUNT);

    for (size_t pos = 0; pos < LOCAL_COUNT; ++pos) {
        if (LOCAL_BUF_HQ[pos].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT) {
            // only publish the data when it contains a full 360 degree scan 

            if ((LOCAL_SCAN_HQ[0].flag & SL_LIDAR_RESP_MEASUREMENT_SYNCBIT)) {
                if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
                    memcpy(_cached_scan_node_hq_buf, LOCAL_SCAN_HQ, LOCAL_SCAN_COUNT * sizeof(sl_lidar_response_measurement_node_hq_t));
                    _cached_scan_node_hq_count = LOCAL_SCAN_COUNT;
                    xEventGroupSetBits(RPLidarScanEventGroup, EVT_RPLIDAR_SCAN_COMPLETE);
                    xSemaphoreGive(lidarLock);
                } else {
                    Serial.printf("Lidar scan failed to acquire semaphore\n");
                    // timeout
                    ;
                }
            }
            LOCAL_SCAN_COUNT = 0;
        }
        LOCAL_SCAN_HQ[LOCAL_SCAN_COUNT++] = LOCAL_BUF_HQ[pos];
        if (LOCAL_SCAN_COUNT == _countof(LOCAL_SCAN_HQ)) {
            LOCAL_SCAN_COUNT -= 1; // prevent overflow
        }

        //for interval retrieve
        // if (xSemaphoreTake(lidarLock, LIDAR_TIMEOUT)) {
        //     _cached_scan_node_hq_buf_for_interval_retrieve[_cached_scan_node_hq_count_for_interval_retrieve++] = LOCAL_BUF_HQ[pos];
        //     if (_cached_scan_node_hq_count_for_interval_retrieve == _countof(_cached_scan_node_hq_buf_for_interval_retrieve)) {
        //         _cached_scan_node_hq_count_for_interval_retrieve -= 1; // prevent overflow
        //     }
        //     xSemaphoreGive(lidarLock);
        // }
    }
    return SL_RESULT_OK;
}

// void rplidar_scan_task(void* params);

void rplidar_scan_task(void* params);

void rplidar_scan_task_init(rplidar_scan_cache scan) {
    // maybe pinned to core
    // literally fuck it global
    SCAN_CACHE_TYPE = scan;
    Serial.printf("CREATING SCAN TASK: %d\n", scan);
    BaseType_t err;
    err = xTaskCreatePinnedToCore(rplidar_scan_task, "RPLIDAR_SCAN", RPLIDAR_SCAN_TASK_STACK_SIZE, NULL, 
            RPLIDAR_SCAN_TASK_PRIORITY, &RPLidarScanTaskHandle, 0);
    Serial.printf("CREATE TASK LIDAR SCAN: %d\n", err);

    return;
}

void rplidar_scan_task(void* params) {
    // memset(LOCAL_SCAN, 0, sizeof(LOCAL_SCAN));
    Result<nullptr_t> ans = SL_RESULT_OK;

    LOCAL_COUNT = LOCAL_BUFFER_SIZE;
    LOCAL_SCAN_COUNT = 0;
    
    // always discard the first data since it may be incomplete, hence the calls to wait functions
    // assign function pointer for cache
    sl_result (*cacheLidarData)(void);
    switch (SCAN_CACHE_TYPE) {
        case CACHE_SCAN_DATA:
            // _waitScanData(LOCAL_BUF, LOCAL_COUNT);
            cacheLidarData = &_cacheScanData;
            break;
        case CACHE_CAPSULED_SCAN_DATA:
            _waitCapsuledNode(capsule_node);
            cacheLidarData = &_cacheCapsuledScanData;
            break;
        case CACHE_HQ_SCAN_DATA:
            _waitHqNode(hq_node);
            cacheLidarData = &_cacheHqScanData;
            break;
        case CACHE_ULTRA_CAPSULED_SCAN_DATA:
            _waitUltraCapsuledNode(ultra_capsule_node);
            cacheLidarData = &_cacheUltraCapsuledScanData;
            break;
        // uh oh
        default:
            // spaghettio
            break;
    }

    // freertos style is to use infinite for loop
    for (;;) {
        // break once turned off
        if (!_isScanning) {
            Serial.printf("SCAN TERMINATING - TURNED OFF EXTERNALLY\n");
            break;
        }
        // function pointer funtimes
        if (!(ans = (*cacheLidarData)())) {
            Serial.printf("FAILED SCAN WITH ERR: %d\n", ans);
            break;
            // _isScanning = false;
            // vTaskDelete(NULL);
        }
        delay(RPLIDAR_SCAN_DELAY);
    }
    _isScanning = false;
    vTaskDelete(NULL);

}
// void AIFreeRTOS::startTaskImpl(void* _this)
// {
//   static_cast<AIFreeRTOS*>(_this)->task();
// }

// void AIFreeRTOS::startTask(void)
// {	
//  xTaskCreatePinnedToCore(this->startTaskImpl, "Task", 2048, this, _taskPriority, NULL, _taskCore);
// }

// void AIFreeRTOS::task(void)
// {	