/*
 * mqtt.h
 *
 *  Created on: 13 mar. 2020
 *      Author: kevin
 */

#ifndef HEADERS_MQTT_H_
#define HEADERS_MQTT_H_

void mqtt_app_start(void);
void get_last_value(esp_mqtt_client_handle_t client,char* topic);
void state_nodes(void);
void Act_estado(void *pvParameter);


#endif /* HEADERS_MQTT_H_ */
