#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

// Audio playback class for speaker output
class AudioPlayback {
public:
    AudioPlayback();
    ~AudioPlayback();

    bool init(int sample_rate);
    void close();

    // Queue audio samples for playback
    void queue(const float* samples, size_t num_samples);

    // Wait for all queued audio to finish playing
    void waitComplete();

    // Check if audio is currently playing
    bool isPlaying() const;

    // Clear any queued audio
    void clear();

private:
    // SDL callback
    static void audioCallback(void* userdata, uint8_t* stream, int len);
    void fillBuffer(uint8_t* stream, int len);

    SDL_AudioDeviceID m_dev_id_out = 0;
    int m_sample_rate = 0;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<float> m_buffer;
    size_t m_read_pos = 0;
    std::atomic_bool m_playing;
};
