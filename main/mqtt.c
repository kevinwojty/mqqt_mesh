/*
 * mqtt.c
 *
 *  Created on: 8 mar. 2020
 *      Author: kevin
 */

#include "mqtt_client.h"
#include "../headers/mqtt.h"
#include "../headers/Mqtt_intr_cb.h"

extern const char *CONFIG_BROKER_URL;
extern esp_mqtt_client_handle_t clientAdafruit;
extern const char* TAG2;

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
