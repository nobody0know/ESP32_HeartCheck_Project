#include "esp_now_app.h"

#define ESPNOW_MAXDELAY 512

static const char *TAG = "espnow_example";

static QueueHandle_t s_espnow_queue;

static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t station_mac[ESP_NOW_ETH_ALEN]; // = {0x88, 0x13, 0xbf, 0x0a, 0xc9, 0xe0};
static uint8_t node_mac[ESP_NOW_ETH_ALEN];
static uint16_t s_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = {0, 0};

uint16_t device_id = 0;
uint32_t time_flag_gap = 0;
extern QueueHandle_t ADC_queue;

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void espnow_data_prepare(espnow_send_param_t *send_param);
static void espnow_task(void *pvParameter);
static void espnow_deinit(espnow_send_param_t *send_param);
void espnow_send_node_data();

esp_err_t espnow_init(void)
{
    espnow_send_param_t *send_param;
    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (s_espnow_queue == NULL)
    {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    esp_wifi_get_mac(ESP_IF_WIFI_STA, node_mac);

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK(esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW));
    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL));
#endif
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

    // esp_wifi_set_ps(WIFI_PS_MAX_MODEM);//配网完成后进入休眠模式

    xTaskCreate(espnow_task, "espnow_task", 4096, send_param, 4, NULL);

    return ESP_OK;
}

static void espnow_deinit(espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_espnow_queue);
    esp_now_deinit();
}

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

    memcpy(payloadbuffer, buf->payload, ESPNOW_RECEIVE_PAYLOAD_LEN);
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

/* Prepare ESPNOW test data to be sent. */
void espnow_data_prepare(espnow_send_param_t *send_param)
{
    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->seq_num = s_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    /* Fill all remaining bytes after the data with random values */
    // esp_fill_random(buf->payload, send_param->len - sizeof(espnow_data_t));
    memset(buf->payload, device_id, send_param->len - sizeof(espnow_data_t));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

// 填充ADC数据报文，为减少发送频率，将5次测量结果放一帧发送，报文总长为 帧头9byte + 5 * 8byte = 49
int espnow_node_data_prepare(espnow_send_param_t *send_param)
{
    // payload_msg msg;
    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->seq_num = s_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    memcpy(buf->dest_mac, station_mac, ESP_NOW_ETH_ALEN);

    buf->payload[0] = 0xAA;

    memset(&buf->payload[1], device_id, sizeof(device_id));

    // printf("data waiting to be read : %d available spaces: %d \n",uxQueueMessagesWaiting(ADC_queue),uxQueueSpacesAvailable(ADC_queue));//队列剩余空间//队列中待读取的消息uxQueueSpacesAvailable(ADC_queue));//队列剩余空间

    if (uxQueueMessagesWaiting(ADC_queue) == 0)
    {
        ESP_LOGE(TAG, "adc queue may have something error");
        return 0;
    }
    uint8_t rx_buffer[100];
    for (int i = 0; i < 25; i++)
    {
        memset(rx_buffer, 0, sizeof(rx_buffer));
        if (xQueueReceive(ADC_queue, rx_buffer, 10))
        {
            memcpy(&buf->payload[3 + i * 6], rx_buffer, 6);
            payload_msg *temp = (payload_msg *)rx_buffer;
            // ESP_LOGI(TAG, "[%ld] ADC Channel[0] Cali Voltage: %ld mV", temp->payload_data.timestamp,temp->payload_data.adc_value);
        }
    }
    // printf("data waiting to be read : %d available spaces: %d \n",uxQueueMessagesWaiting(ADC_queue),uxQueueSpacesAvailable(ADC_queue));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
    return 1;
}

void espnow_send_node_data(espnow_send_param_t *send_param)
{
    extern bool usermsg_send_start;
    if (usermsg_send_start == 1)
    {
        // usermsg_send_start = 0;
        /* Delay a while before sending the next data. */
        if (send_param->delay > 0)
        {
            vTaskDelay(send_param->delay / portTICK_PERIOD_MS);
        }

        send_param->broadcast = true;
        send_param->unicast = false;
        memcpy(send_param->dest_mac, s_broadcast_mac, ESP_NOW_ETH_ALEN);
        if (espnow_node_data_prepare(send_param))
        {
            /* Send the next data after the previous data is sent. */
            esp_err_t esp_now_send_err;
            esp_now_send_err = esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len);
            if (esp_now_send_err != ESP_OK)
            {
                ESP_LOGE(TAG, "Send error reason is :%x", esp_now_send_err);
            }
        }
        else
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    else
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void espnow_task(void *pvParameter)
{
    espnow_event_t evt;
    uint8_t recv_state = 0;
    uint16_t recv_seq = 0;
    uint32_t recv_magic = 0;
    uint8_t recv_destmac[ESP_NOW_ETH_ALEN] = {0};
    uint8_t recv_payloadbuffer[ESPNOW_RECEIVE_PAYLOAD_LEN] = {0};
    int ret;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "my mac addr is " MACSTR " ", MAC2STR(node_mac));
    ESP_LOGI(TAG, "Start sending broadcast data");

    /* Start sending broadcast ESPNOW data. */
    espnow_send_param_t *send_param = (espnow_send_param_t *)pvParameter;
    if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK)
    {
        ESP_LOGE(TAG, "Send error");
        espnow_deinit(send_param);
        vTaskDelete(NULL);
    }

    while (1)
    {
        // espnow_send_node_data(send_param);
        vTaskDelay(pdMS_TO_TICKS(100));
        if (xQueueReceive(s_espnow_queue, &evt, 0) == pdTRUE)
        {
            switch (evt.id)
            {
            case EXAMPLE_ESPNOW_RECV_CB:
            {
                espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

                ret = espnow_data_parse(recv_cb->data, recv_payloadbuffer, recv_cb->data_len, &recv_state, &recv_seq, &recv_magic, recv_destmac);
                free(recv_cb->data);

                if (ret == EXAMPLE_ESPNOW_DATA_BROADCAST)
                {
                    if (memcmp(recv_destmac, node_mac, ESP_NOW_ETH_ALEN) == 0)
                    {
                        ESP_LOGI(TAG, "get my data from brodcast msg");

                        ESP_LOGI(TAG, "Receive %dth unicast data from: " MACSTR ", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

                        ESP_LOGI(TAG, "get msg to " MACSTR "", MAC2STR(recv_destmac));
                        printf("unicast payload is :");
                        for (int i = 0; i < 9; i++)
                        {
                            printf(" %02x", recv_payloadbuffer[i]);
                        }
                        printf("\n");
                        device_id = recv_payloadbuffer[0];

                        uint32_t time_ms = esp_log_timestamp();
                        printf("now timestemp is %ld\n", time_ms);
                        uint32_t get_timestemp = ((((uint32_t)recv_payloadbuffer[4] << 24) | ((uint32_t)recv_payloadbuffer[3] << 16) | ((uint32_t)recv_payloadbuffer[2] << 8) | (uint32_t)recv_payloadbuffer[1]));
                        printf("get timestemp is %ld\n", get_timestemp);
                        time_flag_gap = get_timestemp - time_ms;

                        ESP_LOGI(TAG, "SET time flag shift is %ld", time_flag_gap);

                        memcpy(station_mac, &recv_payloadbuffer[5], sizeof(station_mac));
                        ESP_LOGI(TAG, "SET station mac is " MACSTR " ", MAC2STR(station_mac));

                        extern char ssid[33];
                        extern char password[65];
                        extern char pc_ip[128];
                        memcpy(pc_ip,&recv_payloadbuffer[11],strlen((char*)&recv_payloadbuffer[11]));
                        ESP_LOGI(TAG, "SET PC IP is %s", pc_ip);

                        char ssid_buffer[33];
                        char password_buffer[65];
                        memcpy(ssid_buffer,&recv_payloadbuffer[28],sizeof(ssid));
                        sprintf(ssid,"%s",ssid_buffer);
                        memcpy(password_buffer,&recv_payloadbuffer[28+ strlen((char*)ssid)+1],sizeof(password_buffer));
                        sprintf(password,"%s",password_buffer);
                        // memcpy(ssid,&recv_payloadbuffer[11], strlen((char *)&recv_payloadbuffer[11]));
                        // memcpy(password,&recv_payloadbuffer[11 + strlen((char*)ssid)+1], strlen((char *)&recv_payloadbuffer[11 + strlen((char*)ssid)]+1));
                        ESP_LOGI(TAG, "GET WIFI ssid:%s and password:%s", ssid, password);
                        espnow_deinit(send_param);
                        wifi_connect_config_reinit();
                        vTaskDelete(NULL);
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
}
