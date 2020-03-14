/*
 * mconfig.c
 *
 *  Created on: 13 mar. 2020
 *      Author: kevin
 */
#include "mdf_common.h"
#include "mconfig_blufi.h"
#include "mwifi.h"
#include "../headers/mconfig.h"

extern const char *TAG;

mdf_err_t get_network_config(const char *name, mwifi_config_t *mwifi_config, char custom_data[32])
{
    MDF_PARAM_CHECK(name);
    MDF_PARAM_CHECK(mwifi_config);
    MDF_PARAM_CHECK(custom_data);

    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {
        .tid = 22, /**< Type of device. Used to distinguish different products,
                       APP can display different icons according to this tid. 0-10 Light, 11-20 Button, 21-30 Sensor */
        .company_id = MCOMMON_ESPRESSIF_ID, /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    };

    strncpy(blufi_config.name, name, sizeof(blufi_config.name) - 1);
    MDF_LOGI("BLE name: %s", name);

    /**
     * @brief Initialize Bluetooth network configuration
     */
    MDF_ERROR_ASSERT(mconfig_blufi_init(&blufi_config));


    /**
     * @brief Get Network configuration information from blufi or network configuration chain.
     *      When blufi or network configuration chain complete, will send configuration information to config_queue.
     */
    //MDF_ERROR_ASSERT(mconfig_queue_read(&mconfig_data, 60000 / portTICK_RATE_MS));
    if(mconfig_queue_read(&mconfig_data, 60000 / portTICK_RATE_MS) == MDF_ERR_TIMEOUT)
    {
        MDF_FREE(mconfig_data);
        MDF_ERROR_ASSERT(mconfig_blufi_deinit());
    	return MDF_FAIL;
    }

    /**
     * @brief Deinitialize Bluetooth network configuration and Network configuration chain.
     */
    MDF_ERROR_ASSERT(mconfig_blufi_deinit());

    memcpy(mwifi_config, &mconfig_data->config, sizeof(mwifi_config_t));
    memcpy(custom_data, &mconfig_data->custom, sizeof(mconfig_data->custom));

    MDF_FREE(mconfig_data);

    return MDF_OK;
}
