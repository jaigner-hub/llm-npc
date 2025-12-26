cmake_minimum_required(VERSION 3.14)
project(tts CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths to libpiper installation
set(PIPER_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/piper1-gpl/libpiper/install")
set(PIPER_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/piper1-gpl/libpiper/build")

# Paths to whisper.cpp
set(WHISPER_DIR "C:/msys64/whisper.cpp")

# Find required packages
find_package(SDL2 REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(${PIPER_INSTALL_DIR}/include)
include_directories(${WHISPER_DIR}/include)
include_directories(${WHISPER_DIR}/ggml/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Find piper and onnxruntime libraries
if(WIN32)
    set(PIPER_LIB "${PIPER_INSTALL_DIR}/libpiper.dll.a")
    set(ONNXRUNTIME_LIB "${PIPER_INSTALL_DIR}/lib/onnxruntime.lib")
else()
    set(PIPER_LIB "${PIPER_INSTALL_DIR}/libpiper.so")
    set(ONNXRUNTIME_LIB "${PIPER_INSTALL_DIR}/lib/libonnxruntime.so")
endif()
set(ESPEAK_LIB "${PIPER_BUILD_DIR}/espeak_ng-install/lib/libespeak-ng.a")

# Whisper libraries (static .a files)
set(WHISPER_LIB "${WHISPER_DIR}/build/src/libwhisper.a")
set(GGML_LIB "${WHISPER_DIR}/build/ggml/src/ggml.a")
set(GGML_BASE_LIB "${WHISPER_DIR}/build/ggml/src/ggml-base.a")
set(GGML_CPU_LIB "${WHISPER_DIR}/build/ggml/src/ggml-cpu.a")

# ============================================
# TTS executable (original text-to-speech tool)
# ============================================
add_executable(tts tts.cpp)
target_link_libraries(tts
    ${PIPER_LIB}
    ${ONNXRUNTIME_LIB}
    ${ESPEAK_LIB}
)

# ============================================
# Voice Chat executable (integrated STT + LLM + TTS)
# ============================================
add_executable(voice_chat
    voice_chat.cpp
    audio_capture.cpp
    audio_playback.cpp
    npc_chat.cpp
)

target_include_directories(voice_chat PRIVATE
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(voice_chat
    # Piper TTS
    ${PIPER_LIB}
    ${ONNXRUNTIME_LIB}
    ${ESPEAK_LIB}
    # Whisper STT (order matters for static libs)
    ${WHISPER_LIB}
    ${GGML_LIB}
    ${GGML_BASE_LIB}
    ${GGML_CPU_LIB}
    # OpenMP (required by whisper ggml-cpu)
    -fopenmp
    # Audio I/O
    ${SDL2_LIBRARIES}
    # API calls
    CURL::libcurl
)
