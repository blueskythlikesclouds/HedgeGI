#include "OidnDenoiserDevice.h"

#include "Bitmap.h"
#include <OpenImageDenoise/oidn.h>

std::mutex OidnDenoiserDevice::mutex;
bool OidnDenoiserDevice::initialized;
OIDNDevice OidnDenoiserDevice::device;

std::unique_ptr<Bitmap> OidnDenoiserDevice::denoise(const Bitmap& bitmap, bool denoiseAlpha)
{
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!initialized)
        {
            device = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
            oidnCommitDevice(device);

            std::atexit([]() { oidnReleaseDevice(device); });

            initialized = true;
        }
    }

    std::unique_ptr<Bitmap> denoised = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        OIDNFilter filter = oidnNewFilter(device, "RT");
        oidnSetSharedFilterImage(filter, "color", bitmap.getColors(i), OIDN_FORMAT_FLOAT3, bitmap.width, bitmap.height, 0, sizeof(Color4), 0);
        oidnSetSharedFilterImage(filter, "output", denoised->getColors(i), OIDN_FORMAT_FLOAT3, bitmap.width, bitmap.height, 0, sizeof(Color4), 0);
        oidnSetFilter1b(filter, "hdr", true);
        oidnCommitFilter(filter);
        oidnExecuteFilter(filter);

        const char* errorMessage;
        if (oidnGetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE)
            printf("OIDN Error: %s\n", errorMessage);

        oidnReleaseFilter(filter);
    }

    // TODO: Denoise alpha
    for (size_t i = 0; i < bitmap.width * bitmap.height * bitmap.arraySize; i++)
        denoised->data[i].w() = bitmap.data[i].w();

    return denoised;
}
