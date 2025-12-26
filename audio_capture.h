#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>

// Audio capture class for microphone input
// Adapted from whisper.cpp common-sdl
class AudioCapture {
public:
    AudioCapture(int len_ms);
    ~AudioCapture();

    bool init(int capture_id, int sample_rate);
    bool resume();
    bool pause();
    bool clear();

    // SDL callback
    void callback(uint8_t* stream, int len);

    // Get audio data from circular buffer
    void get(int ms, std::vector<float>& audio);

    int getSampleRate() const { return m_sample_rate; }

private:
    SDL_AudioDeviceID m_dev_id_in = 0;

    int m_len_ms = 0;
    int m_sample_rate = 0;

    std::atomic_bool m_running;
    std::mutex m_mutex;

    std::vector<float> m_audio;
    size_t m_audio_pos = 0;
    size_t m_audio_len = 0;
};

// Voice Activity Detection using energy-based threshold
// Adapted from whisper.cpp common.cpp
bool vad_simple(
    std::vector<float>& pcmf32,
    int sample_rate,
    int last_ms,
    float vad_thold,
    float freq_thold,
    bool verbose);

// High-pass filter for removing low-frequency noise
void high_pass_filter(
    std::vector<float>& data,
    float cutoff,
    float sample_rate);

// Poll SDL events (returns false if quit requested)
bool sdl_poll_events();
