#pragma once

#define RPLIDAR_SERIAL_BAUDRATE 115200
#define RPLIDAR_DEFAULT_TIMEOUT 500
#define RPLIDAR_SERIAL_SIZE_RX  1024
// NOTE - changed
#define RPLIDAR_BUFFER_SIZE     1400
#define DEFAULT_TIMEOUT         2000
#define MAX_SCAN_NODES          RPLIDAR_BUFFER_SIZE
#define LOCAL_BUFFER_SIZE       256

// RPLidar Task Priorities
#define RPLIDAR_TASK_PRIORITY    ( tskIDLE_PRIORITY + 3 )

// RPLidar Task Stack Allocations (prob less big cash money memory )
#define RPLIDAR_TASK_STACK_SIZE    ( configMINIMAL_STACK_SIZE * 50 )

// RPLidar Scan Task Priorities
#define RPLIDAR_SCAN_TASK_PRIORITY    ( tskIDLE_PRIORITY + 4 )

// RPLidar Scan Task Stack Allocations (prob less big cash money memory )
#define RPLIDAR_SCAN_TASK_STACK_SIZE    ( configMINIMAL_STACK_SIZE * 50 )

#define RPLIDAR_SCAN_TASK_CORE 1
