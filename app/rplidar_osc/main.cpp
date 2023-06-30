/*
 *  Slamtec LIDAR OSC sender
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2020 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *  Copyright (c) 2023 - Interferences Arts et Technologies
 *  https://interferences.ca
 * 
 *  @author: Alexandre Quessy <alexandre.quessy@artpluscode.com>
 *  https://artpluscode.com
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
#include <chrono>

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
static inline void delay(sl_word_size_t ms) {
    while (ms >= 1000) {
        usleep(1000 * 1000);
        ms -= 1000;
    };
    if (ms != 0)
        usleep(ms * 1000);
}
#endif

// Constants
static const char* DEFAULT_OSC_SEND_ADDRESS = "127.0.0.1";
static const int DEFAULT_OSC_SEND_PORT = 5678;
static const char* SOFTWARE_VERSION = "0.1.0";

/**
 * Settings for this program.
 */
class Settings {

private:
    /**
     * UDP port to send OSC to.
     */
    int output_osc_port = 5678;
    /**
     * Host to send OSC to.
     */
    std::string output_osc_host = DEFAULT_OSC_SEND_ADDRESS;

    int input_channel_type = CHANNEL_TYPE_SERIALPORT;

    // The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089.
    int input_udp_port = 8089;

    // The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089.
    std::string input_udp_host = "192.168.11.2";

    std::string input_serial_device;
    // The baudrate is 115200(for A2) , 256000(for A3 and S1), 1000000(for S2).
    int input_serial_baudrate;

    // There are two scan modes: express and standard.
    bool is_express_mode = false;

    // SHow help and exit.
    bool is_show_help = false;

    // Display a verbose output.
    bool is_verbose = true;

    // Retry in case of an error (experimental)
    bool is_retry_if_error = false;
public:
    Settings() {
        // Set all the default values:
        this->input_serial_device = "COM3";
        this->input_serial_baudrate = 1000000;
    }
public:

    void set_output_osc_port(int value) {
        this->output_osc_port = value;
    }

    int get_output_osc_port() const {
        return this->output_osc_port;
    }

    void set_output_osc_host(std::string& value) {
        this->output_osc_host = value;
    }

    std::string get_output_osc_host() const {
        return this->output_osc_host;
    }

    int get_input_channel_type() const {
        return this->input_channel_type;
    }

    int is_input_channel_serial() const {
        return this->input_channel_type == CHANNEL_TYPE_SERIALPORT;
    }

    int is_input_channel_udp() const {
        return this->input_channel_type == CHANNEL_TYPE_UDP;
    }

    void set_input_channel_type_serial() {
        this->input_channel_type = CHANNEL_TYPE_SERIALPORT;
    }

    void set_input_channel_type_udp() {
        this->input_channel_type = CHANNEL_TYPE_UDP;
    }

    void set_input_udp_host(std::string& value) {
        this->input_udp_host = value;
    }

    std::string get_input_udp_host() const {
        return this->input_udp_host;
    }

    void set_input_udp_port(int value) {
        this->input_udp_port = value;
    }

    int get_input_udp_port() const {
        return this->input_udp_port;
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

    void set_is_show_help(bool value) {
        this->is_show_help = value;
    }

    bool get_is_show_help() const {
        return this->is_show_help;
    }

    void set_is_verbose(bool value) {
        this->is_verbose = value;
    }

    bool get_is_verbose() const {
        return this->is_verbose;
    }

    void set_is_retry_if_error(bool value) {
        this->is_retry_if_error = value;
    }

    bool get_is_retry_if_error() const {
        return this->is_retry_if_error;
    }

    void set_is_express_mode(bool value) {
        this->is_express_mode = value;
    }

    bool get_is_express_mode() const {
        return this->is_express_mode;
    }

    friend std::ostream& operator<< (std::ostream& out, const Settings& settings);
};

std::ostream& operator<< (std::ostream& out, const Settings& settings) {
    out << "Settings:";
    out << std::endl << " - output_osc_host: " << settings.output_osc_host;
    out << std::endl << " - output_osc_port: " << settings.output_osc_port;
    out << std::endl << " - input_channel_type: ";
    if (settings.is_input_channel_serial()) {
        out << "SERIAL";
        out << std::endl << " - input_serial_baudrate: " << settings.input_serial_baudrate;
        out << std::endl << " - input_serial_device: " << settings.input_serial_device;
    }
    else {
        out << "UDP";
        out << std::endl << " - input_udp_host: " << settings.input_udp_host;
        out << std::endl << " - input_udp_port: " << settings.input_udp_port;
    }
    out << std::endl << " - is_express_mode: " << (settings.get_is_express_mode() ? "true" : "false");
    out << std::endl << " - is_retry_if_error: " << (settings.get_is_retry_if_error() ? "true" : "false");
    out << std::endl << " - is_show_help: " << (settings.get_is_show_help() ? "true" : "false");
    out << std::endl << " - is_verbose: " << (settings.get_is_verbose() ? "true" : "false");
    return out;
}

/**
 * Take only one 360 deg scan and display the result as a histogram
 * you can force slamtec lidar to perform scan operation regardless whether the motor is rotating
 */
void plot_histogram(sl_lidar_response_measurement_node_hq_t* nodes, size_t count) {
    const int BARCOUNT = 75;
    const int MAXBARHEIGHT = 20;
    const float ANGLESCALE = 360.0f / BARCOUNT;

    float histogram[BARCOUNT];
    for (int pos = 0; pos < _countof(histogram); ++pos) {
        histogram[pos] = 0.0f;
    }

    float max_val = 0;
    for (int pos = 0; pos < (int)count; ++pos) {
        int int_deg = (int)(nodes[pos].angle_z_q14 * 90.f / 16384.f);
        if (int_deg >= BARCOUNT) {
            int_deg = 0;
        }
        float cachedd = histogram[int_deg];
        if (cachedd == 0.0f) {
            cachedd = nodes[pos].dist_mm_q2 / 4.0f;
        }
        else {
            cachedd = (nodes[pos].dist_mm_q2 / 4.0f + cachedd) / 2.0f;
        }

        if (cachedd > max_val) {
            max_val = cachedd;
        }
        histogram[int_deg] = cachedd;
    }

    for (int height = 0; height < MAXBARHEIGHT; ++height) {
        float threshold_h = (MAXBARHEIGHT - height - 1) * (max_val / MAXBARHEIGHT);
        for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
            if (histogram[xpos] >= threshold_h) {
                putc('*', stdout);
            }
            else {
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
            for (int pos = 0; pos < (int)count; ++pos) {
                printf("%s theta: %03.2f Dist: %08.2f \n",
                    (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? "S " : "  ",
                    (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                    nodes[pos].dist_mm_q2 / 4.0f);
            }
        }
    }
    else {
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
sl_result capture_and_send_osc_once(ILidarDriver* drv, UdpTransmitSocket& transmitSocket, int elapsed_ms) {
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
            osc::OutboundPacketStream packet_stream_node(oscMessageStringBuffer, OSC_OUTPUT_BUFFER_SIZE);
            packet_stream_node << osc::BeginMessage("/lidar");
            // osc_outbound_packet_stream << is_sync;
            packet_stream_node << angle;
            packet_stream_node << distance;
            packet_stream_node << osc::EndMessage;
            transmitSocket.Send(packet_stream_node.Data(), packet_stream_node.Size());
        }

        // Send number of nodes read.
        osc::OutboundPacketStream packet_stream_nodes_count(oscMessageStringBuffer, OSC_OUTPUT_BUFFER_SIZE);
        packet_stream_nodes_count << osc::BeginMessage("/nodes_count");
        packet_stream_nodes_count << (int) numNodes;
        packet_stream_nodes_count << osc::EndMessage;
        transmitSocket.Send(packet_stream_nodes_count.Data(), packet_stream_nodes_count.Size());

        // Send elapsed time since last reading.
        osc::OutboundPacketStream packet_stream_elapsed(oscMessageStringBuffer, OSC_OUTPUT_BUFFER_SIZE);
        packet_stream_elapsed << osc::BeginMessage("/elapsed");
        packet_stream_elapsed << elapsed_ms;
        packet_stream_elapsed << osc::EndMessage;
        transmitSocket.Send(packet_stream_elapsed.Data(), packet_stream_elapsed.Size());

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
    std::chrono::steady_clock::time_point previous_time_point = std::chrono::steady_clock::now();
    
    while (true) {
        // Measure elapsed time since last reading.
        std::chrono::steady_clock::time_point time_point_now = std::chrono::steady_clock::now();
        std::chrono::milliseconds elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            time_point_now - previous_time_point);
        previous_time_point = time_point_now;

        sl_result result = capture_and_send_osc_once(drv, transmitSocket, static_cast<int>(elapsed_ms.count()));
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
        std::unique_ptr<cxxopts::Options> allocated(new cxxopts::Options(argv[0], "Reads data from LiDAR and send it over OSC"));
        auto& options = *allocated;
        options
            .positional_help("[optional args]")
            .show_positional_help();
        options
            .set_width(70)
            .set_tab_expansion()
            .allow_unrecognised_options()
            .add_options()
            ("u,enable-udp-input", "Enable the UDP input. Otherwise, we use the serial input.", cxxopts::value<bool>()->default_value("false"))
            ("r,input-serial-baudrate", "Input serial baudrate. (1000000) Use 115200 for A2, 256000 for A3 and S1, 1000000 for S2.", cxxopts::value<int>(), "N")
            ("input-serial-device", "Input serial device. (COM3)", cxxopts::value<std::string>())
            ("input-udp-port", "Input UDP port (8089)", cxxopts::value<int>(), "N")
            ("input-udp-host", "Input UDP host (192.168.11.2)", cxxopts::value<std::string>())
            ("H,output-osc-host", "Output OSC host (127.0.0.1) Only IP addresses are supported.", cxxopts::value<std::string>())
            ("p,output-osc-port", "Output OSC port (5678)", cxxopts::value<int>(), "N")
            ("q,quiet", "Quiet non-verbose output.", cxxopts::value<bool>()->default_value("false"))
            ("x,express", "Express scan mode. (the default is standard)", cxxopts::value<bool>()->default_value("false"))
            // TODO: Add a retry if error option
            // TODO: Add an option to show the histogram
            // TODO: Add an option to print the data once and exit.
            // TODO: Add an option for the initial sleep duration. (default is 3000 ms)
            ("help", "Prints this help and exits.")
            ;
        auto result = options.parse(argc, argv);

        if (result.count("quiet")) {
            settings.set_is_verbose(false);
        }
        if (result.count("express")) {
            settings.set_is_express_mode(true);
        }
        if (result.count("input-serial-baudrate")) {
            int value = result["input-serial-baudrate"].as<int>();
            settings.set_input_serial_baudrate(value);
        }
        if (result.count("input-serial-device")) {
            std::string value = result["input-serial-device"].as<std::string>();
            settings.set_input_serial_device(value);
        }
        if (result.count("input-udp-port")) {
            int value = result["input-udp-port"].as<int>();
            settings.set_input_serial_baudrate(value);
        }
        if (result.count("input-udp-host")) {
            std::string value = result["input-udp-host"].as<std::string>();
            settings.set_input_udp_host(value);
        }
        if (result.count("output-osc-host")) {
            std::string value = result["output-osc-host"].as<std::string>();
            settings.set_output_osc_host(value);
        }
        if (result.count("enable-udp-input")) {
            settings.set_input_channel_type_udp();
        }
        if (result.count("help")) {
            settings.set_is_show_help(true);
            if (settings.get_is_verbose()) {
                std::cout << options.help({ "", "Group" }) << std::endl;
            }
            return true;
        }
        if (settings.get_is_verbose()) {
            std::cout << "Version: " << SOFTWARE_VERSION << std::endl;
            // Print remaining arguments:
            auto& arguments = result.arguments();
            if (arguments.size() > 0) {
                std::cout << "Number of command-line arguments: " << argc << std::endl;
                std::cout << "Parsed " << arguments.size() << " options:" << std::endl;
                for (auto iter = arguments.begin(); iter != arguments.end(); iter++) {
                    cxxopts::KeyValue item = *iter;
                    std::cout << " - " << item.key() << ": " << item.value() << std::endl;
                }
            }
            // Print unmatched options:
            std::vector<std::string> unmatchedOptions = result.unmatched();
            if (unmatchedOptions.size() > 0) {
                std::cout << "Warning: Unmatched options: ";
                for (const auto& option : unmatchedOptions) {
                    std::cout << "'" << option << "' ";
                }
                std::cout << std::endl;
            }
        }
    } catch (const cxxopts::exceptions::exception& e) {
        std::cout << "Error parsing options: " << e.what() << std::endl;
        return false;
    }
    return true;
}


int main(int argc, const char* argv[]) {
    const char* opt_channel = NULL;
    const char* opt_channel_param_first = NULL;
    sl_u32 opt_channel_param_second = 0;
    sl_result op_result;
    int opt_channel_type = CHANNEL_TYPE_SERIALPORT;
    Settings settings = Settings();
    int ret_val = 0;

    int parse_ret_val = parse_command_line_options(argc, argv, settings);
    if (! parse_ret_val) {
        return 1; // exit with an error.
    }

    // Exit with 0 if --help option was provided.
    if (settings.get_is_show_help()) {
        return 0; // exit with no error.
    }

    if (settings.get_is_verbose()) {
        std::cout << settings << std::endl;
    }

    UdpTransmitSocket transmitSocket(IpEndpointName(
        settings.get_output_osc_host().c_str(),
        settings.get_output_osc_port()));
    IChannel* _channel;

    // create the driver instance
    ILidarDriver* drv = *createLidarDriver();

    if (! drv) {
        fprintf(stderr, "Insufficent memory, exit\n");
        exit( -2 );
    }

    sl_lidar_response_device_health_t healthinfo;
    sl_lidar_response_device_info_t devinfo;
    // start a while loop:
    do {
        ////////////////////////////////////////
        ret_val = 0; // Reset the return value to 0. (success) In case it's a retry.
        // try to connect
        if (settings.is_input_channel_serial()) {
            _channel = (
                *createSerialPortChannel(
                    settings.get_input_serial_device().c_str(),
                    settings.get_input_serial_baudrate()
                )
            );
        } else if (settings.is_input_channel_udp()) {
            _channel = *createUdpChannel(
                settings.get_input_udp_host(),
                settings.get_input_udp_port()
            );
        }
        if (SL_IS_FAIL((drv)->connect(_channel))) {
            switch (settings.get_input_channel_type()) {
            case CHANNEL_TYPE_SERIALPORT:
                fprintf(
                    stderr,
                    "Error, cannot bind to the specified serial port %s.\n",
                    settings.get_input_serial_device().c_str()
                );
                break; // exit the switch-case
            case CHANNEL_TYPE_UDP:
                fprintf(
                    stderr,
                    "Error, cannot connect to the ip addr %s with the udp port %d.\n",
                    settings.get_input_udp_host().c_str(),
                    settings.get_input_udp_port()
                );
                break; // exit the switch-case
            }
        }

        // Retrieve the device info
        op_result = drv->getDeviceInfo(devinfo);
        if (SL_IS_FAIL(op_result)) {
            if (op_result == SL_RESULT_OPERATION_TIMEOUT) {
                // you can check the detailed failure reason
                fprintf(stderr, "Error, operation time out.\n");
                ret_val = 1; // Will exit with an error
            } else {
                fprintf(stderr, "Error, unexpected error, code: %x\n", op_result);
                ret_val = 1; // Will exit with an error
                // other unexpected result
            }
            break; // exit the while loop
        }

        if (settings.get_is_verbose()) {
            // Print out the device serial number, firmware and hardware version number..
            printf("SLAMTEC LIDAR serial number: ");
            for (int pos = 0; pos < 16; ++pos) {
                printf("%02X", devinfo.serialnum[pos]);
            }
            printf("\n"
                "Version:  %s \n"
                "Firmware Ver: %d.%02d\n"
                "Hardware Rev: %d\n"
                , "SL_LIDAR_SDK_VERSION"
                , devinfo.firmware_version >> 8
                , devinfo.firmware_version & 0xFF
                , (int)devinfo.hardware_version);
        }
        
        // Check the device health
        op_result = drv->getHealth(healthinfo);
        if (SL_IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
            switch (healthinfo.status) {
                case SL_LIDAR_STATUS_OK: {
                    if (settings.get_is_verbose()) {
                        std::cout << "Lidar health status : OK." << std::endl;

                    }
                    break; // exit the switch-case
                }
                case SL_LIDAR_STATUS_WARNING: {
                    std::cout << "Lidar health status : Warning." << std::endl;
                    break; // exit the switch-case
                }
                case SL_LIDAR_STATUS_ERROR: {
                    std::cout << "Lidar health status : Error." << std::endl;
                    break; // exit the switch-case
                }
            }
            if (healthinfo.status != SL_LIDAR_STATUS_OK) {
                printf(" (errorcode: %d)\n", healthinfo.error_code);
            }
        } else {
            fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
            ret_val = 1; // Will exit with an error
            break; // exit the while loop
        }

        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
            ret_val = 1; // Will exit with an error
            if (settings.get_is_retry_if_error()) {
                // Attempt to reboot the slamtec lidar by software (experimental)
                drv->reset();
            } else {
                break; // exit the while loop
            }
        }

        switch (opt_channel_type) {
            case CHANNEL_TYPE_SERIALPORT: {
                drv->setMotorSpeed();
                break; // exit the switch-case
            }
        }

        // Lists the scan modes and try to guess which one to use
        std::vector<LidarScanMode> scanModes;
        scanModes.clear();
        sl_result scanModesResult = drv->getAllSupportedScanModes(scanModes);
        sl_u16 typicalMode;
        std::map<sl_u16, std::string> scanModesMap;
        drv->getTypicalScanMode(typicalMode);

        std::vector<LidarScanMode>::iterator modeIter = scanModes.begin();
        if (settings.get_is_verbose()) {
            std::cout << "All supported scan modes:" << std::endl;
        }
        for (; modeIter != scanModes.end(); modeIter++) {
            std::string scanModeName = std::string(modeIter->scan_mode);
            sl_u16 scanModeId = modeIter->id;
            scanModesMap[scanModeId] = scanModeName;
            if (settings.get_is_verbose()) {
                std::cout << " - " << scanModeName << " (" << static_cast<int>(scanModeId) << ")" << std::endl;
            }
        }
        if (settings.get_is_verbose()) {
            std::cout << "Typical mode: " << typicalMode << std::endl;
        }

        // Take only one 360 deg scan and display the result as a histogram
        // you can force slamtec lidar to perform scan operation regardless whether the motor is rotating
        if (settings.get_is_express_mode()) {
            sl_u16 actual_scan_mode = typicalMode;
            // XXX: Uncomment this to use another scan mode:
            // actual_scan_mode = 0;
            if (settings.get_is_verbose()) {
                std::cout << "Using the express mode." << std::endl;
                std::string scanModeName = scanModesMap[actual_scan_mode];
                std::cout << "Actual scan mode we will attempt to use: " << actual_scan_mode << " (" << scanModeName << ")" << std::endl;
            }
            if (SL_IS_FAIL(drv->startScanExpress(false, actual_scan_mode, 0))) {
                fprintf(stderr, "Error, cannot start the scan operation.\n");
                ret_val = 1; // Will exit with an error
                break; // exit the while loop
            }
        } else {
            if (settings.get_is_verbose()) {
                std::cout << "Using the standard mode." << std::endl;
            }
            if (SL_IS_FAIL(drv->startScan(
                false, // force
                true // use typical scan
            ))) {
                fprintf(stderr, "Error, cannot start the scan operation.\n");
                ret_val = 1; // Will exit with an error
                break; // exit the while loop
            }
        }

        // Wait 3 seconds??
        // TODO: Investigate removing the 3-second delay before capturing and sending data.
        std::cout << "Waiting a little bit..." << std::endl; // FIXME: Not sure why.
        delay(3000);
        if (settings.get_is_verbose()) {
            std::cout << "Starting to scan." << std::endl;
        }

        if (SL_IS_FAIL(capture_and_send_osc(drv, transmitSocket))) {
            fprintf(stderr, "Error, cannot grab scan data.\n");
            ret_val = 1; // Will exit with an error
            break; // exit the while loop
        }

        // Deprecated:
        // TODO: re-enable printing the data on the console, but optionally, using a command-line argument.
        //if (SL_IS_FAIL(capture_and_display(drv))) {
        //    fprintf(stderr, "Error, cannot grab scan data.\n");
        //    break;
        //}
    } while (settings.get_is_retry_if_error());

    // Stop the driver
    drv->stop();

    // Stop the motor
    switch (settings.get_input_channel_type()) {
        case CHANNEL_TYPE_SERIALPORT: {
            delay(20);
            drv->setMotorSpeed(0);
            break;
        }
    }

    // Cleanup
    if (drv) {
        delete drv;
        drv = NULL;
    }

    return 0; // Exit with no error.
}
