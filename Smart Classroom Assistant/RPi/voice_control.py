from vosk import Model, KaldiRecognizer
import time
from google import genai
from tool import tools_map
from prompt import gen_prompt
from semantic import parse_query, LLM, client
from mqtt import mqtt_rpi
import pyaudio
import json
import os
import threading

#voice recognition model
model = Model("vosk-model-small-en-us-0.15")
rec = KaldiRecognizer(model, 16000)

fifo_path = "voice_fifo"
if not os.path.exists(fifo_path):
    os.mkfifo(fifo_path)

voice_active = False
#run main.py using sudo python main.py
#run voice_control.py using python voice_control.py
#main.py sending "start" and "stop" command through voice_fifo
def listen_fifo():
    global voice_active
    while True:
        with open(fifo_path, "r") as fifo:
            for line in fifo:
                cmd = line.strip()
                if cmd == "start":
                    voice_active = True
                    print("Activated")
                elif cmd == "stop":
                    voice_active = False
                    print("Deactivated")
        time.sleep(0.1)

threading.Thread(target=listen_fifo, daemon=True).start()

def voice_control(RPi):
    #voice recognition
    p = pyaudio.PyAudio()
    stream = p.open(format=pyaudio.paInt16, channels=1, rate=16000,
                    input=True, frames_per_buffer=8000)
    stream.start_stream()


    while True:
        data = stream.read(4000, exception_on_overflow=False)
        if rec.AcceptWaveform(data):# finish speaking
            res = json.loads(rec.Result())
            query = res.get("text", "")
            print(query)
            # if contains raspberry pi
            if voice_active and ("raspberry" in query) and ("pi" in query):
                print(2)
                start_time = time.time()

                if parse_query(query): #keyword match, cost 1 msec
                    action_name = parse_query(query)
                else: #call llm, 0.5 ~ 2 sec
                    prompt = gen_prompt()
                    class_assistant = LLM(client)
                    response = class_assistant.action(query, prompt) #get response
                    if not response or not isinstance(response, dict):
                        print("llm exception:{}".format(response))
                    action_info = response.get("action")
                    action_name = action_info.get("name")
                    action_args = action_info.get("args")

                try: #action
                    func = tools_map.get(action_name)
                    call_function_result = func(RPi, action_args)
                except Exception as e:
                    print("error:{}".format(e))

                end_time = time.time()
                print(end_time - start_time)
                query = ""
                
voice_control(mqtt_rpi)