#ifndef RPLIDAR_HEADER
#define RPLIDAR_HEADER

#include "hal/assert.h"
#include "hal/byteops.h"
#include "hal/types.h"
#include "hal/util.h"

#include "sl_crc.h"
#include "sl_lidar_driver.h"
#include "sl_lidar_cmd.h"
#include "sl_lidar_protocol.h"
#include "sl_types.h"
#include "rplidar_config.h"

#include <Arduino.h>
// #include <FreeRTOS.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dbscan.h"
// #include "SPIFFS.h"

using namespace sl;

// Event bits for controlling rplidar task
// Reset LiDAR.
#define EVT_RPLIDAR_RESET 1<<0
// Start LiDAR.
#define EVT_RPLIDAR_START 1<<1
// Stop LiDAR.
#define EVT_RPLIDAR_STOP  1<<2

// All event bits together... the full group...
#define EVT_RPLIDAR_GROUP ( EVT_RPLIDAR_RESET | EVT_RPLIDAR_START | EVT_RPLIDAR_STOP )

#define START_LOCAL_MIN     0
#define START_LOCAL_MAX     960
#define DEPOSIT_LOCAL_MIN   0
#define DEPOSIT_LOCAL_MAX   1192

#define START_FOREIGN_MIN   1800
#define START_FOREIGN_MAX   2612
#define DEPOSIT_FOREIGN_MIN 1800
#define DEPOSIT_FOREIGN_MAX 2515

#define PYLON_DISTANCE      2430

// bounding coordinates that the lidar can occupy
#define MIN_X_COORD         0
#define MAX_X_COORD         1200
#define MIN_START_Y_COORD   0
#define MAX_START_Y_COORD   600
#define MIN_DEPOSIT_Y_COORD 1800
#define MAX_DEPOSIT_Y_COORD 2400

// max range of lidar data that will be clustered
#define CLUSTER_MAX_DISTANCE 2700
// minimum number of samples required per cluster
#define CLUSTER_MIN_SAMPLES 1
// maximum radius between individual points within a cluster
#define CLUSTER_EPS 80.0
// maximum distance between extreme points of a cluster
#define CLUSTER_MAX_LENGTH 100

void rplidar_task_init();
extern ILidarDriver* drv;

typedef struct Coord2D {
    float x, y;
} Coord2D;

typedef struct LidarCluster {
    // cluster is valid candidate for pole
    bool valid = true;
    // Centre point of cluster
    Coord2D centroid;
    // coordinates of min X, Y points in cluster
    Coord2D x_min, y_min;
    // coordinates of max X, Y points in cluster
    Coord2D x_max, y_max;
    // num points in cluster
    int numPts;
    // clustered ID
    int clusterID;
} LidarCluster;

typedef struct PylonCluster {
    int clusterID;
    Coord2D centroid;
} PylonCluster;

typedef enum Zone {
    START_ZONE,
    DEPOSIT_ZONE
} Zone;

typedef enum ZonePylon {
    LOCAL_ZONE_PYLON,
    FOREIGN_ZONE_PYLON
} ZonePylon;


#endif
