/*
 ===================================================================================
 Name        : extract_combined_data.c
 Author      : Combined from Infineon Technologies examples
 Version     : 1.0
 Description : Combined example for extracting both raw data and target data
               with command line options to choose mode
 ===================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "include/Protocol.h"
#include "include/COMPort.h"
#include "include/EndpointRadarBase.h"
#include "include/EndpointTargetDetection.h"

#define AUTOMATIC_DATA_FRAME_TRIGGER (1)
#define RAW_DATA_TRIGGER_TIME_US (1000000)     // 1 second for raw data
#define TARGET_DATA_TRIGGER_TIME_US (100000000) // 100 seconds for target data

// Mode flags
typedef enum {
    MODE_RAW_ONLY = 1,
    MODE_TARGET_ONLY = 2,
    MODE_BOTH = 3
} extraction_mode_t;

// Global variables
static extraction_mode_t g_mode = MODE_BOTH;
static uint32_t counter = 0;

#ifdef _WIN32
static SOCKET udp_sock = INVALID_SOCKET;
static struct sockaddr_in udp_addr;
#endif

// Raw data callback - called when raw ADC data is received
void received_frame_data(void* context,
                        int32_t protocol_handle,
                        uint8_t endpoint,
                        const Frame_Info_t* frame_info)
{
    if (g_mode & MODE_RAW_ONLY) {
        printf("=== RAW DATA FRAME ===\n");
        printf("Samples per chirp: %u\n", frame_info->num_samples_per_chirp);
        
        // Print first 10 samples to avoid overwhelming output
        uint32_t samples_to_print = (frame_info->num_samples_per_chirp > 10) ? 10 : frame_info->num_samples_per_chirp;
        for (uint32_t i = 0; i < samples_to_print; i++) {
            printf("ADC sample %d: %f\n", i, frame_info->sample_data[i]);
        }
        if (frame_info->num_samples_per_chirp > 10) {
            printf("... (%u more samples)\n", frame_info->num_samples_per_chirp - 10);
        }
        printf("=====================\n\n");
    }
}

// Target data callback - called when target detection data is received
void received_target_data(void* context,
                         int32_t protocol_handle,
                         uint8_t endpoint,
                         const Target_Info_t* target_info, 
                         uint8_t num_targets)
{
    if (g_mode & MODE_TARGET_ONLY) {
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

#ifdef _WIN32
        // Send over UDP every 20th frame to reduce traffic
        if (counter % 20 == 0 && udp_sock != INVALID_SOCKET) {
            sendto(udp_sock, buffer, (int)strlen(buffer), 0,
                   (struct sockaddr*)&udp_addr, sizeof(udp_addr));
        }
#endif

        // Print to console
        printf("=== TARGET DATA ===\n%s\n===================\n\n", buffer);
        counter++;
    }
}

int radar_auto_connect(void)
{
    int radar_handle = 0;
    int num_of_ports = 0;
    char comp_port_list[256];
    char* comport;
    const char *delim = ";";

    num_of_ports = com_get_port_list(comp_port_list, (size_t)256);

    if (num_of_ports == 0) {
        printf("No COM ports found.\n");
        return -1;
    }

    comport = strtok(comp_port_list, delim);
    while (num_of_ports > 0) {
        num_of_ports--;
        printf("Trying to connect to: %s\n", comport);
        
        radar_handle = protocol_connect(comport);
        if (radar_handle >= 0) {
            printf("Successfully connected to radar on port: %s\n", comport);
            break;
        }

        comport = strtok(NULL, delim);
    }

    return radar_handle;
}

#ifdef _WIN32
int init_udp_socket(void)
{
    WSADATA wsaData;
    
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return -1;
    }

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock == INVALID_SOCKET) {
        printf("UDP socket creation failed.\n");
        WSACleanup();
        return -1;
    }

    // Setup UDP address (loopback, port 5000)
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &udp_addr.sin_addr);
    
    printf("UDP socket initialized for 127.0.0.1:5000\n");
    return 0;
}

void cleanup_udp_socket(void)
{
    if (udp_sock != INVALID_SOCKET) {
        closesocket(udp_sock);
        udp_sock = INVALID_SOCKET;
    }
    WSACleanup();
}
#endif

void print_usage(const char* program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -r, --raw-only     Extract raw data only\n");
    printf("  -t, --target-only  Extract target data only\n");
    printf("  -b, --both         Extract both raw and target data (default)\n");
    printf("  -h, --help         Show this help message\n");
    printf("\nExample:\n");
    printf("  %s --raw-only      # Extract only raw ADC data\n", program_name);
    printf("  %s --target-only   # Extract only target detection data\n", program_name);
    printf("  %s --both          # Extract both types of data\n", program_name);
}

int main(int argc, char* argv[])
{
    int res = -1;
    int protocolHandle = 0;
    int endpointRadarBase = -1;
    int endpointTargetDetection = -1;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--raw-only") == 0) {
            g_mode = MODE_RAW_ONLY;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--target-only") == 0) {
            g_mode = MODE_TARGET_ONLY;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--both") == 0) {
            g_mode = MODE_BOTH;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    printf("Starting combined radar data extraction...\n");
    printf("Mode: ");
    switch (g_mode) {
        case MODE_RAW_ONLY:
            printf("Raw data only\n");
            break;
        case MODE_TARGET_ONLY:
            printf("Target data only\n");
            break;
        case MODE_BOTH:
            printf("Both raw and target data\n");
            break;
    }

#ifdef _WIN32
    // Initialize UDP socket if target mode is enabled
    if (g_mode & MODE_TARGET_ONLY) {
        if (init_udp_socket() < 0) {
            printf("Failed to initialize UDP socket.\n");
            return -1;
        }
    }
#endif

    // Connect to radar
    protocolHandle = radar_auto_connect();
    if (protocolHandle < 0) {
        printf("Failed to connect to radar.\n");
#ifdef _WIN32
        if (g_mode & MODE_TARGET_ONLY) {
            cleanup_udp_socket();
        }
#endif
        return -1;
    }

    // Find endpoints
    printf("Scanning for endpoints...\n");
    for (int i = 1; i <= protocol_get_num_endpoints(protocolHandle); ++i) {
        if (ep_radar_base_is_compatible_endpoint(protocolHandle, i) == 0) {
            endpointRadarBase = i;
            printf("Found radar base endpoint: %d\n", i);
        }
        if (ep_targetdetect_is_compatible_endpoint(protocolHandle, i) == 0) {
            endpointTargetDetection = i;
            printf("Found target detection endpoint: %d\n", i);
        }
    }

    // Setup raw data extraction
    if ((g_mode & MODE_RAW_ONLY) && endpointRadarBase >= 0) {
        printf("Setting up raw data extraction...\n");
        ep_radar_base_set_callback_data_frame(received_frame_data, NULL);
        
        if (AUTOMATIC_DATA_FRAME_TRIGGER) {
            res = ep_radar_base_set_automatic_frame_trigger(protocolHandle,
                                                           endpointRadarBase,
                                                           RAW_DATA_TRIGGER_TIME_US);
            printf("Raw data automatic trigger set to %d us\n", RAW_DATA_TRIGGER_TIME_US);
        }
    }

    // Setup target data extraction
    if ((g_mode & MODE_TARGET_ONLY) && endpointTargetDetection >= 0) {
        printf("Setting up target data extraction...\n");
        ep_targetdetect_set_callback_target_processing(received_target_data, NULL);
        
        if (AUTOMATIC_DATA_FRAME_TRIGGER) {
            res = ep_radar_base_set_automatic_frame_trigger(protocolHandle,
                                                           endpointTargetDetection,
                                                           TARGET_DATA_TRIGGER_TIME_US);
            printf("Target data automatic trigger set to %d us\n", TARGET_DATA_TRIGGER_TIME_US);
        }
    }

    printf("Starting data extraction loop...\n");
    printf("Press Ctrl+C to stop.\n\n");

    // Main extraction loop
    while (1) {
        // Get raw data if enabled
        if ((g_mode & MODE_RAW_ONLY) && endpointRadarBase >= 0) {
            res = ep_radar_base_get_frame_data(protocolHandle, endpointRadarBase, 1);
        }

        // Get target data if enabled
        if ((g_mode & MODE_TARGET_ONLY) && endpointTargetDetection >= 0) {
            res = ep_targetdetect_get_targets(protocolHandle, endpointTargetDetection);
        }

#ifdef _WIN32
        Sleep(10); // Small delay to prevent excessive CPU usage
#else
        usleep(10000); // 10ms delay for non-Windows systems
#endif
    }

    // Cleanup
#ifdef _WIN32
    if (g_mode & MODE_TARGET_ONLY) {
        cleanup_udp_socket();
    }
#endif

    return res;
}