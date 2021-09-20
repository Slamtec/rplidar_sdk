#include "rplidar.h" //RPLIDAR standard sdk, all-in-one header
#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>

using namespace rp::standalone::rplidar;

enum LidarVersion
{
    NONE_DETECTED = 0,
    A2 = 1,
    A3 = 2
};

LidarVersion lidar_version = NONE_DETECTED;

bool check_if_lidar_connected_at_baudrate(unsigned int baudrate)
{
    RPlidarDriver* driver = RPlidarDriver::CreateDriver();
    u_result res = driver->connect("/dev/rplidar", baudrate);
    bool connected = false;
    if (IS_OK(res))
    {
        std::vector<RplidarScanMode> scanModes;
        driver->getAllSupportedScanModes(scanModes);
        if (scanModes.size() > 0)
        {
            connected = true;
        }
        driver->disconnect();
    }

    RPlidarDriver::DisposeDriver(driver);
    return connected;
}

void save_lidar_version(std::string path)
{

    std::ofstream myfile;
    myfile.open(path.c_str());

    if (lidar_version == A2)
    {
        myfile << "export RPLIDAR_VERION=A2\n";
    }
    else if (lidar_version == A3)
    {
        myfile << "export RPLIDAR_VERION=A3\n";
    }
    else
    {
        myfile << "export RPLIDAR_VERION=NOT_DETECTED\n";
    }
    myfile.close();
}

int main(int argc, char* argv[])
{
    std::string file_path;
    if (argc != 2)
    {
        std::cout << "path to rplidar_version file is not provided!" << std:: endl;
        return -1;
    }
    else
    {
        file_path = std::string(argv[1]);
    }
    


    //A2 lidar
    if (check_if_lidar_connected_at_baudrate(115200))
    {
        lidar_version = A2;
    }

    //A3 lidar 
    if (check_if_lidar_connected_at_baudrate(256000))
    {
        lidar_version = A3;
    }

    save_lidar_version(file_path);


    return 0;
}
