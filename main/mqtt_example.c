// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mdf_common.h"
#include "mwifi.h"
#include "mesh_mqtt_handle.h"
#include "../headers/Mqtt_intr_cb.h"

const char *CONFIG_BROKER_URL ="mqtt://chuka:75fee8844a354e30a3d8b1ac2c5d375c@io.adafruit.com";

const char *TAG = "mesh";


void LectPir (void *pvParameter);

// #define MEMORY_DEBUG

char *CUARTO = "cuarto";
SemaphoreHandle_t pir_sem, alarma_onoff_sem;
esp_mqtt_client_handle_t clientAdafruit;

/*
static bool addrs_remove(uint8_t *addrs_list, size_t *addrs_num, const uint8_t *addr)
{
    for (int i = 0; i < *addrs_num; i++, addrs_list += MWIFI_ADDR_LEN) {
        if (!memcmp(addrs_list, addr, MWIFI_ADDR_LEN)) {
            if (--(*addrs_num)) {
                memcpy(addrs_list, addrs_list + MWIFI_ADDR_LEN, (*addrs_num - i) * MWIFI_ADDR_LEN);
            }

            return true;
        }
    }

    return false;
}
*/
void root_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *rcv    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};
    node_msj data;

    const char s[2] = ",";

    MDF_LOGI("Root write task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT)
    {
        /**
         * @brief Recv data from node, and forward to mqtt server.
         */
        memset(rcv, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_root_read(src_addr, &data_type, rcv, &size, portMAX_DELAY);

        //Separo los campos que me enviaron separados por comas
        data.cuarto = strtok(rcv, s);
        data.topic = strtok(NULL, s);
        data.msj = strtok(NULL, s);

        if(strcmp(data.topic,TOPIC2) == 0)
        {
        	char alarma[50] = "Alarma en: ";

			strcpy(alarma,data.cuarto);
			esp_mqtt_client_publish(clientAdafruit, data.topic , alarma, 0, 1, 0);
        }
    }

    MDF_LOGW("Root write task is exit");
	esp_mqtt_client_stop(clientAdafruit);
    vTaskDelete(NULL);
}

static void node_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Node read task is running");

    for (;;)
    {
        if (!mwifi_is_connected())
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));
        MDF_LOGD("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
        //xSemaphoreTake(alarma_onoff_sem,portMAX_DELAY); //Dejo el led cte PRUEBA
    }

    MDF_LOGW("Node read task is exit");
    MDF_FREE(data);
    vTaskDelete(NULL);
}

static void node_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    int count     = 0;
    size_t size   = 0;
    char *data    = NULL;
    mwifi_data_type_t data_type     = {0x0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};

    MDF_LOGI("Node task is running");

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    for (;;) {
        if (!mwifi_is_connected() || !mwifi_get_root_status()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        /**
         * @brief Send device information to mqtt server throught root node.
         */
        size = asprintf(&data, "{\"mac\": \"%02x%02x%02x%02x%02x%02x\", \"seq\":%d,\"layer\":%d}",
                        MAC2STR(sta_mac), count++, esp_mesh_get_layer());

        MDF_LOGD("Node send, size: %d, data: %s", size, data);
        ret = mwifi_write(NULL, &data_type, data, size, true);
        MDF_FREE(data);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));

        vTaskDelay(3000 / portTICK_RATE_MS);
    }

    MDF_LOGW("Node task is exit");
    vTaskDelete(NULL);
}



static mdf_err_t wifi_init()
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


/*Tarea para indicar si esta activada o no la alarma*/
void Postled(void *pvParameter)
{
    while(1)
    {
        xSemaphoreTake(alarma_onoff_sem,portMAX_DELAY); //Me aseguro que este habilitada la alarma
        xSemaphoreGive(alarma_onoff_sem);
        gpio_set_level(LED_BUILT_IN,0); //
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(LED_BUILT_IN,1); //prendo el led
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}
/*Tarea encargada de enviar una alerta por mqtt en caso de haber movimiento y estar activada la alarma*/
void LectPir (void *pvParameter)
{
    size_t size   = 0;
    char *data    = NULL;
    mwifi_data_type_t data_type     = {0x0};
    while(1)
    {
        xSemaphoreTake(pir_sem,portMAX_DELAY);  //espero a que me llegue una señal del sensor
        if(xSemaphoreTake( alarma_onoff_sem, ( TickType_t ) 10 ) == pdTRUE ) //Me fijo si esta habilitada la alarma, caso negativo no hago nada
        {
            gpio_isr_handler_remove(PIR_PIN);//evito que vuelva a llamarme el pir dentro de los 30 seg
            xSemaphoreGive(alarma_onoff_sem);

            if(esp_mesh_is_root())	//si soy root envio por mesh caso contrario le envio al root el msj
            {
            	char alarma[50] = "Alarma en: ";
    			strcpy(alarma,CUARTO);
    			esp_mqtt_client_publish(clientAdafruit, TOPIC2 , alarma, 0, 1, 0);

            }
            else
            {
                size = asprintf(&data,"%s,%s,%s",CUARTO,TOPIC2,"no hay msj");
                while(mwifi_write(NULL, &data_type,data , size, true) != MDF_OK);
            }
            MDF_FREE(data);
            vTaskDelay(30000 / portTICK_RATE_MS);
            gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);   //habilito nuevamente el pir
        }
    }
    vTaskDelete(NULL);
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

void app_main()
{
    gpio_config_t io_conf;

    mwifi_init_config_t cfg   = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config     = {
        .router_ssid     = CONFIG_ROUTER_SSID,
        .router_password = CONFIG_ROUTER_PASSWORD,
        .mesh_id         = CONFIG_MESH_ID,
        .mesh_password   = CONFIG_MESH_PASSWORD,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.pull_down_en = 1;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);      //configure GPIO with the given settings

	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);      //configure GPIO with the given settings


    //Instalo las interrupciones para GPIO
    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);

    pir_sem = xSemaphoreCreateBinary();
    alarma_onoff_sem = xSemaphoreCreateBinary();

    /**
     * @brief Create node handler
     */

    xSemaphoreTake(pir_sem,100 / portTICK_RATE_MS);     //por seguridad,segun documentacion empieza tomado
    xSemaphoreTake(alarma_onoff_sem,100 / portTICK_RATE_MS);

    if(pir_sem == NULL || alarma_onoff_sem == NULL)
    {
        ESP_LOGE(TAG, "No se pudo inicializar el semaforo");
        esp_restart();
    }
//    xTaskCreate(node_write_task, "node_write_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
	xTaskCreate(node_read_task, "node_read_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

	gpio_set_level(LED_BUILT_IN,1);

    xTaskCreate(&LectPir, "LectPir", 4 * 1024,NULL,2,NULL );
    xTaskCreate(&Postled, "Postled", 2048,NULL,1,NULL );
}
