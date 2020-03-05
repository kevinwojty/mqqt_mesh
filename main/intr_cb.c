/*
 * intr_cb.c
 *
 *  Created on: 4 mar. 2020
 *      Author: kevin
 */

#include "mdf_common.h"
#include "mwifi.h"
#include "mesh_mqtt_handle.h"
#include "../headers/Mqtt_intr_cb.h"

const char *TAG2 = "mqtt_adafruit";

extern const char *TAG;
extern esp_mqtt_client_handle_t clientAdafruit;
extern SemaphoreHandle_t pir_sem,alarma_onoff_sem;

esp_err_t mqtt_event_handler_adafuit(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char *aux_topic,*aux_data;
    mdf_err_t ret = MDF_OK;
    mwifi_data_type_t data_type = {0x0};
    char *data    = NULL;
    uint8_t dest_addr[MWIFI_ADDR_LEN] = MWIFI_ADDR_BROADCAST;

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG2, "MQTT_EVENT_CONNECTED");

            msg_id = esp_mqtt_client_subscribe(client, TOPIC1, 1);
            ESP_LOGI(TAG2, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, TOPIC2, 1);
            ESP_LOGI(TAG2, "sent subscribe successful, msg_id=%d", msg_id);

            break;


        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG2, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG2, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG2, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG2, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:

            ESP_LOGI(TAG2, "MQTT_EVENT_DATA");

            //Analizo la información que llego
            asprintf(&aux_topic,"%.*s",event->topic_len, event->topic);
            /**
             * @brief Send mqtt server information to nodes throught root node.
             */
            if(strcmp(aux_topic,TOPIC1)==0)
            {
                MDF_LOGD("Node send, size: %d, data: %s", event->data_len, event->data);

                ret = mwifi_root_write(dest_addr, 1, &data_type, event->data, event->data_len, true);
                MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_write", mdf_err_to_name(ret));

MEM_FREE:
		MDF_FREE(data);

				//xSemaphoreTake(alarma_onoff_sem,portMAX_DELAY); //Dejo el led cte PRUEBA

                /*
                if(strcmp(aux_data,CUARTO " ON") == 0)
                {
                    xSemaphoreGive(alarma_onoff_sem);
                    //Agrego la interrupción para un pin en particular del GPIO
                    gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
                    xSemaphoreTake(pir_sem,1); //si no hubo movimiento que no sea bloqueante
                }
                if(strcmp(aux_data,CUARTO " OFF") == 0)
                {
                    gpio_isr_handler_remove(PIR_PIN);   //Evito que salte la interrupcion del pir
                    xSemaphoreTake(alarma_onoff_sem,1); //si ya esta apagado que no sea bloqueante
                }*/
            }   //Si es movimiento no hago nada ya que yo envie la notificacion

            MDF_FREE(aux_topic);

            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG2, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG2, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(pir_sem, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken != pdFALSE)
    {
        portYIELD_FROM_ISR();   //obligo a que se vuelva a ejecutar el scheduler si la tarea liberada es de mayor prioridad
    }
}

/**
 * @brief Timed printing system information
 */
void print_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    mesh_assoc_t mesh_assoc         = {0x0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_wifi_vnd_mesh_get(&mesh_assoc);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u", primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mesh_assoc.rssi, esp_mesh_get_total_node_num(), esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

#ifdef MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
    mdf_mem_print_task();
#endif /**< MEMORY_DEBUG */
}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);
    static node_list_t node_list = {0x0};

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            if (esp_mesh_is_root()) {
            	esp_mqtt_client_stop(clientAdafruit);
            	//mesh_mqtt_stop();
            }

            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_ADD, total_num: %d", esp_mesh_get_total_node_num());
/*
            if (esp_mesh_is_root()) {


                // @brief find new add device.

                node_list.change_num  = esp_mesh_get_routing_table_size();
                node_list.change_list = MDF_MALLOC(node_list.change_num * sizeof(mesh_addr_t));
                ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)node_list.change_list,
                                node_list.change_num * sizeof(mesh_addr_t), (int *)&node_list.change_num));

                for (int i = 0; i < node_list.last_num; ++i) {
                    addrs_remove(node_list.change_list, &node_list.change_num, node_list.last_list + i * MWIFI_ADDR_LEN);
                }

                node_list.last_list = MDF_REALLOC(node_list.last_list,
                                                  (node_list.change_num + node_list.last_num) * MWIFI_ADDR_LEN);
                memcpy(node_list.last_list + node_list.last_num * MWIFI_ADDR_LEN,
                       node_list.change_list, node_list.change_num * MWIFI_ADDR_LEN);
                node_list.last_num += node_list.change_num;


                // @brief subscribe topic for new node

                MDF_LOGI("total_num: %d, add_num: %d", node_list.last_num, node_list.change_num);
                mesh_mqtt_subscribe(node_list.change_list, node_list.change_num);
                MDF_FREE(node_list.change_list);
            }
*/
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE, total_num: %d", esp_mesh_get_total_node_num());
/*
            if (esp_mesh_is_root()) {

                // @brief find removed device.

                size_t table_size      = esp_mesh_get_routing_table_size();
                uint8_t *routing_table = MDF_MALLOC(table_size * sizeof(mesh_addr_t));
                ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                                table_size * sizeof(mesh_addr_t), (int *)&table_size));

                for (int i = 0; i < table_size; ++i) {
                    addrs_remove(node_list.last_list, &node_list.last_num, routing_table + i * MWIFI_ADDR_LEN);
                }

                node_list.change_num  = node_list.last_num;
                node_list.change_list = MDF_MALLOC(node_list.last_num * MWIFI_ADDR_LEN);
                memcpy(node_list.change_list, node_list.last_list, node_list.change_num * MWIFI_ADDR_LEN);

                node_list.last_num  = table_size;
                memcpy(node_list.last_list, routing_table, table_size * MWIFI_ADDR_LEN);
                MDF_FREE(routing_table);


                // @brief unsubscribe topic for leaved node

                MDF_LOGI("total_num: %d, add_num: %d", node_list.last_num, node_list.change_num);
                mesh_mqtt_unsubscribe(node_list.change_list, node_list.change_num);
                MDF_FREE(node_list.change_list);
            }
*/
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {

            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");

            mqtt_app_start();
/*
            mesh_mqtt_start(CONFIG_MQTT_URL);


             // @brief subscribe topic for all subnode

            size_t table_size      = esp_mesh_get_routing_table_size();
            uint8_t *routing_table = MDF_MALLOC(table_size * sizeof(mesh_addr_t));
            ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                            table_size * sizeof(mesh_addr_t), (int *)&table_size));

            node_list.last_num  = table_size;
            node_list.last_list = MDF_REALLOC(node_list.last_list,
                                              node_list.last_num * MWIFI_ADDR_LEN);
            memcpy(node_list.last_list, routing_table, table_size * MWIFI_ADDR_LEN);
            MDF_FREE(routing_table);

            MDF_LOGI("subscribe %d node", node_list.last_num);
            mesh_mqtt_subscribe(node_list.last_list, node_list.last_num);
            MDF_FREE(node_list.change_list);
*/
            xTaskCreate(root_write_task, "root_write", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            //xTaskCreate(root_read_task, "root_read", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL) //PRUEBA
            break;
        }

        case MDF_EVENT_CUSTOM_MQTT_CONNECT:
            MDF_LOGI("MQTT connect");
            mwifi_post_root_status(true);
            break;

        case MDF_EVENT_CUSTOM_MQTT_DISCONNECT:
            MDF_LOGI("MQTT disconnected");
            mwifi_post_root_status(false);
            break;

        default:
            break;
    }

    return MDF_OK;
}
