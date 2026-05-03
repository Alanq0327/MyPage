#ifndef CONFIG_H
#define CONFIG_H


#define TOPIC_TEMP_SENSOR      "ca/temp/sensor"
#define TOPIC_FAN_CONTROL      "ca/temp/fan"
#define TOPIC_HEATER_CONTROL   "ca/temp/heater"
#define TOPIC_TEMP_LOW_SET     "ca/temp/low_set"
#define TOPIC_TEMP_HIGH_SET    "ca/temp/high_set"
#define TOPIC_TEMP_MODE        "ca/temp/mode"

// GPIO Esp8266-1
// Temp Sensor
#define PIN_DHT            5
// Heating & Cooling 
#define PIN_HEATER         10
#define PIN_FAN            4


//Temperature Threshold 
#define TEMP_LOW_C         24.0f
#define TEMP_HIGH_C        26.0f

#endif
