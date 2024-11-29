/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_now_config.h"
#include <sys/time.h>
#include "driver/uart.h"
#include "wifi_config/wifi_config.h"
#include "event_group_config.h"

#define ESPNOW_MAXDELAY 512
#define USER_MAXDEVICES 50

static const char *TAG = "espnow";

static QueueHandle_t s_espnow_queue;

static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t station_mac[ESP_NOW_ETH_ALEN];

static uint16_t s_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = {0, 0};

uint8_t total_nodedevices_num = 0;
static uint8_t total_nodedevices_mac[USER_MAXDEVICES][ESP_NOW_ETH_ALEN] = {0};

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    espnow_event_t evt;
    espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t *mac_addr = recv_info->src_addr;
    uint8_t *des_addr = recv_info->des_addr;

    if (mac_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (IS_BROADCAST_ADDR(des_addr))
    {
        /* If added a peer with encryption before, the receive packets may be
         * encrypted as peer-to-peer message or unencrypted over the broadcast channel.
         * Users can check the destination address to distinguish it.
         */
        ESP_LOGD(TAG, "Receive broadcast ESPNOW data");
    }
    else
    {
        ESP_LOGD(TAG, "Receive unicast ESPNOW data");
    }

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL)
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE)
    {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

/* Parse received ESPNOW data. */
int espnow_data_parse(uint8_t *data, uint8_t *payloadbuffer, uint16_t data_len, uint8_t *state, uint16_t *seq, uint32_t *magic, uint8_t *destmac)
{
    espnow_data_t *buf = (espnow_data_t *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(espnow_data_t))
    {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return -1;
    }

    memcpy(payloadbuffer, buf->payload, ESPNOW_PAYLOAD_LEN);
    memcpy(destmac, buf->dest_mac, ESP_NOW_ETH_ALEN);
    *seq = buf->seq_num;
    *magic = buf->magic;
    crc = buf->crc;
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    if (crc_cal == crc)
    {
        return buf->type;
    }

    return -1;
}

/* Prepare ESPNOW data to be sent. */
void espnow_data_prepare(espnow_send_param_t *send_param)
{
    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->seq_num = s_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    /* Fill all remaining bytes after the data with random values */
    esp_fill_random(buf->payload, send_param->len - sizeof(espnow_data_t));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

// 给节点配置ID并写入时间戳差值
void device_prov_prepare(espnow_send_param_t *send_param, uint8_t node_mac[])
{

    uint8_t prov_device_num = 0;
    for (int i = 0; i < 50; i++)
    {

        if ((memcmp(node_mac, total_nodedevices_mac[i], ESP_NOW_ETH_ALEN) != 0) && (total_nodedevices_mac[i][0] == 0)) // 如果保存的设备列表中没有此设备
        {
            total_nodedevices_num++;
            ESP_LOGI(TAG, "set " MACSTR " device id is %d", MAC2STR(node_mac), total_nodedevices_num);
            memcpy(total_nodedevices_mac[i], node_mac, ESP_NOW_ETH_ALEN);
            ESP_LOGI(TAG, "add " MACSTR " device to device list to No:%d", MAC2STR(total_nodedevices_mac[i]), i);
            prov_device_num = i;
            break;
        }
        else if (memcmp(node_mac, total_nodedevices_mac[i], ESP_NOW_ETH_ALEN) == 0)
        {
            prov_device_num = i;
            break;
        }
    }

    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(espnow_data_t));

    memset(buf, 0, CONFIG_ESPNOW_SEND_LEN); // clean the buffer

    memcpy(buf->dest_mac, node_mac, ESP_NOW_ETH_ALEN);
    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->seq_num = s_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;

    prov_device_num++; // 设备id从1开始
    memcpy(buf->payload, &prov_device_num, sizeof(uint8_t));

    uint32_t time_ms = esp_log_timestamp();

    memcpy(&buf->payload[1], &time_ms, sizeof(uint32_t));
    memcpy(&buf->payload[5], station_mac, sizeof(station_mac));

    extern char pc_addr_str[128];
    memcpy(&buf->payload[11],pc_addr_str,strlen(pc_addr_str));
    printf("send pc ip info:%s\n",&buf->payload[11]);

    extern char wifi_ssid[33];
    extern char wifi_password[65];
    memcpy(&buf->payload[28], wifi_ssid, strlen(wifi_ssid));
    memcpy(&buf->payload[28 + strlen((char *)wifi_ssid) + 1], wifi_password, strlen((char *)wifi_password));
    printf("send wifi info:\nssid is :%s\npassword is :%s ", &buf->payload[28], &buf->payload[28 + strlen((char *)wifi_ssid) + 1]);

    printf("prov pay load is :");
    for (int i = 0; i < CONFIG_ESPNOW_SEND_LEN; i++)
    {
        printf("%c", buf->payload[i]);
    }
    printf("\n");
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

static void espnow_task(void *pvParameter)
{
    espnow_event_t evt;
    uint8_t recv_state = 0;
    uint16_t recv_seq = 0;
    uint32_t recv_magic = 0;
    uint8_t recv_destmac[ESP_NOW_ETH_ALEN] = {0};
    uint8_t recv_payloadbuffer[ESPNOW_PAYLOAD_LEN - 15] = {0};

    int ret;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Start reveiving broadcast data");

    /* Start sending broadcast ESPNOW data. */
    espnow_send_param_t *send_param = (espnow_send_param_t *)pvParameter;

    while (xQueueReceive(s_espnow_queue, &evt, portMAX_DELAY) == pdTRUE)
    {
        switch (evt.id)
        {
        case EXAMPLE_ESPNOW_RECV_CB:
        {
            espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
            memset(recv_payloadbuffer, 0, sizeof(recv_payloadbuffer));
            ret = espnow_data_parse(recv_cb->data, recv_payloadbuffer, recv_cb->data_len, &recv_state, &recv_seq, &recv_magic, recv_destmac);
            free(recv_cb->data);
            if (ret == EXAMPLE_ESPNOW_DATA_BROADCAST)
            {
                if (memcmp(recv_destmac, station_mac, ESP_NOW_ETH_ALEN) == 0)
                {
                    // ESP_LOGI(TAG, "Receive %dth unicast data from: " MACSTR ", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);
                    //  printf("unicast payload is :");//data[9]开始为payload
                    // for (int i = 0; i < sizeof(recv_payloadbuffer); i++)
                    // {
                    //     printf("%02x", recv_payloadbuffer[i]);
                    // }
                    // printf("\n");
                }
                else
                {
                    ESP_LOGI(TAG, "Receive %dth broadcast data from: " MACSTR ", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

                    /* Delay a while before sending the next data. */
                    if (send_param->delay > 0)
                    {
                        vTaskDelay(send_param->delay / portTICK_PERIOD_MS);
                    }
                    memcpy(send_param->dest_mac, s_broadcast_mac, ESP_NOW_ETH_ALEN); // 广播报文，通过payload实现单播
                    device_prov_prepare(send_param, recv_cb->mac_addr);

                    ESP_LOGI(TAG, "send data to " MACSTR "", MAC2STR(send_param->buffer));

                    /* Send the next data after the previous data is sent. */
                    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Send error");
                    }
                }
            }
            else
            {
                ESP_LOGI(TAG, "Receive error data from: " MACSTR "", MAC2STR(recv_cb->mac_addr));
            }
            break;
        }
        default:
            ESP_LOGE(TAG, "Callback type error: %d", evt.id);
            break;
        }
    }
}

static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

}

esp_err_t espnow_init(void)
{
    extern EventGroupHandle_t gui_event_group;
    xEventGroupWaitBits(gui_event_group, LCD_GET_PCIP_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    ESP_ERROR_CHECK(esp_wifi_stop());

    ESP_ERROR_CHECK(esp_wifi_deinit());

    ESP_ERROR_CHECK(esp_event_loop_delete_default());

    example_wifi_init();

    espnow_send_param_t *send_param;

    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (s_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    esp_wifi_get_mac(ESP_IF_WIFI_STA, station_mac);
    ESP_LOGI(TAG, "my mac is " MACSTR "", MAC2STR(station_mac));

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(espnow_send_param_t));
    if (send_param == NULL)
    {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(send_param, 0, sizeof(espnow_send_param_t));
    send_param->unicast = false;
    send_param->broadcast = true;
    send_param->magic = esp_random();
    send_param->count = CONFIG_ESPNOW_SEND_COUNT;
    send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param->len = CONFIG_ESPNOW_SEND_LEN;
    send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
    if (send_param->buffer == NULL)
    {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param->dest_mac, s_broadcast_mac, ESP_NOW_ETH_ALEN);
    espnow_data_prepare(send_param);

    xEventGroupSetBits(gui_event_group, LCD_UPDATE_MAIN_BIT);//设置显示主界面

    xTaskCreate(espnow_task, "espnow_task", 4096, send_param, 4, NULL);

    return ESP_OK;
}