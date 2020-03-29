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
#include "../headers/fnvs.h"


extern const char *TAG;
extern SemaphoreHandle_t alarma_onoff_sem;
extern esp_mqtt_client_handle_t clientAdafruit;
extern DRAM_ATTR char CUARTO[20];
extern QueueHandle_t estado_nodos_int;


void root_write_task(void *arg)
{
    //mdf_err_t ret = MDF_OK;
    char *rcv = NULL,* temp_rcv = NULL;
    size_t size = 0;
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
    	size=0;
    	mwifi_root_read(src_addr, &data_type, &temp_rcv, &size, portMAX_DELAY);

    	size = asprintf(&rcv,"%.*s",size, temp_rcv);

        MDF_FREE(temp_rcv);

        MDF_LOGD("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, rcv);


        //Separo los campos que me enviaron separados por comas
        data.cuarto = strtok(rcv, s);
        data.topic = strtok(NULL, s);
        data.msj = strtok(NULL, s);

        //ESP_LOGI(TAG,"Se recibio %s=%s=%s",data.cuarto,data.topic,data.msj);

        if(strcmp(data.topic,TOPIC_ESTADO) == 0)
        {
        	char *str_temp = NULL;
        	asprintf(&str_temp,"%s,/,%s",data.cuarto,data.msj);
        	xQueueSendToBack(estado_nodos_int,str_temp,portMAX_DELAY);
        	ESP_LOGI(TAG, "Enviado a la cola");
        	free(str_temp);
        }

        else if(strcmp(data.topic,TOPIC_DISPARO) == 0)
        {
        	char alarma[30] = "Alarma en: ";

			strcat(alarma,data.cuarto);
			esp_mqtt_client_publish(clientAdafruit, data.topic , alarma, 0, 1, 0);
        }
        free(rcv);
    }

    MDF_LOGW("Root write task is exit");
	esp_mqtt_client_stop(clientAdafruit);
    vTaskDelete(NULL);
}

void node_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *rcv = NULL, *rcv_temp = NULL;
    size_t size   = 0;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    node_msj data;
    const char s[2] = ",";
    MDF_LOGI("Node read task is running");

    while(1)
    {
        if (!mwifi_is_connected())
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

    	size=0;
        ret = mwifi_read(src_addr, &data_type, &rcv_temp, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));

        size = asprintf(&rcv,"%.*s",size,rcv_temp);

        MDF_FREE(rcv_temp);

        MDF_LOGD("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size,rcv);

        //Separo los campos que me enviaron separados por comas
        data.cuarto = strtok(rcv, s);
        data.topic = strtok(NULL, s);
        data.msj = strtok(NULL, s);

        ESP_LOGI(TAG,"Info recibida %s=%s=%s",data.cuarto,data.topic,data.msj);

        if(strcmp(data.topic,TOPIC_ENCENDIDO)==0)	//si es que se prendio o apago alguna alarma
        {
        	char *temp_on = NULL,*temp_off = NULL;

        	temp_on = MDF_MALLOC(sizeof(char)*25);
        	temp_off = MDF_MALLOC(sizeof(char)*25);

        	strcpy(temp_on,CUARTO);
        	strcat(temp_on," ON");
        	strcpy(temp_off,CUARTO);
        	strcat(temp_off," OFF");

        	ESP_LOGI(TAG,"%s=%s",data.cuarto,temp_on);

            if(strcmp(data.cuarto,temp_on) == 0)
            {
            	char* data3;
            	size_t size3;

				xSemaphoreGive(alarma_onoff_sem);
				//Agrego la interrupción para un pin en particular del GPIO
				gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
				set_lastState_nvs(1);

                size3 = asprintf(&data3,"%s,%s,%s",CUARTO,TOPIC_ESTADO,"ON");
                while(mwifi_write(NULL, &data_type,data3 , size3, true) != MDF_OK);
                MDF_LOGI("Enviado: %s",data3);
                MDF_FREE(data3);
				//xSemaphoreTake(pir_sem,1); //si no hubo movimiento que no sea bloqueante
            }
            else if(strcmp(data.cuarto,temp_off) == 0)
			{
            	char* data3;
            	size_t size3;

				gpio_isr_handler_remove(PIR_PIN);   //Evito que salte la interrupcion del pir
				xSemaphoreTake(alarma_onoff_sem,(TickType_t)1); //si ya esta apagado que no sea bloqueante
				set_lastState_nvs(0);

                size3 = asprintf(&data3,"%s,%s,%s",CUARTO,TOPIC_ESTADO,"OFF");
                while(mwifi_write(NULL, &data_type,data3 , size3, true) != MDF_OK);
                MDF_LOGI("Enviado: %s",data3);
                MDF_FREE(data3);
			}

    		free(temp_on);
    		free(temp_off);
        }//Si es movimiento no hago nada ya que yo envie la notificacion
        MDF_FREE(rcv);
    }

    MDF_LOGW("Node read task is exit");
    vTaskDelete(NULL);
}

