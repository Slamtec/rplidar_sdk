Slamtec RPLIDAR Public SDK for C++
==================================

Introduction
------------

Slamtec RPLIDAR(https://www.slamtec.com/lidar/a3) series is a set of high-performance and low-cost LIDAR(https://en.wikipedia.org/wiki/Lidar) sensors, which is the perfect sensor of 2D SLAM, 3D reconstruction, multi-touch, and safety applications.

This is the public SDK of RPLIDAR products in C++, and open-sourced under GPLv3 license.

If you are using ROS (Robot Operating System), please use our open-source ROS node directly: https://github.com/slamtec/rplidar_ros .

If you are just evaluating RPLIDAR, you can use Slamtec RoboStudio(https://www.slamtec.com/robostudio) (currently only support Windows) to do the evaulation.

License
-------

The SDK itself is licensed under BSD 2-clause license.
The demo applications are licensed under GPLv3 license.

Release Notes
-------------

* [v1.11.0](https://github.com/slamtec/rplidar_sdk/blob/master/docs/ReleaseNote.v1.10.0.md)
* [v1.10.0](https://github.com/slamtec/rplidar_sdk/blob/master/docs/ReleaseNote.v1.10.0.md)
* [v1.9.1](https://github.com/slamtec/rplidar_sdk/blob/master/docs/ReleaseNote.v1.9.1.md)
* [v1.9.0](https://github.com/slamtec/rplidar_sdk/blob/master/docs/ReleaseNote.v1.9.0.md)
* [v1.8.1](https://github.com/slamtec/rplidar_sdk/blob/master/docs/ReleaseNote.v1.8.1.md)
* [v1.8.0](https://github.com/slamtec/rplidar_sdk/blob/master/docs/ReleaseNote.v1.8.0.md)

Supported Platforms
-------------------

RPLIDAR SDK supports Windows, macOS and Linux by using Visual Studio 2010 projects and Makefile.

| LIDAR Model \ Platform | Windows | macOS | Linux |
| ---------------------- | ------- | ----- | ------|
| A1                     | Yes     | Yes   | Yes   |
| A2                     | Yes     | Yes   | Yes   |
| A3                     | Yes     | Yes   | Yes   |

Quick Start
-----------

### On Windows

If you have Microsoft Visual Studio 2010 installed, just open sdk/workspaces/vc10/sdk_and_demo.sln, and compile. It contains the library as well as some demo applications.

### On macOS and Linux

Please make sure you have make and g++ installed, and then just invoke make in the root directory, you can get the compiled result at `output/$PLATFORM/$SCHEME`, such as `output/Linux/Release`.

    make

The Makefile compiles Release build by default, and you can also use `make DEBUG=1` to compile Debug builds.

Cross Compile
-------------

The Makefile system used by RPLIDAR public SDK support cross compiling.

The following command can be used to cross compile the SDK for `arm-linux-gnueabihf` targets:

    CROSS_COMPILE_PREFIX=arm-linux-gnueabihf ./cross_compile.sh

Demo Applications
-----------------

RPLIDAR public SDK includes some simple demos to do fast evaulation:

### ultra_simple

This demo application simply connects to an RPLIDAR device and outputs the scan data to the console.

    ultra_simple <serial_port_device>

For instance:

    ultra_simple \\.\COM11  # on Windows
    ultra_simple /dev/ttyUSB0

> Note: Usually you need root privilege to access tty devices under Linux. To eliminate this limitation, please add `KERNEL=="ttyUSB*", MODE="0666"` to the configuration of udev, and reboot.

### simple_grabber

This application demonstrates the process of getting RPLIDAR’s serial number, firmware version and healthy status after connecting the PC and RPLIDAR. Then the demo application grabs two round of scan data and shows the range data as histogram in the command line mode.

### frame_grabber (Legacy)

This demo application can show real-time laser scans in the GUI and is only available on Windows platform.

We have stopped the development of this demo application, please use Slamtec RoboStudio (https://www.slamtec.com/robostudio) instead.

SDK Usage
---------

> For detailed documents of RPLIDAR SDK, please refer to our user manual: https://download.slamtec.com/api/download/rplidar-sdk-manual/1.0?lang=en

### Include Header

    #include <rplidar.h>

Usually you only need to include this file to get all functions of RPLIDAR SDK.

### SDK Initialization and Termination

There are two static interfaces to create and dispose RPLIDAR driver instance. Each RPLIDAR driver instance can only be used to communicate with one RPLIDAR device. You can freely allocate arbitrary number of RPLIDAR driver instances to communicate with multiple RPLIDAR devices concurrently.

    /// Create an RPLIDAR Driver Instance
    /// This interface should be invoked first before any other operations
    ///
    /// \param drivertype the connection type used by the driver. 
    static RPlidarDriver * RPlidarDriver::CreateDriver(_u32 drivertype = DRIVER_TYPE_SERIALPORT);

    /// Dispose the RPLIDAR Driver Instance specified by the drv parameter
    /// Applications should invoke this interface when the driver instance is no longer used in order to free memory
    static void RPlidarDriver::DisposeDriver(RPlidarDriver * drv);

For example:

    #include <rplidar.h>

    int main(int argc, char* argv)
    {
        RPlidarDriver* lidar = RPlidarDriver::CreateDriver();

        // TODO

        RPlidarDriver::DisposeDriver(lidar);
    }

### Connect to RPLIDAR

After creating an RPlidarDriver instance, you can use `connect()` method to connect to a serial port:

    u_result res = lidar->connect("/dev/ttyUSB0", 115200);

    if (IS_OK(res))
    {
        // TODO
        lidar->disconnect();
    }
    else
    {
        fprintf(stderr, "Failed to connect to LIDAR %08x\r\n", res);
    }

### Start spinning motor

The LIDAR is not spinning by default. Method `startMotor()` is used to start this motor.

> For RPLIDAR A1 series, this method will enable DTR signal to make the motor rotate; for A2 and A3 serieses, the method will make the accessory board to output a PWM signal to MOTOR_PWM pin.

    lidar->startMotor();
    // TODO
    lidar->stopMotor();

### Start scan

Slamtec RPLIDAR support different scan modes for compatibility and performance. Since RPLIDAR SDK 1.6.0, a new API `getAllSupportedScanModes()` has been added to the SDK.

    std::vector<RplidarScanMode> scanModes;
    lidar->getAllSupportedScanModes(scanModes);

You can pick a scan mode from this list like this:

    lidar->startScanExpress(false, scanModes[0].id);

Or you can just use the typical scan mode of RPLIDAR like this:

    RplidarScanMode scanMode;
    lidar->startScan(false, true, 0, &scanMode);

### Grab scan data

When the RPLIDAR is scanning, you can use `grabScanData()` and `grabScanDataHq()` API to fetch one frame of scan. The difference between `grabScanData()` and `grabScanDataHq()` is the latter one support distances farther than 16.383m, which is required for RPLIDAR A2M6-R4 and RPLIDAR A3 series.

> The `grabScanDataHq()` API is backward compatible with old LIDAR models and old firmwares. So we recommend always using this API, and use `grabScanData()` only for compatibility.

    rplidar_response_measurement_node_hq_t nodes[8192];
    size_t nodeCount = sizeof(nodes)/sizeof(rplidar_response_measurement_node_hq_t);
    res = lidar->grabScanDataHq(nodes, nodeCount);

    if (IS_FAIL(res))
    {
        // failed to get scan data
    }

### Defination of data structure `rplidar_response_measurement_node_hq_t`

The defination of `rplidar_response_measurement_node_hq_t` is:

    #if defined(_WIN32)
    #pragma pack(1)
    #endif

    typedef struct rplidar_response_measurement_node_hq_t {
        _u16   angle_z_q14; 
        _u32   dist_mm_q2; 
        _u8    quality;  
        _u8    flag;
    } __attribute__((packed)) rplidar_response_measurement_node_hq_t;

    #if defined(_WIN32)
    #pragma pack()
    #endif

The definiton of each fields are:

| Field       | Data Type | Comments                                             |
| ----------- | --------- | -----------------------------------------------------|
| angle_z_q14 | u16_z_q14 | It is a fix-point angle desciption in z presentation |
| dist_mm_q2  | u32_q2    | Distance in millimeter of fixed point values         |
| quality     | u8        | Measurement quality (0 ~ 255)                        |
| flag        | u8        | Flags, current only one bit used: `RPLIDAR_RESP_MEASUREMENT_SYNCBIT` |

For example:

    float angle_in_degrees = node.angle_z_q14 * 90.f / (1 << 14);
    float distance_in_meters = node.dist_mm_q2 / 1000.f / (1 << 2);

Contact Slamtec
---------------

If you have any extra questions, please feel free to contact us at our support email:

    support AT slamtec DOT com