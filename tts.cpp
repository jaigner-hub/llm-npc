#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <piper.h>

// WAV file header structure
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t file_size;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 3; // IEEE float
    uint16_t num_channels = 1;
    uint32_t sample_rate = 22050;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample = 32;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
};

void write_wav(const std::string &filename, const std::vector<float> &samples,
               int sample_rate) {
    WavHeader header;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 32;
    header.num_channels = 1;
    header.byte_rate = sample_rate * header.num_channels * (header.bits_per_sample / 8);
    header.block_align = header.num_channels * (header.bits_per_sample / 8);
    header.data_size = samples.size() * sizeof(float);
    header.file_size = sizeof(WavHeader) - 8 + header.data_size;

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(samples.data()),
               samples.size() * sizeof(float));
}

void print_usage(const char *program) {
    std::cerr << "Usage: " << program << " [options] <text>\n"
              << "\nOptions:\n"
              << "  -m, --model <path>     Path to voice model (.onnx)\n"
              << "  -c, --config <path>    Path to voice config (.onnx.json)\n"
              << "  -d, --data <path>      Path to espeak-ng data directory\n"
              << "  -o, --output <path>    Output WAV file (default: output.wav)\n"
              << "  -s, --speed <float>    Speech speed (0.5=fast, 2.0=slow, default: 1.0)\n"
              << "  -h, --help             Show this help message\n"
              << "\nExample:\n"
              << "  " << program << " -m voice.onnx -d espeak-ng-data \"Hello world!\"\n";
}

int main(int argc, char *argv[]) {
    std::string model_path;
    std::string config_path;
    std::string espeak_data;
    std::string output_file = "output.wav";
    std::string text;
    float speed = 1.0f;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-m" || arg == "--model") && i + 1 < argc) {
            model_path = argv[++i];
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "-d" || arg == "--data") && i + 1 < argc) {
            espeak_data = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_file = argv[++i];
        } else if ((arg == "-s" || arg == "--speed") && i + 1 < argc) {
            speed = std::stof(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg[0] != '-') {
            text = arg;
        }
    }

    if (model_path.empty()) {
        std::cerr << "Error: Model path is required (-m)\n";
        print_usage(argv[0]);
        return 1;
    }

    if (espeak_data.empty()) {
        std::cerr << "Error: espeak-ng data path is required (-d)\n";
        print_usage(argv[0]);
        return 1;
    }

    if (text.empty()) {
        std::cerr << "Error: Text to synthesize is required\n";
        print_usage(argv[0]);
        return 1;
    }

    // Create synthesizer
    const char *cfg = config_path.empty() ? nullptr : config_path.c_str();
    piper_synthesizer *synth = piper_create(model_path.c_str(), cfg, espeak_data.c_str());

    if (!synth) {
        std::cerr << "Error: Failed to create synthesizer\n";
        return 1;
    }

    std::cout << "Synthesizing: \"" << text << "\"\n";

    // Set options
    piper_synthesize_options options = piper_default_synthesize_options(synth);
    options.length_scale = speed;

    // Start synthesis
    int result = piper_synthesize_start(synth, text.c_str(), &options);
    if (result != PIPER_OK) {
        std::cerr << "Error: Failed to start synthesis\n";
        piper_free(synth);
        return 1;
    }

    // Collect audio samples
    std::vector<float> all_samples;
    int sample_rate = 22050;
    piper_audio_chunk chunk;

    while ((result = piper_synthesize_next(synth, &chunk)) != PIPER_DONE) {
        if (result != PIPER_OK) {
            std::cerr << "Error: Synthesis failed\n";
            piper_free(synth);
            return 1;
        }
        sample_rate = chunk.sample_rate;
        all_samples.insert(all_samples.end(), chunk.samples,
                           chunk.samples + chunk.num_samples);
    }

    // Write WAV file
    write_wav(output_file, all_samples, sample_rate);

    std::cout << "Audio saved to: " << output_file << "\n";
    std::cout << "Duration: " << (float)all_samples.size() / sample_rate << " seconds\n";

    piper_free(synth);
    return 0;
}
