#include "RaytracingDevice.h"

RTCDevice RaytracingDevice::rtcDevice {};

RTCDevice RaytracingDevice::get()
{
    if (rtcDevice)
        return rtcDevice;

    rtcDevice = rtcNewDevice(nullptr);
    std::atexit([]() { rtcReleaseDevice(rtcDevice); });

    return rtcDevice;
}
