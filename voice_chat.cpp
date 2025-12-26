#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "whisper.h"
#include "ggml-backend.h"
#include "piper.h"
#include "npc_chat.h"
#include "audio_capture.h"
#include "audio_playback.h"

struct voice_chat_params {
    std::string whisper_model = "";
    std::string piper_model = "";
    std::string piper_config = "";
    std::string espeak_data = "";

    int capture_id = -1;
    int n_threads = 4;

    float vad_thold = 0.6f;
    float freq_thold = 100.0f;

    int step_ms = 3000;
    int length_ms = 10000;
};

void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -wm, --whisper-model <path>  Path to whisper model (.bin)\n");
    fprintf(stderr, "  -pm, --piper-model <path>    Path to piper voice model (.onnx)\n");
    fprintf(stderr, "  -pc, --piper-config <path>   Path to piper config (.onnx.json)\n");
    fprintf(stderr, "  -ed, --espeak-data <path>    Path to espeak-ng data directory\n");
    fprintf(stderr, "  -c,  --capture <id>          Capture device ID (default: -1 for default)\n");
    fprintf(stderr, "  -t,  --threads <n>           Number of threads (default: 4)\n");
    fprintf(stderr, "  -h,  --help                  Show this help\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Environment:\n");
    fprintf(stderr, "  OPENROUTER_API_KEY           Required for Claude Haiku API\n");
}

bool parse_args(int argc, char** argv, voice_chat_params& params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        }
        else if ((arg == "-wm" || arg == "--whisper-model") && i + 1 < argc) {
            params.whisper_model = argv[++i];
        }
        else if ((arg == "-pm" || arg == "--piper-model") && i + 1 < argc) {
            params.piper_model = argv[++i];
        }
        else if ((arg == "-pc" || arg == "--piper-config") && i + 1 < argc) {
            params.piper_config = argv[++i];
        }
        else if ((arg == "-ed" || arg == "--espeak-data") && i + 1 < argc) {
            params.espeak_data = argv[++i];
        }
        else if ((arg == "-c" || arg == "--capture") && i + 1 < argc) {
            params.capture_id = std::stoi(argv[++i]);
        }
        else if ((arg == "-t" || arg == "--threads") && i + 1 < argc) {
            params.n_threads = std::stoi(argv[++i]);
        }
        else {
            fprintf(stderr, "Unknown argument: %s\n", arg.c_str());
            print_usage(argv[0]);
            return false;
        }
    }

    if (params.whisper_model.empty()) {
        fprintf(stderr, "Error: Whisper model path required (-wm)\n");
        return false;
    }
    if (params.piper_model.empty()) {
        fprintf(stderr, "Error: Piper model path required (-pm)\n");
        return false;
    }
    if (params.espeak_data.empty()) {
        fprintf(stderr, "Error: espeak-ng data path required (-ed)\n");
        return false;
    }

    return true;
}

// Trim whitespace and filter out whisper artifacts
std::string clean_transcription(const std::string& text) {
    std::string result;

    // Skip leading whitespace
    size_t start = 0;
    while (start < text.size() && std::isspace(text[start])) {
        start++;
    }

    // Skip trailing whitespace
    size_t end = text.size();
    while (end > start && std::isspace(text[end - 1])) {
        end--;
    }

    result = text.substr(start, end - start);

    // Filter out common whisper artifacts
    if (result == "[BLANK_AUDIO]" || result == "(silence)" ||
        result == "[silence]" || result == "[inaudible]" ||
        result.empty()) {
        return "";
    }

    return result;
}

int main(int argc, char** argv) {
    voice_chat_params params;

    if (!parse_args(argc, argv, params)) {
        return 1;
    }

    // Check for API key
    const char* api_key = std::getenv("OPENROUTER_API_KEY");
    if (!api_key || strlen(api_key) == 0) {
        fprintf(stderr, "Error: OPENROUTER_API_KEY environment variable not set\n");
        return 1;
    }
    fprintf(stderr, "API key loaded (length: %zu)\n", strlen(api_key));

    fprintf(stderr, "Initializing voice chat...\n");

    // Initialize ggml backends
    ggml_backend_load_all();

    // Initialize Whisper
    fprintf(stderr, "Loading whisper model: %s\n", params.whisper_model.c_str());
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = true;
    cparams.flash_attn = true;

    whisper_context* ctx = whisper_init_from_file_with_params(params.whisper_model.c_str(), cparams);
    if (!ctx) {
        fprintf(stderr, "Error: Failed to load whisper model\n");
        return 1;
    }

    // Initialize Piper
    fprintf(stderr, "Loading piper model: %s\n", params.piper_model.c_str());
    const char* piper_cfg = params.piper_config.empty() ? nullptr : params.piper_config.c_str();
    piper_synthesizer* synth = piper_create(params.piper_model.c_str(), piper_cfg, params.espeak_data.c_str());
    if (!synth) {
        fprintf(stderr, "Error: Failed to create piper synthesizer\n");
        whisper_free(ctx);
        return 1;
    }

    // Initialize NPC Chat
    // Create NPC with full config
    NPCConfig npcConfig = createGuardNPC();
    NPCChat npc(api_key, npcConfig);

    // Initialize audio capture
    AudioCapture capture(params.length_ms);
    if (!capture.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        fprintf(stderr, "Error: Failed to initialize audio capture\n");
        piper_free(synth);
        whisper_free(ctx);
        return 1;
    }

    // Initialize audio playback
    AudioPlayback playback;
    piper_synthesize_options piper_opts = piper_default_synthesize_options(synth);
    // Get sample rate from a test synthesis
    if (!playback.init(22050)) {  // Default piper sample rate
        fprintf(stderr, "Error: Failed to initialize audio playback\n");
        piper_free(synth);
        whisper_free(ctx);
        return 1;
    }

    // Set up whisper parameters
    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.translate = false;
    wparams.single_segment = true;
    wparams.no_context = true;
    wparams.language = "en";
    wparams.n_threads = params.n_threads;

    // Start capturing
    capture.resume();

    fprintf(stderr, "\n");
    fprintf(stderr, "=== Voice Chat with %s ===\n", npc.getName().c_str());
    fprintf(stderr, "Speak into your microphone. Press Ctrl+C to quit.\n");
    fprintf(stderr, "\n");

    std::vector<float> pcmf32;
    std::vector<float> pcmf32_vad;

    bool is_running = true;
    bool was_speaking = false;
    int silence_count = 0;
    const int silence_threshold = 3;  // Number of silent iterations before processing

    while (is_running) {
        is_running = sdl_poll_events();
        if (!is_running) break;

        // Get recent audio for VAD check
        capture.get(2000, pcmf32_vad);

        if (pcmf32_vad.size() < 1600) {  // Need at least 100ms at 16kHz
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Check for voice activity
        bool is_speaking = !vad_simple(pcmf32_vad, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, false);

        if (is_speaking) {
            was_speaking = true;
            silence_count = 0;
        } else if (was_speaking) {
            silence_count++;

            // User stopped speaking, process the audio
            if (silence_count >= silence_threshold) {
                was_speaking = false;
                silence_count = 0;

                // Get the full audio buffer
                capture.get(params.length_ms, pcmf32);

                if (pcmf32.size() < 1600) {
                    continue;
                }

                // Run whisper inference
                if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                    fprintf(stderr, "Whisper inference failed\n");
                    continue;
                }

                // Get transcription
                std::string transcription;
                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; i++) {
                    const char* text = whisper_full_get_segment_text(ctx, i);
                    transcription += text;
                }

                transcription = clean_transcription(transcription);

                if (transcription.empty()) {
                    continue;
                }

                fprintf(stderr, "You: %s\n", transcription.c_str());

                // Get response from Claude Haiku
                std::string response = npc.chat(transcription);
                fprintf(stderr, "%s: %s\n", npc.getName().c_str(), response.c_str());

                // Synthesize and play response
                if (piper_synthesize_start(synth, response.c_str(), &piper_opts) == PIPER_OK) {
                    piper_audio_chunk chunk;
                    while (piper_synthesize_next(synth, &chunk) == PIPER_OK) {
                        playback.queue(chunk.samples, chunk.num_samples);
                    }
                    playback.waitComplete();
                }

                // Clear the audio buffer after processing
                capture.clear();

                fprintf(stderr, "\n[Listening...]\n");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    fprintf(stderr, "\nShutting down...\n");

    capture.pause();
    playback.clear();
    piper_free(synth);
    whisper_free(ctx);

    return 0;
}
