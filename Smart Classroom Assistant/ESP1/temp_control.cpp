#include "temp_control.h"
#include "temp_sensor.h"
#include "config.h"
#include <Arduino.h>


static bool temp_auto_mode = true;   // control temp auto
static bool fan_on = false;
static bool heater_on = false;


static float temp_low_threshold  = TEMP_LOW_C;   
static float temp_high_threshold = TEMP_HIGH_C;


// Initialization
void temp_control_init(void)
{
    pinMode(PIN_HEATER, OUTPUT);
    pinMode(PIN_FAN, OUTPUT);

    digitalWrite(PIN_HEATER, LOW);
    digitalWrite(PIN_FAN, LOW);

    Serial.println("temp_control initialized");
}


// Manual Fan Control
void temp_set_fan(bool on)
{
    temp_auto_mode = false;  // exit auto

    if (on) heater_on = false;

    fan_on = on;

    // update GPIO
    digitalWrite(PIN_FAN, fan_on ? HIGH : LOW);
    digitalWrite(PIN_HEATER, heater_on ? HIGH : LOW);

    Serial.printf("Manual Fan = %s\n", fan_on ? "ON" : "OFF");
}


// Manual Heater Control
void temp_set_heater(bool on)
{
    temp_auto_mode = false;   // exit AUTO

    if (on) fan_on = false;

    heater_on = on;

    // update GPIO
    digitalWrite(PIN_HEATER, heater_on ? HIGH : LOW);
    digitalWrite(PIN_FAN,    fan_on    ? HIGH : LOW);

    Serial.printf("Heater = %s\n", heater_on ? "ON" : "OFF");
}


void temp_set_auto(bool enable)
{
    temp_auto_mode = enable;

    Serial.printf("Temp mode = %s\n", temp_auto_mode ? "AUTO" : "MANUAL");
}


// Threshold set
void temp_set_low_threshold(float low)
{
    temp_low_threshold = low;
    Serial.printf("Temp low threshold updated to %.2f\n", temp_low_threshold);
}

void temp_set_high_threshold(float high)
{
    temp_high_threshold = high;
    Serial.printf("Temp high threshold updated to %.2f\n", temp_high_threshold);
}


bool temp_get_auto_mode()     { return temp_auto_mode; }
bool temp_get_fan_state()     { return fan_on; }
bool temp_get_heater_state()  { return heater_on; }
float temp_get_low_threshold(){ return temp_low_threshold; }
float temp_get_high_threshold(){ return temp_high_threshold; }


// auto control Loop
void temp_control_loop(void)
{
    static long last_ms = 0;
    long now = millis();
    if (now - last_ms < 1000) return;    // run every sec
    last_ms = now;

    if (!temp_auto_mode) {
        Serial.printf("Manual Mode: Heater=%d Fan=%d\n",
            heater_on, fan_on);
        return;
    }

    float t = temperature;

    // turn on heater
    if (t < temp_low_threshold)
    {
        // internal state
        heater_on = true;
        fan_on    = false;

        // GPIO
        digitalWrite(PIN_HEATER, HIGH);
        digitalWrite(PIN_FAN, LOW);

        Serial.println("Heater on, Fan off");
    }

    else if (t > temp_high_threshold)
    {
        heater_on = false;
        fan_on    = true;

        digitalWrite(PIN_HEATER, LOW);
        digitalWrite(PIN_FAN, HIGH);

        Serial.println("Heater off, Fan on");
    }

    else
    {
        heater_on = false;
        fan_on    = false;

        digitalWrite(PIN_HEATER, LOW);
        digitalWrite(PIN_FAN, LOW);

        Serial.println("all off");
    }
}
