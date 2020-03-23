/*
 * fnvs.h
 *
 *  Created on: 19 mar. 2020
 *      Author: kevin
 */

#include "esp_err.h"
#include "mwifi.h"

#ifndef HEADERS_FNVS_H_
#define HEADERS_FNVS_H_

esp_err_t get_mwifi_nvs (mwifi_config_t * cfg, char * cuarto, uint8_t *last_state);
esp_err_t set_mwifi_nvs (mwifi_config_t * cfg,char * cuarto);
esp_err_t set_lastState_nvs (uint8_t last_state);


#endif /* HEADERS_FNVS_H_ */
