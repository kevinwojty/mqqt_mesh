/*
 * nvs.c
 *
 *  Created on: 16 mar. 2020
 *      Author: kevin
 */

#include "mdf_common.h"
#include "mwifi.h"
#include "../headers/nvs.h"

esp_err_t get_mwifi_nvs (mwifi_config_t * cfg, char * cuarto)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open("MWIFI_CONF", NVS_READONLY, &my_handle);
	if (err != ESP_OK) return err;

	// Read
	size_t required_size;
	err = nvs_get_str(my_handle,"router_ssid",cfg->router_ssid,&required_size);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	err = nvs_get_str(my_handle,"router_pass",cfg->router_password,&required_size);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	err = nvs_get_str(my_handle,"mesh_pass",cfg->mesh_password,&required_size);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	char *aux;

	err = nvs_get_str(my_handle,"room",NULL,&required_size);
	aux=malloc(sizeof(char)*(required_size+1));
	err = nvs_get_str(my_handle,"room",aux,&required_size);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	strcpy(cuarto,aux);
	free(aux);

	size_t size = sizeof(cfg->mesh_id);
	nvs_get_blob(my_handle, "mesh_id", cfg->mesh_id, &size);

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t set_mwifi_nvs (mwifi_config_t * cfg,char * cuarto)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open("MWIFI_CONF", NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	// Write
	err = nvs_set_str(my_handle,"router_ssid",cfg->router_ssid);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	err = nvs_set_str(my_handle,"router_pass",cfg->router_password);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;


	err = nvs_set_str(my_handle,"mesh_pass",cfg->mesh_password);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	err = nvs_set_str(my_handle,"room",cuarto);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

	size_t size = sizeof(cfg->mesh_id);
	nvs_set_blob(my_handle, "mesh_id", cfg->mesh_id, size);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

