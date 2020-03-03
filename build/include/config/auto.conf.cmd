deps_config := \
	/home/kevin/esp/esp-mdf/esp-idf/components/app_trace/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/aws_iot/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/bt/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/driver/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/third_party/esp-aliyun/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/esp32/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/esp_adc_cal/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/esp_event/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/esp_http_client/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/esp_http_server/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/ethernet/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/fatfs/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/freemodbus/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/freertos/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/heap/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/libsodium/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/log/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/lwip/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/maliyun_linkkit/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/mbedtls/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/mcommon/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/mconfig/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/mdebug/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/mdns/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/mespnow/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/third_party/miniz/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/mqtt/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/mupgrade/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/mwifi/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/nvs_flash/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/openssl/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/pthread/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/spi_flash/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/spiffs/Kconfig \
	C:/msys32/home/kevin/esp/esp-mdf/components/third_party/esp-aliyun/components/ssl/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/tcpip_adapter/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/vfs/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/wear_levelling/Kconfig \
	/home/kevin/esp/esp-mdf/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/kevin/esp/esp-mdf/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/kevin/esp/mqtt_example/main/Kconfig.projbuild \
	/home/kevin/esp/esp-mdf/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/kevin/esp/esp-mdf/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
