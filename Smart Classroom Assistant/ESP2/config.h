#ifndef CONFIG_H
#define CONFIG_H

// MQTT setup

#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "esp8266-2"

// MQTT Topics
#define TOPIC_LIGHT_PWM       "ca/light/pwm"
#define TOPIC_LIGHT_MODE      "ca/light/mode"
#define TOPIC_LIGHT_SWITCH    "ca/light/switch"
#define TOPIC_LIGHT_AUTO      "ca/light/auto"
#define TOPIC_LIGHT_SENSOR      "ca/light/sensor"

#define TOPIC_PROJ_POWER      "ca/projector/power"
#define TOPIC_PROJ_CURTAIN    "ca/projector/curtain"

// WS2812 setup
#define PIN_LED_STRIP   15
#define LED_COUNT       6

//projector LED
#define PIN_PROJECTOR_LED  2
// TB6612 curtain motor
#define PWMA   12
#define AIN1   14
#define AIN2   13

// BH1750 
#define I2C_SDA 4
#define I2C_SCL 5

//light auto variable
extern float g_target_lux;
extern float g_current_lux;

#endif