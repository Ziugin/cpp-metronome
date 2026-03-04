// This C++ metronome uses the miniaudio library to make clicking souunds at a set BPM.

// To build:    cmake -S . -B build -G Ninja; cmake --build build
// To run:      ./build/metronome

// Audio is processed in "frames": 1 frame = 1 sample per channel at a single time instant.
// With stereo (2 channels), each frame has 2 samples (L and R), interleaved in the output buffer.

#include <iostream>
#include <cmath>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct Metronome {
    std::atomic<int> bpm = 90;
    std::atomic<bool> changeBpm = false;
    // Both variables are shared between the main input thread
    // and the audio callback thread.
};

// Called repeatedly by the playback device after it is initialized in main. 
// pOutput points to interleaved 32-bit float samples to be played by the device.
// frameCount represents number of frames requested this call. 
// Must write exactly frameCount * channels samples into pOutput every call.
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        
        Metronome* metronome = static_cast<Metronome*>(pDevice->pUserData);
        
        float* out = (float*)pOutput;
        ma_uint32 channels = pDevice->playback.channels;

        static ma_uint64 globalFrame = 0;
        // globalFrame tracks how many frames have been played since the program started.
        // It increases by frameCount after each callback.

        static ma_uint64 nextClickFrame = 0;
        // nextClickFrame stores the frame number at which the next metronome click should start.

        static ma_uint64 clickFrame = 0;
        // Tracks how many frames into the current click have been rendered across multiple callbacks.

        const  ma_uint32 sampleRate = pDevice->sampleRate;
        const  ma_uint64 framesPerBeat = (ma_uint64)(sampleRate*60/metronome->bpm.load());
        const  ma_uint32 clickLenFrames = (ma_uint32)(sampleRate*0.15);

        ma_uint64 offset = 0;
        bool doClick = false;
        // doClick prevents click sound from playing continuously when not scheduled.

        if (nextClickFrame == 0) {
            nextClickFrame = framesPerBeat;
        }

        // Reset and delay the next click by 0.5 seconds when the BPM is adjusted.
        if (metronome->changeBpm.load()) {
            nextClickFrame = globalFrame + 24000;
            metronome->changeBpm = false;
        }
        
        // Catch up if nextClickFrame falls behind (avoid missing clicks).
        while (nextClickFrame < globalFrame) {
            nextClickFrame += framesPerBeat;
        }
        
        // Schedule the next click.
        if (nextClickFrame < globalFrame + frameCount) {
            doClick = true;
            offset = nextClickFrame - globalFrame;
            clickFrame = 0;
            nextClickFrame += framesPerBeat;
        }
        
        // Write the sine tone to the device's audio channels if a click is scheduled and silence otherwise.
        for (ma_uint32 frame = 0; frame < frameCount; ++frame) {
            for (ma_uint32 channel = 0; channel < channels; ++channel) {
                if (clickFrame < clickLenFrames && frame >= (doClick ? offset : 0)) {
                    float t = (float)clickFrame / (float)sampleRate;                                        // Time elapsed since the click started.
                    float envelope = 1.0f - (float)clickFrame / (float)clickLenFrames;                      // Fade out for each click.
                    out[frame * channels + channel] = 0.25f * sin(2.0f * M_PI * 880.0f * t) * envelope;     // 880Hz sine tone.
                } else {
                    out[frame * channels + channel] = 0.0f;
                }
            }
            if (clickFrame < clickLenFrames && frame >= (doClick ? offset : 0)) {
                clickFrame++;
            }
        }
        globalFrame += frameCount;
}

int main() {

    Metronome metronome;
    
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;       // Use 32-bit float samples.
    config.playback.channels = 2;                   // Use 2 channels (stereo) for the device.
    config.sampleRate        = 48000;               // Set sample rate to 48000 Hz.
    config.dataCallback      = data_callback;       // This function will be called when miniaudio needs more data.
    config.pUserData         = &metronome;          // Can be accessed from the device object (device.pUserData).

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    ma_device_start(&device);     // Start the device manually as it is sleeping by default.
    
    std::cout << "Metronome started" << std::endl;
    std::cout << "Enter a positive integer between 1 and 300 to change the BPM. Enter 0 to stop the metronome" << std::endl;

    // User input loop to adjust BPM.
    int inputBpm = 90;  
    while (inputBpm != 0) {
        std::cout << "BPM: ";
        std::cin >> inputBpm;
        if (inputBpm < 0 || inputBpm > 300) {
            std::cout << "BPM must be between 1 and 300" << std::endl;
        } else {
            metronome.changeBpm = true;
            metronome.bpm = inputBpm;
        }
    }
    
    ma_device_uninit(&device);
    std::cout << "Metronome stopped" << std::endl;

    return 0;
}