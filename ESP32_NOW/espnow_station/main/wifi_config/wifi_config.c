#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_mac.h"
#include "wifi_config.h"
#include "event_group_config.h"
#include "lvgl_gui.h"

#define WIFI_MAX_RETRY_TIMES 10

extern EventGroupHandle_t s_wifi_event_group;
extern EventGroupHandle_t gui_event_group;

uint8_t rvd_data[33] = {0};

char wifi_ssid[33] = {0};
char wifi_password[65] = {0};

uint8_t wifi_config_flag;
uint8_t wifi_disconnect_flag = 0;

static nvs_handle wifi_config_nvs_h;
static uint16_t wifi_retry_count = 0;
static char my_ip_addr[16] = {0};

static const char *TAG = "smartconfig";

static void smartconfig_task(void *parm);

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "wifi connect faile try to reconnect!");
        wifi_disconnect_flag = 1;
        wifi_retry_count++;
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);

        // printf("reset flag is %d",reset_flag);//debug
        extern uint8_t reset_flag;
        if (wifi_retry_count > WIFI_MAX_RETRY_TIMES && reset_flag == 0) // 刚开机触发首次联网失败且重连次数过多，在运行期重连失败不执行配网程序
        {
            ESP_LOGW(TAG, "wifi connect error turn to esptouch!");
            xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 5, NULL);
            wifi_retry_count = 0;
        }
        else
        {
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(my_ip_addr,""IPSTR"",IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        
        xEventGroupSetBits(gui_event_group, LCD_WIFI_OK_BIT);
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(TAG, "Scan done");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(TAG, "Found channel");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

        memcpy(wifi_ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(wifi_password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", wifi_ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", wifi_password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2)
        {
            ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i = 0; i < 33; i++)
            {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        if (wifi_config_flag == 0) // first need write wifi config
        {
            ESP_LOGI(TAG,"wifi_cfg update now... \n");
            wifi_config_flag = 1;
            ESP_ERROR_CHECK(nvs_set_str(wifi_config_nvs_h, "wifi_ssid", wifi_ssid));
            ESP_ERROR_CHECK(nvs_set_str(wifi_config_nvs_h, "wifi_passwd", wifi_password));
            ESP_ERROR_CHECK(nvs_set_u8(wifi_config_nvs_h, "wifi_update", wifi_config_flag));
            ESP_LOGI(TAG,"wifi_cfg update ok. \n");
            ESP_ERROR_CHECK(nvs_commit(wifi_config_nvs_h)); /* 提交 */
            nvs_close(wifi_config_nvs_h);                   /* 关闭 */
        }
        else // next power on just to need check wifi config
        {
            char nvs_wifi_ssid[33] = {0};
            char nvs_wifi_password[65] = {0};

            size_t size = sizeof(wifi_ssid);
            nvs_get_str(wifi_config_nvs_h, "wifi_ssid", nvs_wifi_ssid, &size);
            size = sizeof(wifi_password);
            nvs_get_str(wifi_config_nvs_h, "wifi_passwd", nvs_wifi_password, &size);
            ESP_LOGI(TAG, "NVS SSID:%s", nvs_wifi_ssid);
            ESP_LOGI(TAG, "NSV PASSWORD:%s", nvs_wifi_password);

            if (memcmp(wifi_ssid, nvs_wifi_ssid, sizeof(wifi_ssid)) && memcmp(wifi_password, nvs_wifi_password, sizeof(wifi_password)))
            {
                ESP_LOGI(TAG,"wifi_cfg update now... \n");
                ESP_ERROR_CHECK(nvs_set_str(wifi_config_nvs_h, "wifi_ssid", wifi_ssid));
                ESP_ERROR_CHECK(nvs_set_str(wifi_config_nvs_h, "wifi_passwd", wifi_password));
                ESP_LOGI(TAG,"wifi_cfg update ok. \n");
                ESP_ERROR_CHECK(nvs_commit(wifi_config_nvs_h)); /* 提交 */
                nvs_close(wifi_config_nvs_h);                   /* 关闭 */
            }
        }

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        ESP_LOGI(TAG, "ESP TOUCH OK");
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void smartconfig_task(void *parm)
{
    xEventGroupWaitBits(gui_event_group, LCD_INIT_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    show_esptouch_screen();
    EventBits_t uxBits;
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    while (1)
    {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if (uxBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smartconfig over");

            xEventGroupSetBits(gui_event_group, LCD_WIFI_OK_BIT);

            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void wifi_connect_task()
{
    s_wifi_event_group = xEventGroupCreate();
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READWRITE, &wifi_config_nvs_h));
    esp_err_t err = nvs_get_u8(wifi_config_nvs_h, "wifi_update", &wifi_config_flag);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGI(TAG,"wifi_cfg find value is %d. \n", wifi_config_flag);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAG,"wifi need to config\n");
        break;
    default:
        ESP_LOGI(TAG,"Error (%s) reading!\n", esp_err_to_name(err));
    }

    if (wifi_config_flag == 1)
    {
        size_t size = sizeof(wifi_ssid);
        nvs_get_str(wifi_config_nvs_h, "wifi_ssid", wifi_ssid, &size);
        size = sizeof(wifi_password);
        nvs_get_str(wifi_config_nvs_h, "wifi_passwd", wifi_password, &size);
        ESP_LOGI(TAG, "NVS SSID:%s", wifi_ssid);
        ESP_LOGI(TAG, "NSV PASSWORD:%s", wifi_password);
    }

    ESP_LOGI(TAG,"ESP_WIFI_MODE_STA \n");

    ESP_ERROR_CHECK(esp_netif_init());                // 用于初始化tcpip协议栈
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // 创建一个默认系统事件调度循环，之后可以注册回调函数来处理系统的一些事件
    esp_netif_create_default_wifi_sta();              // 使用默认配置创建STA对象

    // 初始化WIFI
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    // 设置账户和密码
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password));

    // 启动WIFI
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));               // 设置工作模式为STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // 设置wifi配置
    ESP_ERROR_CHECK(esp_wifi_start());                               // 启动WIFI

    if (wifi_config_flag == 0)
    {
        xTaskCreate(smartconfig_task, "smartconfig_task", 5120, NULL, 5, NULL);
    }

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    vTaskDelete(NULL);
}

void wifi_config_init(void)
{
    xTaskCreate(wifi_connect_task, "wifi_connect_task", 4096, NULL, 3, NULL);
}

char * get_ip_addr(void)
{
    return my_ip_addr;
}

uint8_t ifneed_smart_config(void)
{
    if(wifi_config_flag == 0)
        return 1;
    else return 0;
}