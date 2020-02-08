/* RPLIDAR S1 GTK-based demo
 *
 * Thomas Kircher <tkircher@gnu.org>, 2019
 *
 * Based on ../ultra_simple
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
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "rplidar.h"

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

using namespace rp::standalone::rplidar;

// Global variables
GtkWidget *window, *drawing_area;
RPlidarDriver *drv;
rplidar_response_measurement_node_hq_t nodes[8192];
size_t count = _countof(nodes);
float scale_factor = 10.0f;

// This is called on SIGINT
void handle_signal(int) {
   gtk_main_quit();
}

bool checkRPLIDARHealth(RPlidarDriver *drv)
{
    u_result op_result;
    rplidar_response_device_health_t healthinfo;

    op_result = drv->getHealth(healthinfo);
    if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
        printf("RPLidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, rplidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want rplidar to be reboot by software
            // drv->reset();
            return false;
        } else {
            return true;
        }

    } else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        return false;
    }
}

gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    guint width, height;
    GtkStyleContext *context;
    double r, t;
    int x, y, n;

    context = gtk_widget_get_style_context(widget);
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(context, cr, 0, 0, width, height);

    if(drv != NULL) {
        drv->ascendScanData(nodes, count);

        for(n = 0; n < (int)count; ++n) {
            // Convert to distance in meters
            r = (nodes[n].dist_mm_q2 / scale_factor) / (1 << 2);

            // Convert to angle in radians
            t = ((M_PI / 180) * (nodes[n].angle_z_q14 * 90.f)) / (1 << 14);

            // Center the points in the window
            x = (int)(r * sin(t)) + 1024 / 2;
            y = (int)(r * cos(t)) + 768 / 2;

            // Draw a dot
            cairo_move_to(cr, x, y);
            cairo_arc(cr, x, y, 2, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
    return FALSE;
}

// Timer callback
gboolean read_LIDAR_data(RPlidarDriver *drv) {
    u_result op_result;

    // Read scan data
    op_result = drv->grabScanDataHq(nodes, count);

    // Force a redraw
    gtk_widget_queue_draw(GTK_WIDGET(window));

    return TRUE;
}

gboolean handle_keypress(GtkWidget *widget, GdkEventKey *event, gpointer data) {
   switch(event->keyval) {
      case GDK_KEY_Up:
         if(scale_factor > 1)
            scale_factor -= 1;
         break;
      case GDK_KEY_Down:
         if(scale_factor < 1000)
            scale_factor += 1;
         break;
      default:
         break;
   }
   return FALSE;
}

gint main(int argc, char *argv[]) {
    const char * opt_com_path = NULL;
    u_result op_result;

    if(!opt_com_path)
#ifdef _WIN32
        // use default com port
        opt_com_path = "\\\\.\\com57";
#elif __APPLE__
        opt_com_path = "/dev/tty.SLAB_USBtoUART";
#else
        opt_com_path = "/dev/ttyUSB0";
#endif

    // create the driver instance
    drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);

    if(!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    
    rplidar_response_device_info_t devinfo;
    bool connectSuccess = false;

    if(!drv)
        drv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);

    if(IS_OK(drv->connect(opt_com_path, 256000))) {
        op_result = drv->getDeviceInfo(devinfo);

        if(IS_OK(op_result))
            connectSuccess = true;
        else {
            delete drv;
            drv = NULL;
        }
    }

    if (!connectSuccess) {
        fprintf(stderr, "Can't connect to specified serial port %s.\n", opt_com_path);
        goto on_finished;
    }

    // print out the device serial number, firmware and hardware version number..
    printf("RPLIDAR S/N: ");
    for (int pos = 0; pos < 16 ;++pos) {
        printf("%02X", devinfo.serialnum[pos]);
    }

    printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);

    // check health...
    if (!checkRPLIDARHealth(drv)) {
        goto on_finished;
    }

    // Start the motor
    drv->startMotor();

    // Start the LIDAR scan
    drv->startScanNormal(0,1);

    // Start the GUI
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
   
    //gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(handle_keypress), NULL);

    drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), drawing_area);
    gtk_widget_set_size_request(drawing_area, 1024, 768);

    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_callback), NULL);
    gtk_widget_show_all(window);

    // Call this every 100ms
    g_timeout_add_full(G_PRIORITY_HIGH, 100, G_SOURCE_FUNC(read_LIDAR_data), drv, NULL);

    signal(SIGINT, handle_signal);

    gtk_main();

    // Shut down the S1 and clean up when gtk_main() returns
    printf("Shutting down RPLIDAR S1\n");
    drv->stop();
    drv->stopMotor();

on_finished:
    RPlidarDriver::DisposeDriver(drv);
    drv = NULL;

    return 0;
}

