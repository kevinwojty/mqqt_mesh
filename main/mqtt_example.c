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

#include <ctype.h>
#include "mdf_common.h"
#include "mwifi.h"
#include "mqtt_client.h"
#include "mdf_event_loop.h"
#include "esp_bt.h"
//#include "mesh_mqtt_handle.h"
#include "../headers/Mqtt_intr_cb.h"
#include "../headers/mqtt.h"
#include "../headers/node.h"
#include "../headers/mconfig.h"
#include "../headers/fnvs.h"
#include "../headers/wifi_mesh.h"

void LectPir (void *pvParameter);

// #define MEMORY_DEBUG

const char *CONFIG_BROKER_URL ="mqtt://chuka:75fee8844a354e30a3d8b1ac2c5d375c@io.adafruit.com";
const char *TAG = "mesh";
const char *TAG2 = "mqtt_adafruit";
const char *TAG3 = "NVS";

DRAM_ATTR char CUARTO[20] = "default";
SemaphoreHandle_t pir_sem, alarma_onoff_sem, config_sem;
QueueHandle_t estado_nodos;
esp_mqtt_client_handle_t clientAdafruit;

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

            if(esp_mesh_is_root())	//si soy root envio por wifi caso contrario le envio al root el msj
            {
            	char alarma[30] = "Alarma en: ";
    			strcat(alarma,CUARTO);
    			esp_mqtt_client_publish(clientAdafruit, TOPIC_DISPARO , alarma, 0, 1, 0);
    			ESP_LOGI(TAG2, "Publicado: %s",alarma);
            }
            else
            {
                size = asprintf(&data,"%s,%s,%s",CUARTO,TOPIC_DISPARO,"A");
                while(mwifi_write(NULL, &data_type,data , size, true) != MDF_OK);
                MDF_LOGI("Enviado: %s",data);
                MDF_FREE(data);
            }

            vTaskDelay(30000 / portTICK_RATE_MS);
            gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);   //habilito nuevamente el pir
        }
    }
    vTaskDelete(NULL);
}

void task_reset_config(void *pvParameter)
{
    xSemaphoreTake(config_sem,portMAX_DELAY);
    while(1)
    {
		ESP_LOGI(TAG, "Reseteando para configurar dispositivo");
		esp_restart();
    }
	vTaskDelete(NULL);
}


void app_main()
{
    gpio_config_t io_conf;

    mwifi_init_config_t cfg   = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t mwifi_config = {0x0};
    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    esp_log_level_set("mconfig_blufi", ESP_LOG_DEBUG);
    esp_log_level_set(TAG2, ESP_LOG_DEBUG);
    esp_log_level_set(TAG3, ESP_LOG_DEBUG);

    // @brief Configuraci�n de pines y habilitaci�n de int
  io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.pull_down_en = 1;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);      //configure GPIO with the given settings

  io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL2;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);      //configure GPIO with the given settings

	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);      //configure GPIO with the given settings

	gpio_set_level(LED_BUILT_IN,0);


    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());

    esp_reset_reason_t rst_reason;
    uint8_t last_state;

	if(get_mwifi_nvs(&mwifi_config,CUARTO, &last_state) != ESP_OK)
	{
		ESP_LOGE(TAG, "No se pudo leer datos en NVS");
		nvs_flash_erase_partition("MWIFI_CONF");
		esp_restart();
	}

    rst_reason = esp_reset_reason();

    if(rst_reason == ESP_RST_SW)
    {
    	config_mesh(&mwifi_config);
    	last_state = 0;		//al reiniciar el sensor empieza apagado
    }

    MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR ", cuarto: %s, last state: %d",
             mwifi_config.router_ssid, mwifi_config.router_password,
             MAC2STR(mwifi_config.mesh_id), CUARTO, last_state);


    //Instalo las interrupciones para GPIO
    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
    //Creo los semaforos y los inicializo tomados
    pir_sem = xSemaphoreCreateBinary();
    alarma_onoff_sem = xSemaphoreCreateBinary();
    config_sem = xSemaphoreCreateBinary();
    estado_nodos = xQueueCreate(1,sizeof(char)*200);

    if(pir_sem == NULL || alarma_onoff_sem == NULL || config_sem == NULL)
    {
        ESP_LOGE(TAG, "No se pudo inicializar el semaforo");
        esp_restart();
    }

    xSemaphoreTake(pir_sem,10 / portTICK_RATE_MS);     //por seguridad,segun documentacion empieza tomado
    xSemaphoreTake(config_sem,10 / portTICK_RATE_MS);

    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&mwifi_config));
    MDF_ERROR_ASSERT(mwifi_start());

    //Inicializacion de las tareas
    xTaskCreate(node_read_task, "node_read_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    xTaskCreate(&LectPir, "LectPir", 4 * 1024,NULL,2,NULL );
    xTaskCreate(&Postled, "Postled", 2048,NULL,1,NULL );
    xTaskCreate(&task_reset_config, "task_reset", 2048,NULL,5,NULL );
	gpio_isr_handler_add(PUL_BOOT, gpio_isr_handler, (void*) PUL_BOOT);

    if(last_state == 1)	// significa que estaba prendido antes de apagarse el sistema
    {
    	xSemaphoreGive(alarma_onoff_sem);
    	gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
    }
    else
    	xSemaphoreTake(alarma_onoff_sem,10 / portTICK_RATE_MS);

}
