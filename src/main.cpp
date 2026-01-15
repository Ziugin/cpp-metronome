#include <iostream>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) 
{
        // In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
        // pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
        // frameCount frames.
        return;
}

int main() {

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_unknown;   // Use the device's native format.
    config.playback.channels = 0;                   // Use the device's native channel count.
    config.sampleRate        = 48000;               // Set sample rate to 48000 Hz.
    config.dataCallback      = data_callback;       // This function will be called when miniaudio needs more data.
    config.pUserData         = nullptr;             // Can be accessed from the device object (device.pUserData).

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    ma_device_start(&device);   // Start the device manually as it is sleeping by default.

    std::cout << "Press Enter to stop...";
    int x;
    std::cin >> x;  // Keep the program running until Enter is pressed.
    
    ma_device_uninit(&device);
    return 0;
}