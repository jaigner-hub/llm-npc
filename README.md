# Voice Chat - NPC Voice Assistant

A voice-based NPC chat system that integrates:
- **Speech-to-Text**: whisper.cpp for real-time transcription
- **LLM**: Claude Haiku via OpenRouter API
- **Text-to-Speech**: Piper TTS for voice synthesis

Speak to an NPC character and hear them respond in real-time.

## Prerequisites

- Windows with MSYS2 (UCRT64 environment)
- OpenRouter API key (https://openrouter.ai/)

## Install Dependencies

Open MSYS2 UCRT64 terminal:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2 \
          mingw-w64-ucrt-x86_64-curl git
```

## Clone

```bash
git clone --recursive git@github.com:jaigner-hub/llm-npc.git
cd llm-npc
```

If you already cloned without `--recursive`:
```bash
git submodule update --init --recursive
```

## Build

### Quick Build (recommended)

```bash
./build.sh
```

### Manual Build

#### 1. Build whisper.cpp

```bash
cd whisper.cpp
cmake -B build -G Ninja
cmake --build build

# Download a model
./models/download-ggml-model.sh base.en
cd ..
```

#### 2. Build libpiper

```bash
cd piper1-gpl/libpiper
cmake -B build -G Ninja -DCMAKE_INSTALL_PREFIX=install
cmake --build build
cmake --install build
cd ../..
```

#### 3. Build voice_chat

```bash
cmake -B build -G Ninja
cmake --build build
```

#### 4. Copy DLLs

```bash

# Core libraries
cp piper1-gpl/libpiper/install/libpiper.dll build/
cp piper1-gpl/libpiper/install/lib/onnxruntime.dll build/

# Runtime DLLs
cp /ucrt64/bin/libgomp-1.dll build/
cp /ucrt64/bin/libstdc++-6.dll build/
cp /ucrt64/bin/libgcc_s_seh-1.dll build/
cp /ucrt64/bin/libwinpthread-1.dll build/
cp /ucrt64/bin/SDL2.dll build/
cp /ucrt64/bin/libcurl-4.dll build/

# Curl dependencies
cp /ucrt64/bin/zlib1.dll build/
cp /ucrt64/bin/libbrotlidec.dll build/
cp /ucrt64/bin/libbrotlicommon.dll build/
cp /ucrt64/bin/libidn2-0.dll build/
cp /ucrt64/bin/libnghttp2-14.dll build/
cp /ucrt64/bin/libpsl-5.dll build/
cp /ucrt64/bin/libssh2-1.dll build/
cp /ucrt64/bin/libssl-3-x64.dll build/
cp /ucrt64/bin/libcrypto-3-x64.dll build/
cp /ucrt64/bin/libzstd.dll build/
cp /ucrt64/bin/libunistring-5.dll build/
cp /ucrt64/bin/libiconv-2.dll build/
cp /ucrt64/bin/libintl-8.dll build/
cp /ucrt64/bin/libnghttp3-9.dll build/
cp /ucrt64/bin/libngtcp2-16.dll build/
cp /ucrt64/bin/libngtcp2_crypto_ossl-0.dll build/
```

## Run

### From MSYS2:

```bash
export OPENROUTER_API_KEY=sk-or-v1-your-key-here
./build/voice_chat.exe \
    -wm whisper.cpp/models/ggml-base.en.bin \
    -pm models/en_US-lessac-medium.onnx \
    -ed piper1-gpl/libpiper/install/espeak-ng-data
```

### From Windows cmd.exe:

```cmd
cd build
set OPENROUTER_API_KEY=sk-or-v1-your-key-here
voice_chat.exe -wm ..\whisper.cpp\models\ggml-base.en.bin -pm ..\models\en_US-lessac-medium.onnx -ed ..\piper1-gpl\libpiper\install\espeak-ng-data
```

## Command Line Options

| Option | Description |
|--------|-------------|
| `-wm <path>` | Path to Whisper model (ggml format) |
| `-pm <path>` | Path to Piper voice model (.onnx) |
| `-ed <path>` | Path to espeak-ng-data directory |
| `-t <ms>` | VAD threshold in ms (default: 500) |
| `-l <ms>` | Audio capture length in ms (default: 5000) |

## Project Structure

```
llm-npc/
├── voice_chat.cpp      # Main application
├── audio_capture.cpp/h # SDL2 microphone input with VAD
├── audio_playback.cpp/h# SDL2 audio output
├── npc_chat.cpp/h      # Claude API client
├── npc_config.h        # NPC configuration framework
├── CMakeLists.txt      # Build configuration
├── models/             # Piper voice models
├── whisper.cpp/        # Speech-to-text (submodule)
└── piper1-gpl/         # Text-to-speech (submodule)
    └── libpiper/
```

## NPC Configuration

NPCs are configured using the `NPCConfig` struct in `npc_config.h`:

```cpp
NPCConfig config;

// World context
config.worldName = "Eldoria";
config.currentLocation = "Village Square";
config.recentEvents = {"Dragon spotted nearby"};

// Character
config.name = "Gareth";
config.role = "village guard";
config.personality = "gruff but kind";
config.speechStyle = "direct, practical";

// Quests
NPCQuest quest;
quest.name = "Wolf Problem";
quest.description = "Collect 5 wolf pelts";
config.quests.push_back(quest);

NPCChat npc(api_key, config);
```

## License

- libpiper: GPL-3.0
- whisper.cpp: MIT
- Piper voices: Various (check individual model licenses)
