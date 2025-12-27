# Patch espeak-ng speak_lib.h header for static linking on Windows
# This script modifies the ESPEAK_API macro to check for ESPEAK_STATIC

set(HEADER_FILE ${CMAKE_ARGV3})

if(NOT EXISTS ${HEADER_FILE})
    message(FATAL_ERROR "Header file not found: ${HEADER_FILE}")
endif()

file(READ ${HEADER_FILE} HEADER_CONTENT)

# Check if already patched
string(FIND "${HEADER_CONTENT}" "ESPEAK_STATIC" ALREADY_PATCHED)
if(NOT ALREADY_PATCHED EQUAL -1)
    message(STATUS "Header already patched for static linking")
    return()
endif()

# Replace the ESPEAK_API definition
# Original: #define ESPEAK_API __declspec(dllimport)
# New: Check for ESPEAK_STATIC first
string(REPLACE
    "#define ESPEAK_API __declspec(dllimport)"
    "#ifdef ESPEAK_STATIC
#define ESPEAK_API
#else
#define ESPEAK_API __declspec(dllimport)
#endif"
    HEADER_CONTENT "${HEADER_CONTENT}")

file(WRITE ${HEADER_FILE} "${HEADER_CONTENT}")
message(STATUS "Patched ${HEADER_FILE} for static linking")
