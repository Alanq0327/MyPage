#ifndef MQTT_H
#define MQTT_H
//MQTT initialization
void mqtt_init(void);
//main loop
void mqtt_loop(void);
//publish
void mqtt_publish(const char *topic, const char *msg);

#endif