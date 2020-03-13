/*
 * mqtt.c
 *
 *  Created on: 8 mar. 2020
 *      Author: kevin
 */

#include "mqtt.h"

void get_last_value(esp_mqtt_client_handle_t client,char* topic)
{
	char* aux;

	asprintf(aux, "%s/get",topic);

	esp_mqtt_client_publish(client, aux , "\0", 0, 1, 0);
	free(aux);
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
