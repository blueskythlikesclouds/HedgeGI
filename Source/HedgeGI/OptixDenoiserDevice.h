#pragma once

class Bitmap;

typedef struct OptixDeviceContext_t* OptixDeviceContext;
typedef struct OptixDenoiser_t* OptixDenoiser;

class OptixDenoiserDevice
{
    static CriticalSection criticalSection;
    static OptixDeviceContext context;
    static OptixDenoiser denoiser;

public:
    static const bool available;

    static std::unique_ptr<Bitmap> denoise(const Bitmap& bitmap, bool denoiseAlpha = false);
};
