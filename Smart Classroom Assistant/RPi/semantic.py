import json
import time
from google import genai
from tool import tools_map
from prompt import gen_prompt

#keywords
state_on = ["turn on", "switch on", "start", "activate", "power on"]
state_off = ["turn off", "switch off", "stop", "deactivate", "power off"]

devices_on = {
    "light_on": ["light", "lights"],
    "fan_on": ["fan"],
    "heater_on": ["heater"],
    "projector_on": ["projector"],
    "projector_movie_on": ["movie mode", "movie projector", "start movie"],
}

devices_off = {
    "light_off": ["light", "lights"],
    "fan_off": ["fan"],
    "heater_off": ["heater"],
    "projector_off": ["projector"],
}

#keyword matching before calling llm
def parse_query(text: str):
    text = text.lower()

    for act in state_on:
        if act in text:
            for action, device in devices_on.items():
                for device in devices_on:
                    if device in text:
                        return action
    for act in state_off:
        if act in text:
            for action, device in devices_off.items():
                for device in devices_off:
                    if device in text:
                        return action
    return


# the llm
client = genai.Client(api_key="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")

class LLM:
    def __init__(self, client, model="gemini-2.5-flash-lite"):
        self.client = client
        self.model = model

    def action(self, query, prompt):
        full_prompt = f"{prompt}\n\n{query}"
        response_action = self.client.models.generate_content(
            model="gemini-2.5-flash",
            contents = full_prompt,
        )
        response_action = response_action.text
        print(response_action)
        response_action = response_action.strip().strip("`")
        # parse the response
        try:
            action = json.loads(response_action)
        except Exception as e:
            print("JSON parse error:", e)
            return None

        return action
