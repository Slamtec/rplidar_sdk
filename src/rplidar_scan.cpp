#include "rplidar_scan.h"

#define LOCAL_BUFFER_SIZE 256

// Bunch o' dirty global variables - pls no blame me i hate it as much as you

// some cheeky globals for processing the scan
sl_lidar_response_measurement_node_t      LOCAL_BUF[LOCAL_BUFFER_SIZE];
sl_lidar_response_measurement_node_hq_t   LOCAL_SCAN[MAX_SCAN_NODES];

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
// HardwareSerial * _channel = NULL;
// bool _isConnected = false;
// bool _isScanning = false;

// internal sync of lidar buffer
bool _scan_node_synced = false;

// Task handle for rplidar_scan_task to allow for deletion and creation
TaskHandle_t RPLidarScanTaskHandle = NULL;
EventGroupHandle_t RPLidarScanEventGroup = NULL;
SemaphoreHandle_t lidarLock = NULL;

using namespace sl;


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