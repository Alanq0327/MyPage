import json
import paho.mqtt.client as mqtt


MQTT_HOST = "10.49.246.17"
MQTT_PORT = 1883

# Topics for ESP to RPi(sensors)
TOPIC_TEMP_SENSOR = "ca/temp/sensor"
TOPIC_LUX_SENSOR  = "ca/light/sensor"

# Topics for RPi to ESP(control)
# temperature
TOPIC_FAN           = "ca/temp/fan"
TOPIC_HEATER        = "ca/temp/heater"
TOPIC_TEMP_MODE     = "ca/temp/mode"
TOPIC_TEMP_LOW_SET  = "ca/temp/low_set"
TOPIC_TEMP_HIGH_SET = "ca/temp/high_set"

# lighting
TOPIC_LIGHT_PWM     = "ca/light/pwm"
TOPIC_LIGHT_MODE    = "ca/light/mode"
TOPIC_LIGHT_SWITCH  = "ca/light/switch"
TOPIC_LIGHT_AUTO    = "ca/light/auto"

# projector
TOPIC_PROJ_POWER    = "ca/projector/power"
TOPIC_PROJ_CURTAIN  = "ca/projector/curtain"
# projector light
TOPIC_PROJ_LIGHT = "ca/projector/light"

# MQTT CLIENT CLASS
class MQTTClient:
    def __init__(self):
        self.client = mqtt.Client()
        self.client.username_pw_set("sara", "12345678")
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

        # sensor values
        self.current_temp = None
        self.current_lux  = None
        self.brightness   = None
    # connect
    def connect(self):
        self.client.connect(MQTT_HOST, MQTT_PORT, 60)
    # subscribe on connect
    def on_connect(self, client, userdata, flags, rc):
        client.subscribe(TOPIC_TEMP_SENSOR)
        client.subscribe(TOPIC_LUX_SENSOR)

    # handle incoming messages
    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode()
        # temperature sensor
        if topic == TOPIC_TEMP_SENSOR:
            try:
                self.current_temp = float(payload)
            except:
                print("Invalid temperature:", payload)
        # lux sensor
        elif topic == TOPIC_LUX_SENSOR:
            try:
                self.current_lux = float(payload)
            except:
                print("Invalid lux:", payload)

    # TEMPERATURE CONTROL
    def fan_control(self, on):
        msg = "fan_on" if on else "fan_off"
        self.client.publish(TOPIC_FAN, msg)

    def heater_control(self, on):
        msg = "heater_on" if on else "heater_off"
        self.client.publish(TOPIC_HEATER, msg)

    def temp_mode(self, mode):
        self.client.publish(TOPIC_TEMP_MODE, mode)

    def set_temp_thresholds(self, low, high):
        self.client.publish(TOPIC_TEMP_LOW_SET,  str(low))
        self.client.publish(TOPIC_TEMP_HIGH_SET, str(high))

    # LIGHT CONTROL
    def light_pwm(self, value):

        self.client.publish(TOPIC_LIGHT_PWM, str(value))

    def light_mode(self, mode):
        if mode not in ("normal", "eye"):
            print("Invalid mode:", mode)
            return
        self.client.publish(TOPIC_LIGHT_MODE, mode)

    def light_switch(self, on):
        msg = "on" if on else "off"
        self.client.publish(TOPIC_LIGHT_SWITCH, msg)


    def light_auto(self, msg: str):
        self.client.publish(TOPIC_LIGHT_AUTO, msg)
    # PROJECTOR CONTROL
    def proj_power(self, on):
        msg = "on" if on else "off"
        self.client.publish(TOPIC_PROJ_POWER, msg)

    def projector_motor(self, cmd):
        if cmd not in ("up", "down", "stop"):
            print("Invalid curtain cmd:", cmd)
            return
        self.client.publish(TOPIC_PROJ_CURTAIN, cmd)

    def loop(self):
        self.client.loop_start()
        
mqtt_rpi = MQTTClient()
mqtt_rpi.connect()
mqtt_rpi.loop()

