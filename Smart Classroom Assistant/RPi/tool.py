#all the actions available
def light_on(RPi, value):
    RPi.light_switch(True)
    
def light_off(RPi,value):
    RPi.light_switch(False)
    
def light_brightness(RPi, value):
    RPi.light_pwm(int(value))

def fan_on(RPi, value):
    RPi.fan_control(True)
    RPi.heater_control(False)
    
def fan_off(RPi, value):
    RPi.fan_control(False)

def heater_on(RPi, value):
    RPi.heater_control(True)
    RPi.fan_control(False)

def heater_off(RPi, value):
    RPi.heater_control(False)
    
def projector_on(RPi, value):
    RPi.projector_motor("down")
    RPi.proj_power("on")

def projector_off(RPi, value):
    RPi.projector_motor("up")
    RPi.proj_power(False)
    RPi.light_switch(True)
    
def projector_movie_on(RPi, value):
    RPi.projector_motor("down")
    RPi.proj_power(True)
    RPi.light_switch(False)
# if voice command doesn't match any function, llm respond with none
def none(RPi, value):
    pass

# description of all tools
tools_info = [
    {
        "name": "light_on",
        "description": "turn on lights in the classroom",
        "args": "none",
    },
    {
        "name": "light_off",
        "description": "turn off lights in the classroom",

    },
    {
        "name": "light_brightness",
        "description": "change light brightness in the classroom (0-100)",
        "args": [
            {
                "name": "brightness",
                "type": "string",
                "description": "the light brightness value from 0 - 100"
            },
        ]
    },
    {
        "name": "fan_on",
        "description": "turn on fan in the classroom",
        "args": "none",

    },
    {
        "name": "fan_off",
        "description": "turn off fan in the classroom",
        "args": "none",
    },
        {
        "name": "heater_on",
        "description": "turn on heater in the classroom",
        "args": "none",
    },
    {
        "name": "heater_off",
        "description": "turn off heater in the classroom",
        "args": "none",

    },
    {
        "name": "projector_on",
        "description": "start projector in the classroom",
        "args": "none",

    },
    {
        "name": "projector_movie_on",
        "description": "start projector in movie mode in the classroom",
        "args": "none",
    },
    {
        "name": "projector_off",
        "description": "turn off projector in the classroom",
        "args": "none",
    },
    {
        "name": "none",
        "description": "do nothing",
        "args": "none",
    }
]
    

tools_map = {
    "light_on": light_on,
    "light_off": light_off,
    "light_brightness": light_brightness,
    "fan_on": fan_on,
    "fan_off": fan_off,
    "heater_on": heater_on,
    "heater_off": heater_off,
    "projector_on": projector_on,
    "projector_movie_on": projector_movie_on,
    "projector_off": projector_off,
    "none": none
}

# generate the available tools' description to inform the llm
def gen_tools_desc():
    tools_desc = []
    for idx, t in enumerate(tools_info):
        tool_desc = f"{idx+1}.{t['name']}:{t['description']}"
        tools_desc.append(tool_desc)
    tools_prompt = "\n".join(tools_desc)
    return tools_prompt