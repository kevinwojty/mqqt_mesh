/*
 * wifi_mesh.c
 *
 *  Created on: 19 mar. 2020
 *      Author: kevin
 */

#include <ctype.h>
#include "mwifi.h"
#include "esp_bt.h"
#include "../headers/fnvs.h"
#include "../headers/mconfig.h"

extern const char *TAG;
extern const char *TAG3;
extern DRAM_ATTR char CUARTO[20];


esp_err_t config_mesh (mwifi_config_t * cfg)
{
    char custom_data[32] = {0x0};
    char name[28]        = {0x0};
    //mwifi_config_t mwifi_config = {0x0};
    mdf_err_t err = MDF_OK;
    esp_err_t err2 = ESP_OK;

    ESP_LOGD(TAG, "Configuracion iniciada");

    sprintf(name, "ESP-WIFI-MESH_%s",CUARTO);

    do
    {
		err = get_network_config(name, cfg, custom_data);
		if(err == MDF_OK && strlen(custom_data) >3)
		{
			int k;
			strcpy(CUARTO,custom_data);
			for(k=0;k< strlen(CUARTO);k++)
				CUARTO[k] = tolower((int)CUARTO[k]);
			CUARTO[k] = 0;

			err2 = set_mwifi_nvs(cfg,CUARTO);

			if (err2 == ESP_OK)	// Se configuro correctamente
			{
				ESP_LOGI(TAG3, "Actualizada configuracion mesh");
			    MDF_ERROR_ASSERT(esp_bt_mem_release(ESP_BT_MODE_BLE));
				return ESP_OK;
			}
			ESP_LOGE(TAG3, "No se pudo guardar datos en NVS");
			nvs_flash_erase_partition("MWIFI_CONF");
		}
	}while(err2 != ESP_OK);

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    MDF_ERROR_ASSERT(esp_bt_mem_release(ESP_BT_MODE_BLE));
    return ESP_ERR_TIMEOUT;
}

mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}
