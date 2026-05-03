#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "mqtt.h"

#include "light.h"
#include "projector.h"

const char* mqtt_server = "10.49.243.250";
static void mqtt_subscribe_all();
static void mqtt_callback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);

static long lastMQTT = 0;

//MQTT initialization
void mqtt_init() {
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqtt_callback);
    Serial.println("MQTT initiated");
}
//MQTT connection
void mqtt_connect() {
    if (client.connected()) return;
    long now = millis();
    if (now - lastMQTT < 3000) return;
    lastMQTT = now;

    Serial.println("MQTT connecting");

    if (client.connect("esp8266-light-proj", "sara", "12345678")) {
        Serial.println("MQTT connected");
        mqtt_subscribe_all();
    } else {
        Serial.print("rc=");
        Serial.println(client.state());
    }
}

//loop
void mqtt_loop() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!client.connected()) {
        mqtt_connect();
        return;
    }
    client.loop();
}

//publish
void mqtt_publish(const char* topic, const char* msg)
{
    if (!client.connected())
        mqtt_connect();

    if (!client.publish(topic, msg)) {
        Serial.print("MQTT Publish failed: ");
        Serial.println(topic);
    }
}
//subscribe all
static void mqtt_subscribe_all()
{
    client.subscribe(TOPIC_LIGHT_PWM);
    client.subscribe(TOPIC_LIGHT_AUTO);
    client.subscribe(TOPIC_LIGHT_MODE);
    client.subscribe(TOPIC_LIGHT_SWITCH);
    client.subscribe(TOPIC_PROJ_POWER);
    client.subscribe(TOPIC_PROJ_CURTAIN);
    Serial.println("MQTT Subscribed all topics");
}
//MQTT callback
static void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
    String message;
    for (int i = 0; i < length; i++)
        message += (char)payload[i];

    String topicStr = String(topic);

    Serial.print("Topic: ");
    Serial.print(topicStr);
    Serial.print(" | Msg: ");
    Serial.println(message);
//light control
//manual light mode
    if (topicStr == TOPIC_LIGHT_PWM)
    {
        if (message.length() == 0) return;
        int value = message.toInt();
        if (value < 0 || value > 100) return;
        if (value == 0) value = 1;  
        light_set_brightness(value);
        return;
    }

//auto light mode
    if (topicStr == TOPIC_LIGHT_AUTO)
    {
        if (message.startsWith("auto_on"))
        {
            float t = 350;
            int idx = message.indexOf(':');
            if (idx > 0)
                t = message.substring(idx + 1).toFloat();

            Serial.printf("Enable auto, target=%f\n", t);
            light_set_auto(true, t);
        }
        else if (message == "auto_off")
        {
            Serial.println("Auto OFF");
            light_set_auto(false, -1);
        }
        return;
    }

//mode switching
    if (topicStr == TOPIC_LIGHT_MODE)
    {
        if (message == "normal" || message == "eye")
            light_set_mode(message);

        return;
    }

//light turn on/off
    if (topicStr == TOPIC_LIGHT_SWITCH)
    {
        if (message == "on") light_switch(true);
        else if (message == "off") light_switch(false);
        return;
    }

//curtain&projector control
//motor control
    if (topicStr == TOPIC_PROJ_CURTAIN)
    {
        if (message == "up" || message == "down" || message == "stop")
            projector_curtain_action(message);
        return;
    }

//projector LED control
if (topicStr == TOPIC_PROJ_POWER)
{
    if (message == "on") {
        projector_power(true);
    }
    else if (message == "off") {
        projector_power(false);  
    }
    return;
}

}