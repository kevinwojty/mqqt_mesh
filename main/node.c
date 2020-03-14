/*
 * node.c
 *
 *  Created on: 13 mar. 2020
 *      Author: kevin
 */
#include "mdf_common.h"
#include "mwifi.h"
#include "mqtt_client.h"
//#include "mesh_mqtt_handle.h"
#include "../headers/node.h"
#include "../headers/Mqtt_intr_cb.h"

extern const char *TAG;
extern SemaphoreHandle_t alarma_onoff_sem;
extern esp_mqtt_client_handle_t clientAdafruit;
extern char *CUARTO;

void root_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *rcv    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};
    node_msj data;

    const char s[2] = ",";

    MDF_LOGI("Root write task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT)
    {
        /**
         * @brief Recv data from node, and forward to mqtt server.
         */
        memset(rcv, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_root_read(src_addr, &data_type, rcv, &size, portMAX_DELAY);

        //Separo los campos que me enviaron separados por comas
        data.cuarto = strtok(rcv, s);
        data.topic = strtok(NULL, s);
        data.msj = strtok(NULL, s);

        if(strcmp(data.topic,TOPIC2) == 0)
        {
        	char alarma[30] = "Alarma en: ";

			strcat(alarma,data.cuarto);
			esp_mqtt_client_publish(clientAdafruit, data.topic , alarma, 0, 1, 0);
        }
    }

    MDF_LOGW("Root write task is exit");
	esp_mqtt_client_stop(clientAdafruit);
    vTaskDelete(NULL);
}

void node_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Node read task is running");

    for (;;)
    {
        if (!mwifi_is_connected())
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));
        MDF_LOGD("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);

        //if(strcmp(aux_topic,TOPIC1)==0)	//si es que se activo alguna alarma se la envio a todos
        //{
        	char *temp_on = NULL,*temp_off = NULL;

        	temp_on = MDF_MALLOC(sizeof(char)*25);
        	temp_off = MDF_MALLOC(sizeof(char)*25);

        	strcpy(temp_on,CUARTO);
        	strcat(temp_on," ON");
        	strcpy(temp_off,CUARTO);
        	strcat(temp_off," OFF");

            if(strcmp(data,temp_on) == 0)
            {
				xSemaphoreGive(alarma_onoff_sem);
				//Agrego la interrupción para un pin en particular del GPIO
				gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
				//xSemaphoreTake(pir_sem,1); //si no hubo movimiento que no sea bloqueante
            }
            else if(strcmp(data,temp_off) == 0)
			{
				gpio_isr_handler_remove(PIR_PIN);   //Evito que salte la interrupcion del pir
				xSemaphoreTake(alarma_onoff_sem,(TickType_t)1); //si ya esta apagado que no sea bloqueante
			}

    		free(temp_on);
    		free(temp_off);
           //Si es movimiento no hago nada ya que yo envie la notificacion
    }

    MDF_LOGW("Node read task is exit");
    MDF_FREE(data);
    vTaskDelete(NULL);
}

