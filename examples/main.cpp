#include <Arduino.h>

#include <rplidar_driver.h>

using namespace rp::standalone::rplidar;

RPlidarDriver *drv;

HardwareSerial lidarSerial(2);

void setup()
{
    // setup rplidar driver
    drv = RPlidarDriver::CreateDriver();
    // connect to selected serial port
    drv->connect(&lidarSerial);
}

void loop()
{
    // put your main code here, to run repeatedly:
}