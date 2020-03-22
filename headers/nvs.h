/*
 * nvs.h
 *
 *  Created on: 16 mar. 2020
 *      Author: kevin
 */

#ifndef HEADERS_NVS_H_
#define HEADERS_NVS_H_

esp_err_t get_mwifi_nvs (mwifi_config_t * cfg, char * cuarto);
esp_err_t set_mwifi_nvs (mwifi_config_t * cfg,char * cuarto);

#endif /* HEADERS_NVS_H_ */
