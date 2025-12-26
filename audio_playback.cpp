#include "audio_playback.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

AudioPlayback::AudioPlayback() {
    m_playing = false;
}

AudioPlayback::~AudioPlayback() {
    close();
}

bool AudioPlayback::init(int sample_rate) {
    m_sample_rate = sample_rate;

    SDL_AudioSpec spec_requested;
    SDL_AudioSpec spec_obtained;

    SDL_zero(spec_requested);
    SDL_zero(spec_obtained);

    spec_requested.freq = sample_rate;
    spec_requested.format = AUDIO_F32;
    spec_requested.channels = 1;
    spec_requested.samples = 1024;
    spec_requested.callback = audioCallback;
    spec_requested.userdata = this;

    m_dev_id_out = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &spec_requested, &spec_obtained, 0);

    if (!m_dev_id_out) {
        fprintf(stderr, "%s: couldn't open audio device for playback: %s\n", __func__, SDL_GetError());
        return false;
    }

    fprintf(stderr, "%s: opened playback device (SDL Id = %d):\n", __func__, m_dev_id_out);
    fprintf(stderr, "%s:     - sample rate: %d\n", __func__, spec_obtained.freq);
    fprintf(stderr, "%s:     - format:      %d\n", __func__, spec_obtained.format);
    fprintf(stderr, "%s:     - channels:    %d\n", __func__, spec_obtained.channels);
    fprintf(stderr, "%s:     - samples:     %d\n", __func__, spec_obtained.samples);

    // Start paused
    SDL_PauseAudioDevice(m_dev_id_out, 1);

    return true;
}

void AudioPlayback::close() {
    if (m_dev_id_out) {
        SDL_CloseAudioDevice(m_dev_id_out);
        m_dev_id_out = 0;
    }
}

void AudioPlayback::queue(const float* samples, size_t num_samples) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Append samples to buffer
    size_t old_size = m_buffer.size();
    m_buffer.resize(old_size + num_samples);
    memcpy(m_buffer.data() + old_size, samples, num_samples * sizeof(float));

    // Start playback if not already playing
    if (!m_playing && m_dev_id_out) {
        m_playing = true;
        SDL_PauseAudioDevice(m_dev_id_out, 0);
    }
}

void AudioPlayback::waitComplete() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this] {
        return m_read_pos >= m_buffer.size() || !m_playing;
    });

    // Pause and reset
    if (m_dev_id_out) {
        SDL_PauseAudioDevice(m_dev_id_out, 1);
    }
    m_buffer.clear();
    m_read_pos = 0;
    m_playing = false;
}

bool AudioPlayback::isPlaying() const {
    return m_playing;
}

void AudioPlayback::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_dev_id_out) {
        SDL_PauseAudioDevice(m_dev_id_out, 1);
    }
    m_buffer.clear();
    m_read_pos = 0;
    m_playing = false;
    m_cv.notify_all();
}

void AudioPlayback::audioCallback(void* userdata, uint8_t* stream, int len) {
    AudioPlayback* self = static_cast<AudioPlayback*>(userdata);
    self->fillBuffer(stream, len);
}

void AudioPlayback::fillBuffer(uint8_t* stream, int len) {
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t samples_requested = len / sizeof(float);
    size_t samples_available = m_buffer.size() - m_read_pos;
    size_t samples_to_copy = std::min(samples_requested, samples_available);

    if (samples_to_copy > 0) {
        memcpy(stream, m_buffer.data() + m_read_pos, samples_to_copy * sizeof(float));
        m_read_pos += samples_to_copy;
    }

    // Fill remaining with silence
    if (samples_to_copy < samples_requested) {
        memset(stream + samples_to_copy * sizeof(float), 0,
               (samples_requested - samples_to_copy) * sizeof(float));
    }

    // Notify if done
    if (m_read_pos >= m_buffer.size()) {
        m_cv.notify_all();
    }
}
