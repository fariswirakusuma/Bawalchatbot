import os
import sys
import json
import argparse
import logging
import time
import re
from openai import OpenAI

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def execute_gemini_agent(mode: str, user_prompt: str, model_type: str, update_file: str,token :int):
    base_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    config_path = os.path.join(base_path, ".gemini", "workflows", f"{mode}.json")
    
    if not os.path.exists(config_path):
        logging.error(f"Berkas konfigurasi tidak ditemukan: {config_path}")
        sys.exit(1)
        
    with open(config_path, 'r') as f:
        config_data = json.load(f)
        
    agent_config = config_data.get("agent_config", {})
    scope = agent_config.get("scope", {})
    target_dirs = scope.get("target_directories", ["src/src", "src/include", "src", "src/data", "data"])
    allowed_exts = scope.get("allowed_extensions", [".cpp", ".hpp", ".txt", ".h", ".json", ".csv"])

    attached_code = ""
    found_files = set()
    project_map = [] 

    if update_file:
        full_update_path = os.path.join(base_path, update_file)
        if os.path.exists(full_update_path):
            found_files.add(full_update_path)
            logging.info(f"[GATEWAY] Melampirkan berkas target update: {update_file}")
            try:
                with open(full_update_path, 'r', encoding='utf-8') as code_file:
                    attached_code += f"\n\n--- FILE: {update_file} ---\n{code_file.read()}"
            except Exception as e:
                logging.warning(f"Gagal membaca berkas target {update_file}: {e}")

    for target_dir in target_dirs:
        full_dir_path = os.path.join(base_path, target_dir)
        if not os.path.exists(full_dir_path):
            continue
            
        for root, _, files in os.walk(full_dir_path):
            for file_name in files:
                if any(file_name.endswith(ext) for ext in allowed_exts):
                    file_path = os.path.join(root, file_name)
                    rel_path = os.path.relpath(file_path, base_path)
                    project_map.append(rel_path)
                    
                    if file_name in user_prompt and file_path not in found_files:
                        found_files.add(file_path)
                        logging.info(f"[GATEWAY] Melampirkan berkas: {rel_path}")
                        try:
                            with open(file_path, 'r', encoding='utf-8') as code_file:
                                attached_code += f"\n\n--- FILE: {rel_path} ---\n{code_file.read()}"
                        except Exception as e:
                            logging.warning(f"Gagal membaca berkas {rel_path}: {e}")

    final_payload = user_prompt
    if project_map:
        final_payload += "\n\n[SYSTEM INFO] Daftar berkas di workspace saat ini:\n- " + "\n- ".join(project_map)
    if attached_code:
        final_payload += "\n\n[SYSTEM INFO] Konteks berkas lokal yang diminta:\n" + attached_code
    
    combined_system_instruction = ""
    if "agent_config" in config_data:
        instructions_list = agent_config.get("instructions", [])
        agent_instructions = " ".join(instructions_list) if isinstance(instructions_list, list) else instructions_list
        workflow_config = config_data.get("workflow_config", {})
        workflow_instruction = workflow_config.get("ai_context", {}).get("system_instruction", "")
        combined_system_instruction = f"{agent_instructions}\n\nKonteks Tambahan:\n{workflow_instruction}"
    else:
        workflow_config = config_data.get("workflow_config", config_data)
        combined_system_instruction = workflow_config.get("ai_context", {}).get("system_instruction", "Kamu adalah asisten AI.")

    target_tag = update_file if update_file else "path/ke/file.ekstensi"
    auto_save_directive = (
        f"\n\n[SISTEM AUTO-SAVE AKTIF]\n"
        f"Jika Anda memberikan perbaikan kode atau file baru, Anda WAJIB menggunakan format berikut agar skrip dapat menimpanya secara otomatis:\n"
        f"[UPDATE_FILE: {target_tag}]\n"
        f"```\n"
        f"TULIS_SELURUH_KODE_DISINI\n"
        f"```\n"
    )
    combined_system_instruction += auto_save_directive
    
    model_mapping = {
        "pro": "google/gemini-2.5-pro",
        "pro-3.1": "google/gemini-3.1-pro",
        "pro-3.5": "google/gemini-3.5-pro",
        "flash-3.5": "google/gemini-3.5-flash",
        "flash-3.1": "google/gemini-3.1-flash",
        "flash": "google/gemini-2.5-flash"
    }
    selected_model = model_mapping.get(model_type.lower(), "google/gemini-2.5-pro")
    logging.info(f"Menggunakan model OpenRouter: {selected_model}")

    api_key = os.getenv("OPENROUTER_API_KEY", "")
    if not api_key:
        logging.error("Environment variable OPENROUTER_API_KEY tidak ditemukan.")
        sys.exit(1)

    client = OpenAI(
        base_url="https://openrouter.ai/api/v1",
        api_key=api_key,
        default_headers={
            "HTTP-Referer": "https://localhost:3000",
            "X-Title": "Local Code Agent Backend"
        }
    )
    
    logging.info(f"Mengirim request ke OpenRouter untuk mode {mode}...")
    
    response_text = ""
    max_retries = 3
    for attempt in range(1, max_retries + 1):
        try:
            completion = client.chat.completions.create(
                model=selected_model,
                messages=[
                    {"role": "system", "content": combined_system_instruction},
                    {"role": "user", "content": final_payload}
                ],
                temperature=0.1,
                max_tokens=token
            )
            response_text = completion.choices[0].message.content
            break
        except Exception as e:
            logging.error(f"Percobaan {attempt}/{max_retries} gagal: {e}")
            if attempt == max_retries:
                logging.error("API OpenRouter gagal diakses setelah 3 percobaan.")
                sys.exit(1)
            time.sleep(2)

    patch_pattern = r'\[UPDATE_FILE:\s*(.+?)\]\s*```[a-zA-Z]*\n(.*?)```'
    matches = list(re.finditer(patch_pattern, response_text, re.DOTALL))

    if matches:
        logging.info("[AUTO-SAVE] Mendeteksi patch kode dari agen AI.")
        for match in matches:
            target_file = match.group(1).strip()
            new_code = match.group(2)
            
            full_target_path = os.path.join(base_path, target_file)
            os.makedirs(os.path.dirname(full_target_path), exist_ok=True)
            
            if os.path.exists(full_target_path):
                backup_path = full_target_path + ".bak"
                if os.path.exists(backup_path):
                    os.remove(backup_path)
                os.rename(full_target_path, backup_path)
                logging.info(f"[AUTO-SAVE] Backup dibuat: {target_file}.bak")
                
            with open(full_target_path, 'w', encoding='utf-8') as f:
                f.write(new_code)
            logging.info(f"[AUTO-SAVE] Berhasil menulis/menimpa berkas: {target_file}")

    return response_text

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Gemini Agent Gateway Engine")
    parser.add_argument("--mode", type=str, required=True, help="Mode alur kerja (contoh: debugger, web_builder)")
    parser.add_argument("--prompt", type=str, required=True, help="Prompt atau log error mentah untuk dianalisis")
    parser.add_argument("--model-type", type=str, default="pro", choices=["pro", "flash", "flash-3.5", "flash-3.1", "pro-3.1", "pro-3.5"], 
                        help="Tipe model Gemini yang digunakan: pro atau flash (default: pro)")
    # Registrasi flag baru
    parser.add_argument("--update-file", type=str, default="", help="Path berkas target untuk diupdate otomatis")
    parser.add_argument("--token", type=int, default=4000, help="Batas maksimum output token")
    args = parser.parse_args()
    
    output = execute_gemini_agent(
        mode=args.mode, 
        user_prompt=args.prompt, 
        model_type=args.model_type, 
        update_file=args.update_file,
        token=args.token
    )
    print(output)