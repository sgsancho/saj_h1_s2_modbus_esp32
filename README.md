Esta aplicación se ha desarrollado para facilitar el acceso por modbus al Inversor SAJ H1 S2.

¿Cómo funciona? El esp32 se conecta por bluetooth al Dongle del inversor utilizando la librería blufi de espressif. A su vez, el esp32 levanta un servicio tcp que permite recibir llamadas con protocolo modbus tcp. Para poder levantar el servicio tcp, es necesario configurar el ssid y password para poder conectarse a la red local.

Pasos para cargar el código fuente en el esp32:
1º Instalar Arduino IDE
2º Abre el fichero sah_h1_s2_modbus_esp32.ino con Arduino IDE.
3º Conecta el esp32 al equipo. Deberás configurar tu tipo de placa. Si tu esp32 es de 4Mb, debes modificar el esquema de particiones como "Huge APP" para tener espacio suficiente.
4º Debes instalar la librería "ESP32 BLE for Arduino" (BLEDevice). Puedes hacerlo desde "Herramientas" / "Administrar bibliotecas" -> "BLEDevice"
5º Configura el ssid y password de tu wifi:
const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";



Este trabajo ha sido gracias a la colaboración del grupo de Telegram https://t.me/saj_nooficialoriginal. No es una aplicación oficial y podría dejar de funcionar en cualquier momento.

Úsala bajo tu propia responsabilidad. Desconocemos si afecta en algún sentido al rendimiento y fiabilidad de su instalación fotovoltaica

- Ver [Releases](https://github.com/sgsancho/saj_h1_s2_modbus_esp32/releases)

- Ver [Instrucciones](https://github.com/sgsancho/saj_h1_s2_modbus_esp32/blob/main/documentacion/instrucciones_saj_h1s2_modbus.pdf)

- Ver [Modbus EXCEL](https://github.com/sgsancho/saj_h1_s2_modbus_esp32/blob/main/documentacion/SAJ_Modbus_Communication_Protocol_2020.xlsx)

- Ver [Modbus PDF](https://github.com/sgsancho/saj_h1_s2_modbus_esp32/blob/main/documentacion/SAJ_Modbus_Communication_Protocol_2020.pdf)


## MIT License
- Ver [License](LICENSE)
