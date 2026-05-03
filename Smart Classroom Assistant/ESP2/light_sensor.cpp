#include "light_sensor.h"
#include "config.h"
#include "mqtt.h"
#include "light.h"
#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

float g_lux = 0.0f;
// GN1750 target
static BH1750 bh1750;
static void publish_lux(float l)
{
    char lux[10];
    dtostrf(l,1,2,lux);
    mqtt_publish(TOPIC_LIGHT_SENSOR, lux);
}
//BH1750 initialization
void sensor_init(void)
{
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("BH1750 init FAILED");
    } else {
        Serial.println("BH1750 init OK");
    }
}
//main loop
void sensor_loop(void)
{
    static long last_ms = 0;
    long now = millis();

    if (now - last_ms < 1000) return;
    last_ms = now;

    float lux = bh1750.readLightLevel();

    if (!isnan(lux)) {
        g_lux = lux;
        publish_lux(lux);
        light_auto_adjust(g_lux);
    }

    Serial.print("lux = ");
    Serial.println(g_lux);
}