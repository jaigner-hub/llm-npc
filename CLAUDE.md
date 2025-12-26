# CLAUDE.md - Project Context for Claude Code

## Project Overview

This is a real-time voice chat application that allows users to speak with NPC characters. It combines speech-to-text (whisper.cpp), LLM processing (Claude Haiku via OpenRouter), and text-to-speech (Piper TTS).

## Architecture

```
Microphone → Whisper STT → Claude Haiku → Piper TTS → Speakers
     ↓              ↓             ↓            ↓
  SDL2 input    transcribe    respond     synthesize
```

## Key Files

| File | Purpose |
|------|---------|
| `voice_chat.cpp` | Main application loop, coordinates all components |
| `audio_capture.cpp/h` | SDL2 audio input with Voice Activity Detection |
| `audio_playback.cpp/h` | SDL2 audio output queue |
| `npc_chat.cpp/h` | OpenRouter API client for Claude Haiku |
| `npc_config.h` | NPC configuration framework (world, persona, quests) |
| `CMakeLists.txt` | Build configuration |

## Dependencies

- **whisper.cpp**: Located at `C:/msys64/whisper.cpp`, provides STT
- **libpiper**: Located at `piper1-gpl/libpiper`, provides TTS
- **SDL2**: Audio I/O
- **libcurl**: HTTP requests to OpenRouter API
- **OpenMP**: Required by whisper's ggml-cpu

## Build Environment

- Windows with MSYS2 (UCRT64 toolchain)
- CMake + Ninja
- GCC 15.x

## Common Build Issues

1. **Missing DLLs**: Copy all required DLLs to build/ directory (see README.md)
2. **SSL certificate errors**: Code hardcodes CA bundle path to `C:/msys64/usr/ssl/certs/ca-bundle.crt`
3. **espeak dllimport errors**: Headers patched to check for ESPEAK_STATIC define
4. **piper symbols not exported**: Uses .def file for MinGW (WINDOWS_EXPORT_ALL_SYMBOLS only works with MSVC)

## NPC Configuration System

The `NPCConfig` struct combines three elements:

1. **World Background**: Game world context, location, time, weather, events
2. **Persona**: Name, role, personality, speech style, backstory, knowledge
3. **Quests/Events**: Available quests with trigger phrases

`buildPrompt()` combines all elements into a system prompt for Claude.

## API

Uses OpenRouter API (`https://openrouter.ai/api/v1/chat/completions`) with:
- Model: `anthropic/claude-3-haiku`
- Max tokens: 100 (short responses)
- Environment variable: `OPENROUTER_API_KEY`

## Audio Parameters

- Whisper sample rate: 16000 Hz
- Piper sample rate: 22050 Hz
- VAD threshold: 500ms silence
- Capture buffer: 5000ms

## Development Notes

- The main loop uses Voice Activity Detection to know when user stops speaking
- Conversation history is maintained in `NPCChat::m_history` for context
- Audio playback is queued and non-blocking
- Ctrl+C cleanly shuts down all components
