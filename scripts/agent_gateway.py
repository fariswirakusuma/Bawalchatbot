import os
import sys
import json
import argparse
import logging
from google import genai
from google.genai import types

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def execute_gemini_agent(mode: str, user_prompt: str, model_type: str):
    base_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    config_path = os.path.join(base_path, ".gemini", "workflows", f"{mode}.json")
    
    if not os.path.exists(config_path):
        logging.error(f"Berkas konfigurasi tidak ditemukan: {config_path}")
        sys.exit(1)
        
    with open(config_path, 'r') as f:
        policy = json.load(f)
        
    client = genai.Client()
    
    model_mapping = {
        "pro": "gemini-2.5-pro",
        "flash": "gemini-2.5-flash"
    }
    selected_model = model_mapping.get(model_type.lower(), "gemini-2.5-pro")
    logging.info(f"Menggunakan model: {selected_model}")

    agent_data = policy.get("agent_config", policy)
    instructions = agent_data.get("instructions", [])
    
    system_instruction = " ".join(instructions) if isinstance(instructions, list) else instructions

    config = types.GenerateContentConfig(
        system_instruction=system_instruction,
        temperature=0.1,  
        max_output_tokens=2048
    )
    
    logging.info(f"Mengirim request ke Gemini untuk mode {mode}...")
    try:
        response = client.models.generate_content(
            model=selected_model,
            contents=user_prompt,
            config=config
        )
        return response.text
    except Exception as e:
        logging.error(f"Gagal mengeksekusi API Gemini: {e}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Gemini Agent Gateway Engine")
    parser.add_argument("--mode", type=str, required=True, help="Mode alur kerja (contoh: debugger, web_builder)")
    parser.add_argument("--prompt", type=str, required=True, help="Prompt atau log error mentah untuk dianalisis")
    parser.add_argument("--model-type", type=str, default="pro", choices=["pro", "flash"], 
                        help="Tipe model Gemini yang digunakan: pro atau flash (default: pro)")
    
    args = parser.parse_args()
    
    output = execute_gemini_agent(mode=args.mode, user_prompt=args.prompt, model_type=args.model_type)
    print(output)