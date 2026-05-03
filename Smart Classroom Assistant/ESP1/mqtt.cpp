#include "config.h"
#include "mqtt.h"
#include "temp_control.h"

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* mqtt_server = "10.49.243.250";

static void mqtt_subscribe_all();
static void mqtt_callback(char* topic, byte* message, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);

static long lastMQTT = 0;

// init
void mqtt_init() {
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqtt_callback);
    Serial.println("MQTT initiated");
}

void mqtt_connect() {
    if (client.connected()) return;

    long now = millis();
    if (now - lastMQTT < 3000) return;
    lastMQTT = now;

    Serial.println("MQTT connecting");

    if (client.connect("esp8266-1", "sara", "12345678")) {
        Serial.println("MQTT connected");
        mqtt_subscribe_all();
    } else {
        Serial.print("rc=");
        Serial.println(client.state());
    }
}

void mqtt_loop() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!client.connected()) {
        mqtt_connect();
        return;
    }

    client.loop(); 
}

void mqtt_publish(const char *topic, const char *msg)
{
    if (!client.connected())
        mqtt_connect();

    if (!client.publish(topic, msg))
    {
        Serial.print("MQTT Publish failed: ");
        Serial.println(topic);
    }
}

// subscribe topic
static void mqtt_subscribe_all()
{
    // Temperature mode
    client.subscribe(TOPIC_TEMP_MODE);

    //Fan Heater
    client.subscribe(TOPIC_FAN_CONTROL);
    client.subscribe(TOPIC_HEATER_CONTROL);

    //Temperature thresholds
    client.subscribe(TOPIC_TEMP_LOW_SET);
    client.subscribe(TOPIC_TEMP_HIGH_SET);

    Serial.println("MQTT Subscribed");
}

static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    // Convert payload to String
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    String topicStr = String(topic);

    Serial.print("Topic: ");
    Serial.print(topicStr);
    Serial.print(" | Msg: ");
    Serial.println(message);


    if (topicStr == TOPIC_TEMP_MODE)
    {
        if (message == "auto") {
            temp_set_auto(true);
            Serial.println("Temp Mode set to AUTO");
        }
        else if (message == "manual") {
            temp_set_auto(false);
            Serial.println("Temp Mode set to MANUAL");
        }
        else {
            Serial.println("Temp Invalid mode");
        }
        return;
    }


    if (topicStr == TOPIC_FAN_CONTROL)
    {
        if (message == "fan_on") {
            temp_set_fan(true);
        } else {
            temp_set_fan(false);
        }
        return;
    }


    if(topicStr == TOPIC_HEATER_CONTROL)
    {
        if (message == "heater_on") {
            temp_set_heater(true);
        } else {
            temp_set_heater(false);
        }
        return;
    }

    if (topicStr == TOPIC_TEMP_LOW_SET)
    {
        float low = message.toFloat();
        temp_set_low_threshold(low);
        Serial.printf("Temp low threshold updated: %.2f\n", low);
        return;
    }

    if(topicStr == TOPIC_TEMP_HIGH_SET)
    {
        float high = message.toFloat();
        temp_set_high_threshold(high);
        Serial.printf("Temp high threshold updated: %.2f\n", high);
        return;
    }
}
