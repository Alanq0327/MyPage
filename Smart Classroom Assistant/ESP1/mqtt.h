#ifndef MQTT_H
#define MQTT_H

void mqtt_init(void);
void mqtt_loop(void);
void mqtt_publish(const char *topic, const char *msg);

#endif
