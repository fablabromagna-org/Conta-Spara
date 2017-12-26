SDM230_RS485_master_esp8266.ino
===============================

Se non si vuole includere SSID e PASSWD direttamente nello sketch, creare un file myconfig.h, che verrà incluso nello sketch, con il seguente contenuto:

#define MYCONFIG
 
/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "ssid"
#define WLAN_PASS       "passwd"

/************************* Adafruit.io Setup *********************************/

#define MQTT_SERVER      "iot.eclipse.org"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    "conta-spara"
#define MQTT_KEY         ""

