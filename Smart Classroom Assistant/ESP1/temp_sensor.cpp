#include "temp_sensor.h"
#include "config.h"
#include "mqtt.h"
#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>

float temperature;
static DHT dht(PIN_DHT, DHT11);

void temp_sensor_init() {
    dht.begin();
    Serial.println("dht11 initiated");
}

static void publish_temperature(float t)
{
    char temp[10];
    dtostrf(t,1,2,temp);
    mqtt_publish(TOPIC_TEMP_SENSOR, temp);
}

void temp_sensor_loop(void)
{
    static long last_update = 0;
    long now = millis();

    if (now - last_update < 1000) return;
    last_update = now;

    float t = dht.readTemperature();

    if (!isnan(t)) {
        temperature = t;
        publish_temperature(t);
    }

    Serial.print(F("Temperature: "));
    Serial.print(t);
    Serial.println(F("°C "));

}
