// This C++ metronome uses the miniaudio library to make clicking souunds at a set BPM.

// To build:    cmake -S . -B build -G Ninja; cmake --build build
// To run:      ./build/metronome

// Audio is processed in "frames": 1 frame = 1 sample per channel at a single time instant.
// With stereo (2 channels), each frame has 2 samples (L and R), interleaved in the output buffer.

// To-do:   1. Add adjustable BPM by prompting the user on the console. 
//          2. Add nicer click sound using a sine tone.

#include <iostream>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Called repeatedly by the playback device after it is initialized in main. 
// pOutput points to interleaved 32-bit float samples to be played by the device.
// frameCount represents number of frames requested this call. 
// Must write exactly frameCount * channels samples into pOutput every call.
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {

        float* out = (float*)pOutput;
        ma_uint32 channels = pDevice->playback.channels;

        static ma_uint64 globalFrame = 0;
        // globalFrame tracks how many frames have been played since the program started.
        // It increases by frameCount after each callback.

        static ma_uint64 nextClickFrame = 0;
        // nextClickFrame stores the frame number at which the next metronome click should start.

        const  ma_uint32 sampleRate = pDevice->sampleRate;
        const  ma_uint32 bpm = 120;
        const  ma_uint64 framesPerBeat = (ma_uint64)(sampleRate*60/bpm);
        const  ma_uint32 clickLenFrames = (ma_uint32)(sampleRate*0.003);

        ma_uint64 offset = 0;
        bool doClick = false;
        // doClick prevents click sound from playing continuously when not scheduled.

        if (nextClickFrame == 0) {
            nextClickFrame = framesPerBeat;
        }
        
        // Catch up if nextClickFrame falls behind (avoid missing clicks).
        while (nextClickFrame <= globalFrame) {
            nextClickFrame += framesPerBeat;
        }

        if (nextClickFrame < globalFrame + frameCount) {
            doClick = true;
            offset = nextClickFrame - globalFrame;
            nextClickFrame += framesPerBeat;
        }

        for (ma_uint32 frame = 0; frame < frameCount; ++frame) {
            for (ma_uint32 channel = 0; channel < channels; ++channel) {
                if (doClick && (frame >= offset && frame < offset + clickLenFrames)) {
                    out[frame * channels + channel] = 0.25f*(1.0f - (float)(frame - offset)/(float)clickLenFrames);
                } else {
                    out[frame * channels + channel] = 0.0f;
                }
            }
        }
        globalFrame += frameCount;
}

int main() {

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;       // Use 32-bit float samples.
    config.playback.channels = 2;                   // Use 2 channels (stereo) for the device.
    config.sampleRate        = 48000;               // Set sample rate to 48000 Hz.
    config.dataCallback      = data_callback;       // This function will be called when miniaudio needs more data.
    config.pUserData         = nullptr;             // Can be accessed from the device object (device.pUserData).

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    ma_device_start(&device);   // Start the device manually as it is sleeping by default.
    
    std::cout << "Device started" << std::endl;

    std::cout << "Press Enter to stop...";
    std::cin.get();     // Keep the program running until 'Enter' is pressed.
    
    ma_device_uninit(&device);
    return 0;
}