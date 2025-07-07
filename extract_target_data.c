/*
 ===================================================================================
 Name        : extract_target_data.c
 Description : Modified to send UDP data from received_target_data() to 127.0.0.1:5000
 Platform    : Windows (using Winsock)
 ===================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#include "include/Protocol.h"
#include "include/COMPort.h"
#include "include/EndpointTargetDetection.h"
#include "include/EndpointRadarBase.h"

#define AUTOMATIC_DATA_FRAME_TRIGGER (1)
#define AUTOMATIC_DATA_TRIGER_TIME_US (100000000) // 1 Hz

uint32_t counter = 0;
SOCKET udp_sock;
struct sockaddr_in udp_addr;

// Called when new radar target data is received
void received_target_data(void* context,
                          int32_t protocol_handle,
                          uint8_t endpoint,
                          const Target_Info_t* target_info, uint8_t num_targets)
{
    char buffer[512];
    int offset = 0;

	offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "NumTargets: %d ", num_targets);

    for (uint32_t i = 0; i < num_targets; ++i) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "TgtId: %ld Level: %.2f Range: %.2f Azimuth: %.2f Elevation: %.2f "
                           "RadialSpeed: %.2f AzimuthSpeed: %.2f ElevationSpeed: %.2f ",
                           target_info[i].target_id,
                           target_info[i].level,
                           target_info[i].radius,
                           target_info[i].azimuth,
                           target_info[i].elevation,
                           target_info[i].radial_speed,
                           target_info[i].azimuth_speed,
                           target_info[i].elevation_speed);
    }

    // Send over UDP
    if(counter % 20 == 0)
   {
    sendto(udp_sock, buffer, (int)strlen(buffer), 0,
           (struct sockaddr*)&udp_addr, sizeof(udp_addr));

    // Print to console for verification
    printf("%s\n", buffer);
    }
    counter ++;
}

int radar_auto_connect(void)
{
    int radar_handle = 0;
    int num_of_ports = 0;
    char comp_port_list[256];
    char* comport;
    const char *delim = ";";

    num_of_ports = com_get_port_list(comp_port_list, (size_t)256);

    if (num_of_ports == 0)
    {
        return -1;
    }
    else
    {
        comport = strtok(comp_port_list, delim);

        while (num_of_ports > 0)
        {
            num_of_ports--;

            radar_handle = protocol_connect(comport);
            if (radar_handle >= 0)
                break;

            comport = strtok(NULL, delim);
        }

        return radar_handle;
    }
}

int main(void)
{
    WSADATA wsaData;
    int res = -1;
    int protocolHandle = 0;
    int endpointTargetDetection = -1;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return -1;
    }

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock == INVALID_SOCKET)
    {
        printf("UDP socket creation failed.\n");
        WSACleanup();
        return -1;
    }

    // Setup UDP address (loopback, port 5000)
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &udp_addr.sin_addr);

    // Connect radar
    protocolHandle = radar_auto_connect();
    if (protocolHandle < 0)
    {
        printf("Failed to connect to radar.\n");
        closesocket(udp_sock);
        WSACleanup();
        return -1;
    }

    // Find target detection endpoint
    for (int i = 1; i <= protocol_get_num_endpoints(protocolHandle); ++i)
    {
        if (ep_targetdetect_is_compatible_endpoint(protocolHandle, i) == 0)
        {
            endpointTargetDetection = i;
            break;
        }
    }

    if (endpointTargetDetection >= 0)
    {
        // Register callback
        ep_targetdetect_set_callback_target_processing(received_target_data, NULL);

        // Enable or disable automatic trigger
        res = ep_radar_base_set_automatic_frame_trigger(
            protocolHandle,
            endpointTargetDetection,
            AUTOMATIC_DATA_FRAME_TRIGGER ? AUTOMATIC_DATA_TRIGER_TIME_US : 0);

        while (1)
        {
            // Fetch new target data
            res = ep_targetdetect_get_targets(protocolHandle, endpointTargetDetection);
            Sleep(10); // Avoid tight polling (10ms)
        }
    }

    // Cleanup
    closesocket(udp_sock);
    WSACleanup();
    return res;
}
