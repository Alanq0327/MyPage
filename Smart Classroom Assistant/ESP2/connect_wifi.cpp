#include <ESP8266WiFi.h>

const char* wifi = "RedRover";

void wifi_init()
{
    delay(10);

    Serial.print("Connecting to ");
    Serial.println(wifi);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(wifi);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    Serial.println("wifi connected");
}

void wifi_loop() {
    if (WiFi.status() != WL_CONNECTED) {

        static long lastReconnect = 0;
        long now = millis();

        if (now - lastReconnect > 1000) {
            lastReconnect = now;

            Serial.println("WiFi Reconnecting");
            WiFi.disconnect();
            WiFi.begin(wifi);

        }
    }
}