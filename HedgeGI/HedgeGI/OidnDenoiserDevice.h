#pragma once

class Bitmap;

typedef struct OIDNDeviceImpl* OIDNDevice;

class OidnDenoiserDevice
{
public:
    static std::mutex mutex;
    static bool initialized;
    static OIDNDevice device;

    static std::unique_ptr<Bitmap> denoise(const Bitmap& bitmap, bool denoiseAlpha = false);
};
