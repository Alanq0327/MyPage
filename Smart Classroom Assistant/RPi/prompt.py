from tool import gen_tools_desc
#prompt for the llm to receive correct response
class_assistant_prompt = """
You are a classroom assistant. You need to identify the instructor's intended action and then execute.
If the instruction is irrelevant or does not describe an actionable step, return an action with name "none".

resources:
{resources}

Actions: These are the only actions you can perform. Any of your operations must be realized through the following actions.
{actions}

Constraints:
- Do NOT include ```json in the response.
- Output only in the EXACT following format:
{response_format_prompt}

The query is as follows.
"""

resources = [
    "capable of turn on the light",
    "capable of turn off the light",
    "capable of change brightness off the light",
    "capable of turn on the fan",
    "capable of turn off the fan",
    "capable of turn on the heater",
    "capable of turn off the heater",
    "capable of start the projector",
    "capable of stop the projector",
    "capable of start the projector in movie mode"
]

response_format_prompt = """
{
    "action": {
        "name": "action name",
        "args": "args value"
    }
}
"""

resources_prompt = "\n".join([f"{idx+1}. {con}" for idx, con in enumerate(resources)])
action_prompt = gen_tools_desc()

def gen_prompt():
    prompt = class_assistant_prompt.format(
        actions=action_prompt,
        resources=resources_prompt,
        response_format_prompt=response_format_prompt
    )
    return prompt
