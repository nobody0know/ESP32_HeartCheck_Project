#include "ble_mesh.h"

static int32_t time_flag_shift = 0;

static const char *TAG = "MESH_SERVER";

static uint16_t dev_local_id = 0;

static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = { 0x32, 0x10 };

static esp_ble_mesh_cfg_srv_t config_server = {
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
};

static esp_ble_mesh_model_op_t vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,
    vnd_op, NULL, NULL),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
};

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx 0x%03x, addr 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags 0x%02x, iv_index 0x%08" PRIx32, flags, iv_index);
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
            param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
            param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
            param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }
}

static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        switch (param->ctx.recv_op) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                param->value.state_change.appkey_add.net_idx,
                param->value.state_change.appkey_add.app_idx);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:\
            dev_local_id = param->value.state_change.mod_app_bind.element_addr-4;//---------------get node id
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                param->value.state_change.mod_app_bind.element_addr,
                param->value.state_change.mod_app_bind.app_idx,
                param->value.state_change.mod_app_bind.company_id,
                param->value.state_change.mod_app_bind.model_id);
            provision_led();
            break;
        default:
            break;
        }
    }
}

static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        //if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND) 
        {
            //char get_msg[param->model_operation.length];
            //memcpy(get_msg,(uint8_t *)param->model_operation.msg,sizeof(get_msg));
            //uint16_t tid = *(uint16_t *)param->model_operation.msg;
            ok_led();
            ESP_LOGI(TAG, "Recv 0x%06" PRIx32 ", msg is %ld", param->model_operation.opcode,*(uint32_t *)param->model_operation.msg);

            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            uint32_t time_ms = ((int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec)/1000;
            //if(*param->model_operation.msg > time_ms)
            {
                time_flag_shift = *(uint32_t *)param->model_operation.msg - time_ms;
            }

            ESP_LOGI(TAG,"SET time flag shift is %ld",time_flag_shift);
            // esp_err_t err = esp_ble_mesh_server_model_send_msg(&vnd_models[0],
            //         param->model_operation.ctx, ESP_BLE_MESH_VND_MODEL_OP_STATUS,
            //         sizeof(tid), (uint8_t *)&tid);
            // if (err) {
            //     ESP_LOGE(TAG, "Failed to send message 0x%06x", ESP_BLE_MESH_VND_MODEL_OP_STATUS);
            // }
        }

        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
            break;
        }
        //ESP_LOGI(TAG, "Send 0x%06" PRIx32, param->model_send_comp.opcode);
        break;
    default:
        break;
    }
}


void example_ble_mesh_send_vendor_message(bool resend,uint16_t lenght,uint8_t *data)
{
    esp_ble_mesh_msg_ctx_t ctx = {0};
    uint32_t opcode;
    esp_err_t err;

    ctx.addr = 0x1;
    ctx.send_ttl = MSG_SEND_TTL;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_STATUS;
 
    //向client上报消息，其他server收不到
    err = esp_ble_mesh_server_model_send_msg(&vnd_models[0],&ctx, opcode,lenght, (uint8_t *)data);

    if (err) {
        ESP_LOGE(TAG, "Failed to send message 0x%06x", ESP_BLE_MESH_VND_MODEL_OP_STATUS);
    }
    //向其他server发送消息，client收不到
    //err = esp_ble_mesh_server_model_send_msg(&vnd_models[0],&ctx, ESP_BLE_MESH_VND_MODEL_OP_SEND,lenght, (uint8_t *)data);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
        return;
    }
   
}


static esp_err_t ble_mesh_init(void)
{
    esp_err_t err;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack");
        return err;
    }

    err = esp_ble_mesh_node_prov_enable((esp_ble_mesh_prov_bearer_t)(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh node");
        return err;
    }

    ESP_LOGI(TAG, "BLE Mesh Node initialized");

    return ESP_OK;
}

uint8_t SendMessageFlag = 0;
void ble_button_set()
{
    if(SendMessageFlag==0) SendMessageFlag = 1;
    else SendMessageFlag = 0;
}

void  myitoa(int num, char* str, int radix)
{
    int i = 0;
    int sum;
    unsigned int num1 = num;  //如果是负数求补码，必须将他的绝对值放在无符号位中在进行求反码
    char str1[33] = { 0 };
    if (num<0) {              //求出负数的补码
        num = -num;
        num1 = ~num;
        num1 += 1;
    }
    if (num == 0) {             
        str1[i] = '0';
        
        i++;
    }
    while(num1 !=0) {                      //进行进制运算
        sum = num1 % radix;
        str1[i] = (sum > 9) ? (sum - 10) + 'a' : sum + '0';
        num1 = num1 / radix;
        i++;
    }
    i--;
    int j = 0;
    for (; i >= 0; i--) {               //逆序输出 
        str[i] = str1[j];
        j++;
    }
    
}


void BleMesh_Task(void* param) 
{
    esp_err_t err;
    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
    while(1)
    {
        if(SendMessageFlag == 1) {
            static char send_data[400];
            memset(send_data,'0',sizeof(send_data));
            sprintf(send_data,"dev[%2d]msg:",dev_local_id);
            int test = 222;
            // for (int i = 0; i < 10; i++)
            // {

                 myitoa(test,&send_data[11],10);
            //     //printf("add num:%s",&send_data[11 + i*5]);
            // }
            send_data[200] = '\0';
            printf("my id is:%d\n",dev_local_id);
            //SendMessageFlag = 0;
            printf("Send [%d]byte msg to client:%s\n",strlen(send_data),send_data);
            example_ble_mesh_send_vendor_message(false,strlen(send_data),(uint8_t*)send_data);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}