import os
import sys
import urllib.request
import argparse
def progress_callback(block_num, block_size, total_size):
    """Fungsi pembantu untuk menampilkan progress bar unduhan di terminal."""
    downloadED = block_num * block_size
    if total_size > 0:
        percent = min(int(downloadED * 100 / total_size), 100)
        downloaded_mb = downloadED / (1024 * 1024)
        total_mb = total_size / (1024 * 1024)
        
        sys.stdout.write(f"\r[DOWNLOADING] Progress: {percent}% ({downloaded_mb:.1f}/{total_mb:.1f} MB)")
        sys.stdout.flush()

def define_model(model_url:str):
    MODEL_URL = model_url if model_url else "https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF/resolve/main/qwen1_5-1_8b-chat-q4_k_m.gguf"
    
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_dir = os.path.join(base_dir, "model")
    target_file = os.path.join(target_dir, "qwen-1_5b.gguf")

    if not os.path.exists(target_dir):
        print(f"[INFO] Membuat direktori penyimpanan model lokal di: {target_dir}")
        os.makedirs(target_dir)

    if os.path.exists(target_file):
        print(f"[SKIP] Model sudah terpasang di: {target_file}")
        return

    print(f"[START] Mengunduh model biner dari HuggingFace...")
    try:
        urllib.request.urlretrieve(MODEL_URL, target_file, progress_callback)
        print("\n[SUCCESS] Model GGUF berhasil diunduh dan dikunci di dalam folder model/.")
    except Exception as e:
        print(f"\n[ERROR] Gagal mengunduh model: {e}")
        if os.path.exists(target_file):
            os.remove(target_file)
        sys.exit(1)

if __name__ == "__main__":
    

    parser = argparse.ArgumentParser(description="Setup Qwen model for ChatboxAI")
    parser.add_argument("--model-url", type=str, help="URL of the model to download")
    
    args = parser.parse_args()
    define_model(model_url=args.model_url)  