/*
 * intr_cb.c
 *
 *  Created on: 4 mar. 2020
 *      Author: kevin
 */

#include "mdf_common.h"
#include "mwifi.h"
#include "mqtt_client.h"
//#include "mesh_mqtt_handle.h"
#include "../headers/mqtt.h"
#include "../headers/Mqtt_intr_cb.h"
#include "../headers/fnvs.h"
#include "mconfig_blufi.h"

extern const char *TAG;
extern const char *TAG2;
extern esp_mqtt_client_handle_t clientAdafruit;
extern SemaphoreHandle_t pir_sem,alarma_onoff_sem, config_sem;
extern DRAM_ATTR char CUARTO[20];
extern QueueHandle_t estado_nodos;

esp_err_t mqtt_event_handler_adafuit(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char *aux_topic,*aux_data;
    //mdf_err_t ret = MDF_OK;
    mwifi_data_type_t data_type = {0x0};
    //char *data    = NULL;
    uint8_t dest_addr[MWIFI_ADDR_LEN] = MWIFI_ADDR_BROADCAST;

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG2, "MQTT_EVENT_CONNECTED");

            msg_id = esp_mqtt_client_subscribe(client, TOPIC_ESTADO, 1);
            ESP_LOGI(TAG2, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, TOPIC_ENCENDIDO, 1);
            ESP_LOGI(TAG2, "sent subscribe successful, msg_id=%d", msg_id);

    		state_nodes();	//Solicito que se me envie el ultimo estado de todos los nodos

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
            asprintf(&aux_data,"%.*s",event->data_len, event->data);

            /**
             * @brief Send mqtt server information to nodes throught root node.
             */
            if(strcmp(aux_topic,TOPIC_ENCENDIDO)==0)	//si es que se encendio alguna alarma
            {
            	char *temp_on = NULL,*temp_off = NULL, *send_str = NULL;

            	MDF_LOGD("Node send, size: %d, data: %s", event->data_len,aux_data);

            	temp_on = MDF_MALLOC(sizeof(char)*25);
            	temp_off = MDF_MALLOC(sizeof(char)*25);
            	//send_str = MDF_MALLOC(sizeof(char)*45);

            	strcpy(temp_on,CUARTO);
            	strcat(temp_on," ON");
            	strcpy(temp_off,CUARTO);
            	strcat(temp_off," OFF");

            	ESP_LOGI(TAG2, "%s=%s",aux_data,temp_on);

                if(strcmp(aux_data,temp_on) == 0)	//Me fijo si el mensaje es para el root
                {
					xSemaphoreGive(alarma_onoff_sem);
					set_lastState_nvs(1);
					//Agrego la interrupción para un pin en particular del GPIO
					gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
					//xSemaphoreTake(pir_sem,1); //si no hubo movimiento que no sea bloqueante
                }
                else if(strcmp(aux_data,temp_off) == 0)
				{
					gpio_isr_handler_remove(PIR_PIN);   //Evito que salte la interrupcion del pir
					set_lastState_nvs(0);
					xSemaphoreTake(alarma_onoff_sem,(TickType_t)1); //si ya esta apagado que no sea bloqueante
				}
            	else	//caso que no sea se lo re envio a los nodos
            	{
            	    size_t size   = 0;
            	    char *data    = NULL;

                    size = asprintf(&data,"%s,%s,%s",aux_data,TOPIC_ENCENDIDO,"no hay msj");
            		mwifi_root_write(dest_addr, 1, &data_type, data, size, true);
                    MDF_LOGI("Enviado: %s",data);
            		//MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_write", mdf_err_to_name(ret));
            	}
        		free(temp_on);
        		free(temp_off);
        		free(send_str);
//MEM_FREE:
		//MDF_FREE(data);
            }   //Si es movimiento no hago nada ya que yo envie la notificacion
            else if (strcmp(aux_topic,TOPIC_ESTADO) == 0)
            {
            	xQueueOverwrite(estado_nodos,aux_data);
            	ESP_LOGI(TAG2, "Se escribio en la cola: %s",aux_data);
            }
            MDF_FREE(aux_topic);
            MDF_FREE(aux_data);

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
    uint32_t gpio_num = (uint32_t) arg;

    if(gpio_num == PIR_PIN)
    {
		xSemaphoreGiveFromISR(pir_sem, &xHigherPriorityTaskWoken);
    }
    else if(gpio_num == PUL_BOOT)
    {
		xSemaphoreGiveFromISR(config_sem, &xHigherPriorityTaskWoken);
    }
	if(xHigherPriorityTaskWoken != pdFALSE)
	{
		portYIELD_FROM_ISR();   //obligo a que se vuelva a ejecutar el scheduler si la tarea liberada es de mayor prioridad
	}
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
    //static node_list_t node_list = {0x0};

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");	//aca entra cuando vuelve a conectarse a su padre
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface"); //aca entra cuando se desconecta el padre

            if (esp_mesh_is_root()) {
            	esp_mqtt_client_stop(clientAdafruit);
            }

            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_ADD, total_num: %d", esp_mesh_get_total_node_num());
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE, total_num: %d", esp_mesh_get_total_node_num());
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {

            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");

            if (esp_mesh_is_root())
            {
            	mqtt_app_start();
                xTaskCreate(root_write_task, "root_write", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            }
            break;
        }

        case MDF_EVENT_MCONFIG_BLUFI_CONNECTED:
            MDF_LOGI("MDF_EVENT_MCONFIG_BLUFI_CONNECTED");
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED:
            MDF_LOGI("MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED");
            break;

        case MDF_EVENT_MWIFI_TODS_STATE:
            MDF_LOGI("MDF_EVENT_MWIFI_TODS_STATE");
            gpio_set_level(LED_BUILT_IN,1); //prendo el led como indicador de que el sistema arranco
            break;

		/**< Add a custom communication process */
		case MDF_EVENT_MCONFIG_BLUFI_RECV: {
			mconfig_blufi_data_t *blufi_data = (mconfig_blufi_data_t *)ctx;
			MDF_LOGI("recv data: %.*s", blufi_data->size, blufi_data->data);

			// ret = mconfig_blufi_send(blufi_data->data, blufi_data->size);
			// MDF_ERROR_BREAK(ret != MDF_OK, "<%> mconfig_blufi_send", mdf_err_to_name(ret));
			break;
		}
/*
        case MDF_EVENT_CUSTOM_MQTT_CONNECT:
            MDF_LOGI("MQTT connect");
            mwifi_post_root_status(true);
            break;

        case MDF_EVENT_CUSTOM_MQTT_DISCONNECT:
            MDF_LOGI("MQTT disconnected");
            mwifi_post_root_status(false);
            break;
*/
        default:
            break;
    }

    return MDF_OK;
}
