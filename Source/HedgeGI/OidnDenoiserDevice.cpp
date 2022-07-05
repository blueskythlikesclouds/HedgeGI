#include "OidnDenoiserDevice.h"

#if defined(ENABLE_OIDN)
#include "Bitmap.h"
#include "Logger.h"
#include <OpenImageDenoise/oidn.h>

CriticalSection OidnDenoiserDevice::criticalSection;
bool OidnDenoiserDevice::initialized;
OIDNDevice OidnDenoiserDevice::device;
const bool OidnDenoiserDevice::available = true;

std::unique_ptr<Bitmap> OidnDenoiserDevice::denoise(const Bitmap& bitmap, bool denoiseAlpha)
{
    {
        std::lock_guard<CriticalSection> lock(criticalSection);

        if (!initialized)
        {
            device = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
            oidnCommitDevice(device);

            std::atexit([]() { oidnReleaseDevice(device); });

            initialized = true;
        }
    }

    std::unique_ptr<Bitmap> denoised = std::make_unique<Bitmap>(bitmap, false);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        OIDNFilter filter = oidnNewFilter(device, "RTLightmap");
        oidnSetSharedFilterImage(filter, "color", bitmap.getColorPtr(bitmap.width * bitmap.height * i), OIDN_FORMAT_FLOAT3, bitmap.width, bitmap.height, 0, sizeof(Color4), 0);
        oidnSetSharedFilterImage(filter, "output", denoised->getColorPtr(denoised->width * denoised->height * i), OIDN_FORMAT_FLOAT3, denoised->width, denoised->height, 0, sizeof(Color4), 0);
        oidnSetFilter1b(filter, "hdr", true);
        oidnCommitFilter(filter);
        oidnExecuteFilter(filter);

        const char* errorMessage;
        if (oidnGetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE)
            Logger::logFormatted(LogType::Error, "OIDN Error: %s\n", errorMessage);

        oidnReleaseFilter(filter);
    }

    // TODO: Denoise alpha
    for (size_t i = 0; i < bitmap.width * bitmap.height * bitmap.arraySize; i++)
        denoised->setAlpha(bitmap.getAlpha(i), i);

    return denoised;
}

#else
const bool OidnDenoiserDevice::available = false;
#endif