#include <Arduino.h>
#include "config.h"
#include "connect_wifi.h"
#include "mqtt.h"
#include "light_sensor.h"
#include "light.h"
#include "projector.h"

void setup()
{
  Serial.begin(115200);
  // WiFi & MQTT
  wifi_init();
  mqtt_init();

  // hardware initialization
  sensor_init();            // BH1750 setup
  light_init();             // WS2812B
  projector_init();         // projector LED & curtain motivited motor

  Serial.println("System Init Complete");
}

void loop()
{
  mqtt_loop();
  sensor_loop();
 projector_loop();    
  delay(10);
}