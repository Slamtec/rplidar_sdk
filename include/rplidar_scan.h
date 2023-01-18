// #ifndef RPLIDAR_SCAN_H
// #define RPLIDAR_SCAN_H

// #include "hal/types.h"
// #include "hal/assert.h"
// #include "hal/util.h"

// #include "sl_lidar_cmd.h"
// #include "sl_crc.h" 
// #include "rplidar_config.h"

// #include <vector>
// #include <map>
// #include <Arduino.h>
// #include <algorithm>
// #include <string>
// #include <string.h>

// #include "esp_timer.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"


// uint32_t getms();

// /**
// * Wait for some data
// * \param size Bytes to wait
// * \param timeoutInMs Wait timeout (in microseconds, -1 for forever)
// * \param actualReady [out] actual ready bytes
// * \return true for data ready
// */
// bool waitForData(HardwareSerial* serial, size_t size, sl_u32 timeoutInMs = -1, size_t* actualReady = nullptr);

// void rplidar_scan_task_init(rplidar_scan_cache scan);

// extern rplidar_scan_cache SCAN_CACHE_TYPE;

// // yucky global variables controlling lidar scan and communication
// extern HardwareSerial * _channel;
// extern bool _isConnected;
// extern bool _isScanning;

// // internal sync of lidar buffer
// extern bool _scan_node_synced;

// // Task handle for rplidar_scan_task to allow for deletion and creation
// extern TaskHandle_t RPLidarScanTaskHandle;
// extern EventGroupHandle_t RPLidarScanEventGroup;
// extern SemaphoreHandle_t lidarLock;

// extern sl_lidar_response_measurement_node_hq_t   _cached_scan_node_hq_buf[RPLIDAR_BUFFER_SIZE];
// extern size_t                                    _cached_scan_node_hq_count;
// extern sl_u8                                     _cached_capsule_flag;

// // extern sl_lidar_response_measurement_node_hq_t   _cached_scan_node_hq_buf_for_interval_retrieve[RPLIDAR_BUFFER_SIZE];
// // extern size_t                                    _cached_scan_node_hq_count_for_interval_retrieve;

// #endif
