/*
 * mqtt.c
 *
 *  Created on: 8 mar. 2020
 *      Author: kevin
 */

#include "mqtt_client.h"
#include "../headers/mqtt.h"
#include "../headers/Mqtt_intr_cb.h"
#include "mdf_common.h"
#include "mwifi.h"


extern const char *CONFIG_BROKER_URL;
extern esp_mqtt_client_handle_t clientAdafruit;
extern const char* TAG2,*TAG;
extern QueueHandle_t estado_nodos_ext,estado_nodos_int;
extern esp_mqtt_client_handle_t clientAdafruit;


void get_last_value(esp_mqtt_client_handle_t client,char* topic)
{
	char* aux;

	asprintf(&aux, "%s/get",topic);

	esp_mqtt_client_publish(client, aux , "\0", 0, 1, 0);
	ESP_LOGI(TAG2, "Solicito valor en: %s",aux);

	free(aux);
}

void state_nodes(void)
{
	get_last_value(clientAdafruit,TOPIC_ESTADO);
	return;
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .event_handle = mqtt_event_handler_adafuit,
        // .user_context = (void *)your_context
    };

    clientAdafruit = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(clientAdafruit);
}
/*Esta tarea contiene el estado de los actuadores y los publica cuando hay una modificación en el servidor*/
void Act_estado(void *pvParameter)
{
	char *rcv;
	cJSON *json_nodes = NULL;
    const char s[2] = ",";

    MDF_LOGI("Act task is running");


    rcv = malloc(sizeof(char) *400);
	memset(rcv,0,400);

	/* Espero que llegue información del servidor
	 * Esto solo se va a ejecutar al conectarse al servidor por primera vez
	 * despues se considera que la información mas actualizada esta en el dispositivo
	 */

	xQueueReceive(estado_nodos_ext,rcv,portMAX_DELAY);
	json_nodes = cJSON_Parse(rcv);
	free(rcv);

	ESP_LOGI(TAG, "Contenido JSON: %s",cJSON_Print(json_nodes));
    //json_nodes=cJSON_CreateObject();


	while(mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT)
    {
    	cJSON *json_cuarto = NULL;
        node_msj data;
        char rcv2[50];

        memset(rcv2,0,50);
    	xQueueReceive(estado_nodos_int,rcv2,portMAX_DELAY);

        //Separo los campos que me enviaron separados por comas
        data.cuarto = strtok(rcv2, s);
        data.topic = strtok(NULL, s);
        data.msj = strtok(NULL, s);

    	ESP_LOGI(TAG,"Se recibio %s=%s=%s",data.cuarto,data.topic,data.msj);

        json_cuarto = cJSON_GetObjectItem(json_nodes, data.cuarto);

        if(cJSON_IsString(json_cuarto))	//significa que ya se encuentra en el json y hay que modificarlo
		{
        	cJSON_DeleteItemFromObject(json_nodes,data.cuarto);
			cJSON_AddStringToObject(json_nodes,data.cuarto,data.msj);
		}
        else
        {
			cJSON_AddStringToObject(json_nodes,data.cuarto,data.msj);
        }

    	char *rendered=cJSON_Print(json_nodes);
		ESP_LOGI(TAG, "Contenido JSON: %s",rendered);

		esp_mqtt_client_publish(clientAdafruit, TOPIC_ESTADO , rendered, 0, 1, 0);//aca habría que mandar a adafruit

		free(rendered);
    }

    MDF_LOGW("Act_estado task is exit");
    cJSON_Delete(json_nodes);
    vTaskDelete(NULL);
}
