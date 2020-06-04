#pragma once

class RaytracingDevice
{
    static RTCDevice rtcDevice;

public:
    static RTCDevice get();
};
