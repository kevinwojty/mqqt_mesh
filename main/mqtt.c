/*
 * mqtt.c
 *
 *  Created on: 8 mar. 2020
 *      Author: kevin
 */

void get_last_value(esp_mqtt_client_handle_t client,char* topic)
{
	char* aux;

	asprintf(aux, "%s/get",topic);

	esp_mqtt_client_publish(client, aux , "\0", 0, 1, 0);
	free(aux);
}
