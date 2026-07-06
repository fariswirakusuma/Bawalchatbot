# BawalChatbot Engine

<p align="center">
  <a href="https://electronjs.org/">
    <img src="https://img.shields.io/badge/Electron-v30+-blueviolet?style=for-the-badge&logo=electron&logoColor=white" alt="Electron v30+">
  </a>
  <a href="https://react.dev/">
    <img src="https://img.shields.io/badge/React-v18+-cyan?style=for-the-badge&logo=react&logoColor=white" alt="React v18+">
  </a>
  <a href="https://isocpp.org/">
    <img src="https://img.shields.io/badge/C%2B%2B-17-orange?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++17">
  </a>
  <a href="https://cmake.org/">
    <img src="https://img.shields.io/badge/CMake-v3.15+-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
  </a>
</p>

---

## 1. System Architecture

BawalChatbot utilizes a multi-process architecture that decouples the user interface (Frontend) from the native inference engine (C++ Backend). This design ensures that the UI thread remains highly responsive and free from blocking operations via a non-blocking event loop.


---

## 2. Frontend Architecture (Electron & React)

The application frontend is located in the `www/bawalchatbox` directory and is built using a modern desktop web stack:
*   **Electron (v30+)**: Serves as the desktop shell runtime, wrapping the web application and providing secure access to low-level operating system APIs.
*   **React (v18+) & TypeScript**: Used to build reactive, structured, and type-safe user interface components.
*   **TailwindCSS**: Used for utility-first styling, compiled rapidly via LightningCSS.

### IPC (Inter-Process Communication) Data Flow
1.  **User Initiation**: The user inputs a prompt or command in the React interface.
2.  **Renderer to Main**: React dispatches the message payload to the Electron Main process using `window.electron.ipcRenderer.send('generate-text', payload)`.
3.  **Main to Native**: The Electron Main process receives the message, validates the parameters, and writes the structured data to the standard input (`stdin`) stream of the running C++ subprocess.
4.  **Native to Main (Streaming)**: The C++ engine processes tokens incrementally and writes the output to the standard output (`stdout`) stream.
5.  **Main to Renderer**: Electron Main captures these data chunks from `stdout` in real-time and forwards them to the Renderer process via `webContents.send('token-stream', chunk)`.
6.  **UI Update**: React listens to these stream events and dynamically updates the component state to render a real-time typewriter effect.

---

## 3. Core C++ Inference Engine

The native backend located in the `src` directory is responsible for executing local language models efficiently without heavy external runtime dependencies.

### llama.cpp Integration & Memory Management
*   **llama.cpp**: The inference engine is integrated directly at the C++ source level to execute GGUF format models optimally on local CPU/GPU architectures.
*   **Zero-Copy Memory Mapping (mmap)**: The C++ engine maps the GGUF model file directly into the virtual address space of the process using `mmap` system calls (or `VirtualAlloc` on Windows). This prevents duplicate memory allocations, accelerates application startup times, and allows the operating system to manage memory pages efficiently via page caching.

### Bidirectional Communication Protocol (stdio Streams)
Communication between the Node.js process (Electron Main) and the C++ executable is handled via standard pipes:
*   **stdin**: Used by Electron Main to send control instructions and structured text prompts to the C++ engine.
*   **stdout**: Used by the C++ engine to stream inferred tokens back to Electron Main in raw or structured JSON format.
*   **stderr**: Reserved exclusively for diagnostic logs, performance metrics (tokens per second), and system initialization status to keep the primary data path on `stdout` clean.

---

## 4. CLI AI Agent Subsystem

The AI Agent subsystem is controlled externally through the `agent_gateway.py` script. This script acts as an automation orchestrator for code modifications and documentation updates within the repository.

```
+-------------------------------------------------------------------------+
|                          agent_gateway.py                               |
|                                                                         |
|  +--------------------+      Workflow JSON      +--------------------+  |
|  |   Read Workflow    | <---------------------> | .gemini/workflows/ |  |
|  +--------------------+                         +--------------------+  |
|            |                                                            |
|            v                                                            |
|  +--------------------+      API Request        +--------------------+  |
|  |  Gateway Connection| <---------------------> |   OpenRouter API   |  |
|  +--------------------+                         +--------------------+  |
|            |                                                            |
|            v                                                            |
|  +--------------------+                                                 |
|  |  Payload Analysis  | (Reads multi-file arrays & patch instructions)  |
|  +--------------------+                                                 |
|            |                                                            |
|            v                                                            |
|  +--------------------+      Backup File        +--------------------+  |
|  |  Fallback System   | ----------------------> |     Create .bak    |  |
|  +--------------------+                         +--------------------+  |
|            |                                                            |
|            v                                                            |
|  +--------------------+      Regex Match        +--------------------+  |
|  |   Code Patching    | ----------------------> |   Write New Code   |  |
|  +--------------------+                         +--------------------+  |
+-------------------------------------------------------------------------+
```

### Operational Mechanism of `agent_gateway.py`
1.  **Workflow Parsing**: The script reads JSON configuration files from the `.gemini/workflows/` directory to determine target files and modification instructions.
2.  **OpenRouter API Consumption**: Sends system prompts, current code context, and modification instructions to the AI model via the OpenRouter endpoint using secure token authentication.
3.  **Multi-File Processing**: Analyzes and modifies multiple files simultaneously within a single execution cycle to maintain cross-file reference consistency.
4.  **Auto-Save & Code Patching**:
    *   Before applying modifications, the script creates a backup copy of the target files with a `.bak` extension as a fallback mechanism.
    *   Uses precise regular expressions (Regex) to locate specific code blocks that require replacement.
    *   If the writing or compilation process fails after patching, the script automatically restores the original files from the `.bak` backups to prevent source code corruption.

---

## 5. Automation Command Reference (Taskfile)

This project utilizes a Taskfile runner to automate the development, compilation, testing, and maintenance lifecycles. Below is the list of available commands:

| Task Command | Environment Variables | Function & Description |
| :--- | :--- | :--- |
| `task build:core` | `CMAKE_BUILD_TYPE` | Compiles the Core C++ Inference Engine using CMake and the system's native compiler. |
| `task build:ui` | `NODE_ENV` | Compiles the React + Tailwind frontend assets in the `www/bawalchatbox` directory using Vite. |
| `task run` | `DEBUG` | Runs the Electron application in development mode. If `DEBUG=true`, developer tools open automatically and verbose logging is enabled. |
| `task test:core` | - | Runs native C++ unit tests to verify parser functionality and llama.cpp integration. |
| `task agent` | `OPENROUTER_API_KEY`, `WORKFLOW` | Executes the AI agent subsystem (`agent_gateway.py`) by loading a specified workflow (e.g., `WORKFLOW=ui_builder`). |
| `task clean` | - | Cleans the C++ build directories (`build/`, `bin/`) and removes frontend distribution folders (`dist/`, `node_modules/`). |