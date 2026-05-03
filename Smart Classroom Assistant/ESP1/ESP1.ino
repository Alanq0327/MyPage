#include <Arduino.h>
#include "config.h"
#include "connect_wifi.h"
#include "mqtt.h"
#include "temp_sensor.h"
#include "temp_control.h"

void setup()
{
  Serial.begin(115200);

  //wifi & MQTT
  wifi_init();
  mqtt_init();

  temp_sensor_init();         // DHT11
  temp_control_init();        // heating & cooling

  Serial.println("Esp8266-1 Init Complete");
}

void loop()
{
  wifi_loop();
  mqtt_loop();
  temp_sensor_loop();
  temp_control_loop();     
          
}
