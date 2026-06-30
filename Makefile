CXX          := g++
CXXFLAGS     := -std=c++17 -Wall -Wextra -O3 -Isrc/include

SRC_DIR      := src
DIST_DIR     := dist
WWW_DIR      := www 

CXX_SOURCES  := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/src/*.cpp)
TARGET_BIN   := $(DIST_DIR)/chatbox_core

.PHONY: all clean build_backend build_frontend run run_cli setup_model

all: build_backend

build_backend:
	@echo "[BUILD] Mengompilasi Core Engine C++ (Termasuk subfolder src/src/)..."
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) $(CXX_SOURCES) -o $(TARGET_BIN)
	@echo "[SUCCESS] Biner Backend C++ siap di: $(TARGET_BIN)"

build_frontend:
	@echo "[BUILD] Mengompilasi Aset UI..."
	@mkdir -p $(WWW_DIR)
	@# Anda bisa mengaktifkan baris di bawah jika folder 'www' sudah siap dengan package.json
	@# cd $(WWW_DIR) && npm install && npm run build
	@echo "[SUCCESS] Aset UI Statis siap."

run: build_backend build_frontend
	@echo "[LAUNCH] Membuka Aplikasi Chatbox Desktop..."
	@# Pemicu start disesuaikan dengan folder root UI Anda
	@cd $(WWW_DIR) && npm run start &
	@echo "[SYSTEM] Backend Engine berjalan otomatis di background."

run_cli: build_backend
	@echo "[LAUNCH] Menjalankan Chatbox Engine Mode CLI..."
	@./$(TARGET_BIN) --cli

setup_model:
	@echo "[SYSTEM] Menjalankan automasi instalasi model AI lokal via Python..."
	python3 scripts/setup_model.py

clean:
	@echo "[CLEAN] Menghapus seluruh artefak produksi biner C++..."
	rm -rf $(DIST_DIR)
	@echo "[SUCCESS] Cache kompilasi dibersihkan. Model lokal tetap aman."

clean_all: clean
	@echo "[CLEAN] Menghapus folder model AI..."
	rm -rf model
	rm -rf src/model
	@echo "[SUCCESS] Seluruh ruang penyimpanan dibersihkan total."