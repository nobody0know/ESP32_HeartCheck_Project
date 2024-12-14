/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "event_group_config.h"

#define PORT 3333

static const char *TAG = "UDP";

extern EventGroupHandle_t gui_event_group;

char pc_addr_str[128] = {0};

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];

    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    xEventGroupWaitBits(gui_event_group, LCD_WIFI_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    while (1)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        // printf("socket back is %d",sock);//debug
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 300;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (1)
        {
            ESP_LOGI(TAG, "Waiting for PC data");
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, pc_addr_str, sizeof(pc_addr_str) - 1);

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, pc_addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);

                struct sockaddr_in dest_addr;
                dest_addr.sin_addr.s_addr = inet_addr(pc_addr_str);
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(10000);
                addr_family = AF_INET;
                ip_protocol = IPPROTO_IP;

                int sock_request = socket(addr_family, SOCK_DGRAM, ip_protocol);
                // printf("socket back is %d",sock);//debug
                if (sock_request < 0)
                {
                    ESP_LOGE(TAG, "Unable to create request socket: errno %d", errno);
                    break;
                }
                ESP_LOGI(TAG, "Request socket created");

                char request_text[128] = "esp32";
                err = sendto(sock_request, request_text, sizeof(request_text), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }

                xEventGroupSetBits(gui_event_group, LCD_GET_PCIP_OK_BIT);

                shutdown(sock, 0);
                close(sock);

                vTaskDelete(NULL); // get pc ip ok delete this task
                break;
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void udp_app_init(void)
{
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}

char *get_pc_ip_addr(void)
{
    return pc_addr_str;
}