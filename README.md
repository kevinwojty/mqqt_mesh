Detección de movimiento en espacios amplios e informes mediante alarmas en la nube
==========================

Resumen
--------------------------
Se trata de una red de alarmas de movimiento diseñadas con ESP32 para que se conectan en formato mesh, esto permite que se pueda escalar y abarcar grandes espacios sin la necesidad de conexión a internet o reconfiguración de la red, para eso se utiliza el framework [ESP-MDF](https://github.com/espressif/esp-mdf) que agrega estas funcionalidades.

Para poder visualizar de forma remota el estado de las alarmas las mismas se transmiten a un servidor utilizando MQTT en formato JSON en el topico f/EstadoAlarma, en particular se utilizo la plataforma de [AdafruitIO](https://io.adafruit.com/) en donde es almacenada y visualizada desde cualquier dispositivo móvil con alguna aplicación como [MQTT Snooper](https://play.google.com/store/apps/details?id=mqttsnooper.mqttsnooper&hl=en_US&gl=US). Así mismo es posible habilitar y deshabilitar alarmas a través del topic f/Alarma

El dispositivo cuenta con un sensor PIR conectado al pin GPIO13 para identificar movimientos.

Provisionamiento y configuración del equipo
--------------------------
1. Descargar la aplicación [ESPMesh](https://github.com/EspressifApp/EspMeshForAndroid)
2. Tener habilitado el Bluetooth en el dispositivo móvil y estar conectado al WIFI
3. Encender el ESP32 y mantener presionado el boton BOOT 
4. Conectarse con la aplicación movil al ESP y definir el nombre del cuarto donde será ubicado.
