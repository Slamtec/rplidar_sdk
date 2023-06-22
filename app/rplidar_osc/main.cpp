/*
 *  SLAMTEC LIDAR
 *  Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2020 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *  Copyright (c) 2023 - Interferences Arts et Technologies
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <memory>

#include "cxxopts.hpp"
#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/ip/UdpSocket.h"

// Using directives
using namespace sl;

// Macros
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

// Constants
// TODO: Allow to configure the OSC port and host to send to
static const char* DEFAULT_OSC_SEND_ADDRESS = "127.0.0.1";
static const int DEFAULT_OSC_SEND_PORT = 5678;

/**
 * Settings for this program.
 */
class Settings {
    
private:
    /**
     * UDP port to send OSC to.
     */
    int osc_send_port = 5678;
    /**
     * Host to send OSC to.
     */
    std::string osc_send_host = DEFAULT_OSC_SEND_ADDRESS;

    int input_channel_type = CHANNEL_TYPE_SERIALPORT;

    // The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089.
    int udp_input_port = 8089;

    // The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089.
    std::string udp_input_host = "192.168.11.2";

    std::string input_serial_device;
    // "The baudrate is 115200(for A2) , 256000(for A3 and S1), 1000000(for S2).\n"
    int input_serial_baudrate;
public:
    Settings() {
        // Set all the default values:
        this->osc_send_port = DEFAULT_OSC_SEND_PORT;
        this->osc_send_host = std::string(DEFAULT_OSC_SEND_ADDRESS);
        this->input_serial_device = "COM3";
        this->input_serial_baudrate = 1000000;
    }
public:

    void set_osc_send_port(int value) {
        this->osc_send_port = value;
    }

    int get_osc_send_port() const {
        return this->osc_send_port;
    }

    void set_osc_send_host(std::string& value) {
        this->osc_send_host = value;
    }

    std::string get_osc_send_host() const {
        return this->osc_send_host;
    }

    int get_input_channel_type() const {
        return this->input_channel_type;
    }

    void set_input_channel_type_serial() {
        this->input_channel_type = CHANNEL_TYPE_SERIALPORT;
    }

    void set_input_channel_type_udp() {
        this->input_channel_type = CHANNEL_TYPE_UDP;
    }

    void set_udp_input_host(std::string& value) {
        this->udp_input_host = value;
    }

    std::string get_upd_input_host() const {
        return this->udp_input_host;
    }

    void set_udp_input_port(int value) {
        this->udp_input_port = value;
    }

    int get_upd_input_port() const {
        return this->udp_input_port;
    }

    void set_input_serial_device(std::string& value) {
        this->input_serial_device = value;
    }

    std::string get_input_serial_device() const {
        return this->input_serial_device;
    }

    void set_input_serial_baudrate(int value) {
        this->input_serial_baudrate = value;
    }

    int get_input_serial_baudrate() const {
        return this->input_serial_baudrate;
    }
};

void print_usage(int argc, const char * argv[]) {
    printf("LIDAR data grabber and OSC sender for SLAMTEC LIDAR.\n"
           "Version:  %s \n"
           "Usage:\n"
           " For serial channel %s --channel --serial <com port> [baudrate]\n"
           "The baudrate is 115200(for A2) , 256000(for A3 and S1), 1000000(for S2).\n"
		   " For udp channel %s --channel --udp <ipaddr> [port NO.]\n"
           "The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089. Please refer to the datasheet for details.\n"
           , "SL_LIDAR_SDK_VERSION",  argv[0], argv[0]);
}


/**
 * Take only one 360 deg scan and display the result as a histogram
 * you can force slamtec lidar to perform scan operation regardless whether the motor is rotating 
 */
void plot_histogram(sl_lidar_response_measurement_node_hq_t* nodes, size_t count) {
    const int BARCOUNT =  75;
    const int MAXBARHEIGHT = 20;
    const float ANGLESCALE = 360.0f / BARCOUNT;

    float histogram[BARCOUNT];
    for (int pos = 0; pos < _countof(histogram); ++pos) {
        histogram[pos] = 0.0f;
    }

    float max_val = 0;
    for (int pos =0 ; pos < (int)count; ++pos) {
        int int_deg = (int) (nodes[pos].angle_z_q14 * 90.f / 16384.f);
        if (int_deg >= BARCOUNT) {
            int_deg = 0;
        }
        float cachedd = histogram[int_deg];
        if (cachedd == 0.0f ) {
            cachedd = nodes[pos].dist_mm_q2  /4.0f;
        } else {
            cachedd = (nodes[pos].dist_mm_q2 / 4.0f + cachedd) / 2.0f;
        }

        if (cachedd > max_val) {
            max_val = cachedd;
        }
        histogram[int_deg] = cachedd;
    }

    for (int height = 0; height < MAXBARHEIGHT; ++ height) {
        float threshold_h = (MAXBARHEIGHT - height - 1) * (max_val / MAXBARHEIGHT);
        for (int xpos = 0; xpos < BARCOUNT; ++ xpos) {
            if (histogram[xpos] >= threshold_h) {
                putc('*', stdout);
            }else {
                putc(' ', stdout);
            }
        }
        printf("\n");
    }
    for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
        putc('-', stdout);
    }
    printf("\n");
}

/**
 * Capture and display the data on the console.
 */
sl_result capture_and_display(ILidarDriver* drv) {
    sl_result ans;
	sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t count = _countof(nodes);

    printf("waiting for data...\n");

    ans = drv->grabScanDataHq(nodes, count, 0);
    if (SL_IS_OK(ans) || ans == SL_RESULT_OPERATION_TIMEOUT) {
        drv->ascendScanData(nodes, count);
        plot_histogram(nodes, count);
        printf("Do you want to see all the data? (y/n) ");
        int key = getchar();
        if (key == 'Y' || key == 'y') {
            for (int pos = 0; pos < (int)count ; ++pos) {
				printf("%s theta: %03.2f Dist: %08.2f \n", 
                    (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? "S " : "  ", 
                    (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                    nodes[pos].dist_mm_q2 / 4.0f);
            }
        }
    } else {
        printf("error code: %x\n", ans);
    }
    return ans;
}

/**
 * Captures LiDAR data and sends it over OSC.
 * 
 * Signature:
 * `/lidar ,ff <angle> <distance>
 * 
 * It should send 16384 messages per frame.
 */
sl_result capture_and_send_osc_once(ILidarDriver* drv, UdpTransmitSocket& transmitSocket) {
    sl_lidar_response_measurement_node_hq_t nodes[8192]; // FIXME: Consider allocating this only once.
    size_t numNodes = _countof(nodes);
    static const unsigned int OSC_OUTPUT_BUFFER_SIZE = 256;
    static char oscMessageStringBuffer[OSC_OUTPUT_BUFFER_SIZE];
    static const sl_u32 timeout = 2000; // See RPlidarDriver.DEFAULT_TIMEOUT; // FIXME: Should we change the default timeout?

    sl_result result = drv->grabScanDataHq(nodes, numNodes, timeout);
    if (SL_IS_OK(result) || result == SL_RESULT_OPERATION_TIMEOUT) {
        drv->ascendScanData(nodes, numNodes);
        for (int pos = 0; pos < (int) numNodes; ++pos) {
            // bool is_sync = (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? true : false;
            float angle = (nodes[pos].angle_z_q14 * 90.f) / 16384.f;
            float distance = nodes[pos].dist_mm_q2 / 4.0f;
            osc::OutboundPacketStream osc_outbound_packet_stream(oscMessageStringBuffer, OSC_OUTPUT_BUFFER_SIZE);
            osc_outbound_packet_stream << osc::BeginMessage("/lidar");
            // osc_outbound_packet_stream << is_sync;
            osc_outbound_packet_stream << angle;
            osc_outbound_packet_stream << distance;
            osc_outbound_packet_stream << osc::EndMessage;
            transmitSocket.Send(osc_outbound_packet_stream.Data(), osc_outbound_packet_stream.Size());
        }
    } else {
        printf("error code: %x\n", result);
    }
    return result;
}

/**
 * Capture LiDAR data and sends it over OSC.
 * Runs in an infinite loop.
 */
sl_result capture_and_send_osc(ILidarDriver* drv, UdpTransmitSocket& transmitSocket) {
    while (true) {
        sl_result result = capture_and_send_osc_once(drv, transmitSocket);
        if (SL_IS_FAIL(result)) {
            return result;
        }
        // TODO: Handle Ctrl-C interruptions.
    }
}



/**
 * Parse the command-line options for this program.
 */
bool parse_command_line_options(int argc, const char* argv[], Settings& settings) {
    try {
        std::unique_ptr<cxxopts::Options> allocated(new cxxopts::Options(argv[0], " - sends LiDAR data over OSC"));
        auto& options = *allocated;
        options
            .positional_help("[optional args]")
            .show_positional_help();

        bool apple = false;

        options
            .set_width(70)
            .set_tab_expansion()
            .allow_unrecognised_options()
            .add_options()
            ("a,apple,ringo", "an apple", cxxopts::value<bool>(apple))
            ("b,bob", "Bob")
            ("char", "A character", cxxopts::value<char>())
            ("t,true", "True", cxxopts::value<bool>()->default_value("true"))
            ("f, file", "File", cxxopts::value<std::vector<std::string>>(), "FILE")
            ("i,input", "Input", cxxopts::value<std::string>())
            ("o,output", "Output file", cxxopts::value<std::string>()
                ->default_value("a.out")->implicit_value("b.def"), "BIN")
            ("x", "A short-only option", cxxopts::value<std::string>())
            ("positional",
                "Positional arguments: these are the arguments that are entered "
                "without an option", cxxopts::value<std::vector<std::string>>())
            ("long-description",
                "thisisareallylongwordthattakesupthewholelineandcannotbebrokenataspace")
            ("help", "Print help")
            ("tab-expansion", "Tab\texpansion")
            ("int", "An integer", cxxopts::value<int>(), "N")
            ("float", "A floating point number", cxxopts::value<float>())
            ("vector", "A list of doubles", cxxopts::value<std::vector<double>>())
            ("option_that_is_too_long_for_the_help", "A very long option")
            ("l,list", "List all parsed arguments (including default values)")
            ("range", "Use range-for to list arguments")
#ifdef CXXOPTS_USE_UNICODE
            ("unicode", u8"A help option with non-ascii: à. Here the size of the"
                " string should be correct")
#endif
            ;

        options.add_options("Group")
            ("c,compile", "compile")
            ("d,drop", "drop", cxxopts::value<std::vector<std::string>>());

        options.parse_positional({ "input", "output", "positional" });

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            return true;
        }

        if (result.count("list")) {
            if (result.count("range")) {
                for (const auto& kv : result) {
                    std::cout << kv.key() << " = " << kv.value() << std::endl;
                }
            } else {
                std::cout << result.arguments_string() << std::endl;
            }
            return true;
        }

        if (apple) {
            std::cout << "Saw option ‘a’ " << result.count("a") << " times " <<
                std::endl;
        }

        if (result.count("b")) {
            std::cout << "Saw option ‘b’" << std::endl;
        }

        if (result.count("char")) {
            std::cout << "Saw a character ‘" << result["char"].as<char>() << "’" << std::endl;
        }

        if (result.count("f")) {
            auto& ff = result["f"].as<std::vector<std::string>>();
            std::cout << "Files" << std::endl;
            for (const auto& f : ff) {
                std::cout << f << std::endl;
            }
        }

        if (result.count("input")) {
            std::cout << "Input = " << result["input"].as<std::string>()
                << std::endl;
        }

        if (result.count("output")) {
            std::cout << "Output = " << result["output"].as<std::string>()
                << std::endl;
        }

        if (result.count("positional"))
        {
            std::cout << "Positional = {";
            auto& v = result["positional"].as<std::vector<std::string>>();
            for (const auto& s : v) {
                std::cout << s << ", ";
            }
            std::cout << "}" << std::endl;
        }

        if (result.count("int")) {
            std::cout << "int = " << result["int"].as<int>() << std::endl;
        }

        if (result.count("float"))
        {
            std::cout << "float = " << result["float"].as<float>() << std::endl;
        }

        if (result.count("vector")) {
            std::cout << "vector = ";
            const auto values = result["vector"].as<std::vector<double>>();
            for (const auto& v : values) {
                std::cout << v << ", ";
            }
            std::cout << std::endl;
        }

        std::cout << "Arguments remain = " << argc << std::endl;

        auto &arguments = result.arguments();
        std::cout << "Saw " << arguments.size() << " arguments" << std::endl;

        std::cout << "Unmatched options: ";
        for (const auto& option : result.unmatched())
        {
            std::cout << "'" << option << "' ";
        }
        std::cout << std::endl;
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return false;
    }

    return true;
}


int main(int argc, const char * argv[]) {
    const char* opt_channel = NULL;
    const char* opt_channel_param_first = NULL;
    sl_u32 opt_channel_param_second = 0;
    sl_result op_result;
	int opt_channel_type = CHANNEL_TYPE_SERIALPORT;
    UdpTransmitSocket transmitSocket(IpEndpointName(DEFAULT_OSC_SEND_ADDRESS, DEFAULT_OSC_SEND_PORT));
    IChannel* _channel;
    Settings settings = Settings();

    if (!parse_command_line_options(argc, argv, settings)) {
        return 1;
    }

    if (argc < 5) {
        print_usage(argc, argv);
        return -1;
    }

    // TODO: Troubleshoot and simplify command-line arguments parsing
	const char* opt_is_channel = argv[1];
	if (strcmp(opt_is_channel, "--channel") == 0) {
		opt_channel = argv[2];
		if (strcmp(opt_channel, "-s") == 0 || strcmp(opt_channel, "--serial") == 0) {
			opt_channel_param_first = argv[3];
            if (argc > 4) {
                opt_channel_param_second = strtoul(argv[4], NULL, 10);
            }
		}
		else if (strcmp(opt_channel, "-u") == 0 || strcmp(opt_channel, "--udp") == 0) {
			opt_channel_param_first = argv[3];
            if (argc > 4) {
                opt_channel_param_second = strtoul(argv[4], NULL, 10);
            }
			opt_channel_type = CHANNEL_TYPE_UDP;
		} else {
			print_usage(argc, argv);
			return -1;
		}
	} else {
		print_usage(argc, argv);
		return -1;
	}

    // create the driver instance
	ILidarDriver* drv = *createLidarDriver();

    if (! drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }

    sl_lidar_response_device_health_t healthinfo;
    sl_lidar_response_device_info_t devinfo;
    do {
        ////////////////////////////////////////
        // try to connect
        if (opt_channel_type == CHANNEL_TYPE_SERIALPORT) {
            _channel = (*createSerialPortChannel(opt_channel_param_first, opt_channel_param_second));
        } else if (opt_channel_type == CHANNEL_TYPE_UDP) {
            _channel = *createUdpChannel(opt_channel_param_first, opt_channel_param_second);
        }
        if (SL_IS_FAIL((drv)->connect(_channel))) {
			switch (opt_channel_type) {	
				case CHANNEL_TYPE_SERIALPORT:
					fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
						, opt_channel_param_first);
					break;
				case CHANNEL_TYPE_UDP:
					fprintf(stderr, "Error, cannot connect to the ip addr  %s with the udp port %s.\n"
						, opt_channel_param_first, opt_channel_param_second);
					break;
			}
        }

        ////////////////////////////////////////
        // retrieve the device info
        op_result = drv->getDeviceInfo(devinfo);
        if (SL_IS_FAIL(op_result)) {
            if (op_result == SL_RESULT_OPERATION_TIMEOUT) {
                // you can check the detailed failure reason
                fprintf(stderr, "Error, operation time out.\n");
            } else {
                fprintf(stderr, "Error, unexpected error, code: %x\n", op_result);
                // other unexpected result
            }
            break;
        }

        ////////////////////////////////////////
        // print out the device serial number, firmware and hardware version number..
        printf("SLAMTEC LIDAR S/N: ");
        for (int pos = 0; pos < 16 ;++pos) {
            printf("%02X", devinfo.serialnum[pos]);
        }
        printf("\n"
                "Version:  %s \n"
                "Firmware Ver: %d.%02d\n"
                "Hardware Rev: %d\n"
                , "SL_LIDAR_SDK_VERSION"
                , devinfo.firmware_version>>8
                , devinfo.firmware_version & 0xFF
                , (int)devinfo.hardware_version);

        ////////////////////////////////////////
        // check the device health
        op_result = drv->getHealth(healthinfo);
        if (SL_IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
            printf("Lidar health status : ");
            switch (healthinfo.status) {
				case SL_LIDAR_STATUS_OK:
					printf("OK.");
					break;
				case SL_LIDAR_STATUS_WARNING:
					printf("Warning.");
					break;
				case SL_LIDAR_STATUS_ERROR:
					printf("Error.");
					break;
            }
            printf(" (errorcode: %d)\n", healthinfo.error_code);

        } else {
            fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
            break;
        }

        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want slamtec lidar to be reboot by software
            // drv->reset();
            break;
        }

		switch (opt_channel_type) {	
			case CHANNEL_TYPE_SERIALPORT:
				drv->setMotorSpeed();
			break;
		}

        ////////////////////////////////////////
        // take only one 360 deg scan and display the result as a histogram
        // you can force slamtec lidar to perform scan operation regardless whether the motor is rotating
        if (SL_IS_FAIL(drv->startScan(0, 1))) {
            fprintf(stderr, "Error, cannot start the scan operation.\n");
            break;
        }

        ////////////////////////////////////////
        // Wait 3 seconds??
        // TODO: Investigate removing the 3-second delay before capturing and sending data.
		delay(3000);

        if (SL_IS_FAIL(capture_and_send_osc(drv, transmitSocket))) {
            fprintf(stderr, "Error, cannot grab scan data.\n");
            break;
        }

        // Deprecated:
        // TODO: re-enable printing the data on the console, but optionally, using a command-line argument.
        //if (SL_IS_FAIL(capture_and_display(drv))) {
        //    fprintf(stderr, "Error, cannot grab scan data.\n");
        //    break;
        //}
    } while(0);

    drv->stop();
    switch (opt_channel_type) {
		case CHANNEL_TYPE_SERIALPORT:
			delay(20);
			drv->setMotorSpeed(0);
		break;
	}
    if (drv) {
        delete drv;
        drv = NULL;
    }
    return 0;
}
