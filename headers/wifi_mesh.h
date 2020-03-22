/*
 * wifi_mesh.h
 *
 *  Created on: 19 mar. 2020
 *      Author: kevin
 */

#include "mwifi.h"

#ifndef HEADERS_WIFI_MESH_H_
#define HEADERS_WIFI_MESH_H_

mdf_err_t wifi_init();
esp_err_t config_mesh (mwifi_config_t * cfg);


#endif /* HEADERS_WIFI_MESH_H_ */
