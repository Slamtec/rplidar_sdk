# Sending OSC from LiDAR

See "How to send data over OSC" below.

## Building this project

- On Windows 10
- Download and install Visual Studio 2022 Community from https://visualstudio.microsoft.com/downloads/
- `git clone https://github.com/Slamtec/rplidar_sdk.git`
- Decompress `tools/cp2102_driver/CP210x_Windows_Drivers.zip`
- Run `tools/cp2102_driver/CP210x_Windows_Drivers/CP210xVCPInstaller_x64.exe`
- Open `workspaces/vc14/sdk_and_demo.sln` with Visual Studio 2022 Community
- Select the "Release" Solution Configuration by using the dropdown menu that says
  "Debug" in the top of the main window. Instead, it should say "Release".
- In the Solution Explorer panel:
  - Right-click `rplidar_driver`, choose `Build`
  - Right-click `simple_grabber`, choose `Build`
  - Right-click `rplidar_osc`, choose `Build`
   - Right-click `simple_grabber`, choose `Set as Startup Project`
- Choose the menu `Debug`, `Start Without Debugging`
- You will notice a shell starting a tool, but it fails.
- Open another Command Prompt and run a similar command from there:

`mode`: to display the COM ports. (mine is COM3)

```
Microsoft Windows [Version 10.0.19044.1586]
(c) Microsoft Corporation. All rights reserved.

C:\Users\User>mode

Status for device COM3:
-----------------------
    Baud:            115200
    Parity:          None
    Data Bits:       8
    Stop Bits:       1
    Timeout:         OFF
    XON/XOFF:        OFF
    CTS handshaking: OFF
    DSR handshaking: OFF
    DSR sensitivity: OFF
    DTR circuit:     OFF
    RTS circuit:     OFF
 ```

Now, run a command like this, but with your COM port identifier instead of mine:

```
cd C:\Users\User\src\rplidar_sdk\output\win32\Debug\
simple_grabber.exe --channel --serial COM3 1000000 --udp 127.0.0.1 1234
```

You should see something like this:

```
C:\Users\User\src\rplidar_sdk\output\win32\Debug>simple_grabber.exe --channel --serial COM3 1000000 --udp 127.0.0.1 1234
SLAMTEC LIDAR S/N: C1E2ECF8C4E699D7B8EB99F94D3D4717
Version:  SL_LIDAR_SDK_VERSION
Firmware Ver: 1.01
Hardware Rev: 18
Lidar health status : OK. (errorcode: 0)
waiting for data...
                                                          *****
                                                      *  ******
                                                     **  ******
                                                 **  **  ****** ******
                                             ******  *** ****** ***********
                                          *********  *** ****** ***********
                                         **********  *** ****** ***********
                                         **********  ********** ***********
                                         **********  ********** ***********
                                         **********  ********** ***********
                                         **********  ********** ***********
                                         **********  ********** ***********
                                         **********  ********** ***********
                                         ********** *********** ***********
                                         ********** ***********************
                                        *********** ***********************
                                        *********** ***********************
*                                    *  *********** ***********************
***************   ********************  ***********************************
***************************************************************************
---------------------------------------------------------------------------
Do you want to see all the data? (y/n)
```

You can also see its help message:

```
C:\Users\User\src\rplidar_sdk\output\win32\Debug>simple_grabber.exe --help
Simple LIDAR data grabber for SLAMTEC LIDAR.
Version:  SL_LIDAR_SDK_VERSION
Usage:
 For serial channel simple_grabber.exe --channel --serial <com port> [baudrate]
The baudrate is 115200(for A2) , 256000(for A3 and S1), 1000000(for S2).
 For udp channel simple_grabber.exe --channel --udp <ipaddr> [port NO.]
The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089. Please refer to the datasheet for details.
```

## How to send data over OSC

```
cd C:\Users\User\src\rplidar_sdk\output\win32\Debug\
rplidar_osc.exe --channel --serial COM3 1000000
```

This will send OSC over port 5678 on localhost.

See the Max patch in receive_lidar_osc.maxpat
