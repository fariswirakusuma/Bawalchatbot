import os
import sys
import json
import argparse
import logging
from google import genai
from google.genai import types

def execute_gemini_agent(mode: str, user_prompt: str):
    base_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    config_path = os.path.join(base_path, ".gemini", "workflows", f"{mode}.json")
    
    with open(config_path, 'r') as f:
        policy = json.load(f)
    client = genai.Client()
    
    config = types.GenerateContentConfig(
        system_instruction=" ".join(policy["instructions"]),
        temperature=0.1,  
        max_output_tokens=2048
    )
    logging.info(f"Mengirim request ke Gemini untuk mode {mode}...")
    response = client.models.generate_content(
        model='gemini-2.5-pro',
        contents=user_prompt,
        config=config
    )
    
    return response.text