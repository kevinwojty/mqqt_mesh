


#define TOPIC1 "chuka/f/Alarma"
#define TOPIC2 "chuka/f/EstadoAlarma"

#define PIR_PIN GPIO_NUM_13
#define GPIO_INPUT_PIN_SEL  (1ULL<<PIR_PIN)
#define LED_BUILT_IN GPIO_NUM_2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<LED_BUILT_IN)

typedef struct {
    size_t last_num;
    uint8_t *last_list;
    size_t change_num;
    uint8_t *change_list;
} node_list_t;

typedef struct{
	char* topic;
	char* cuarto;
	char* msj;
} node_msj;

esp_err_t mqtt_event_handler_adafuit(esp_mqtt_event_handle_t event);
void mqtt_app_start(void);
void print_system_info_timercb(void *timer);
mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx);
void IRAM_ATTR gpio_isr_handler(void* arg);
void root_write_task(void *arg);



