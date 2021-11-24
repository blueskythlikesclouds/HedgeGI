#include "OptixDenoiserDevice.h"
#include "Bitmap.h"

// cuda
#include <cuda.h>
#include <cuda_runtime.h>

// optix
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

CriticalSection OptixDenoiserDevice::criticalSection;
bool OptixDenoiserDevice::initialized;
OptixDeviceContext OptixDenoiserDevice::context;
OptixDenoiser OptixDenoiserDevice::denoiser;

std::unique_ptr<Bitmap> OptixDenoiserDevice::denoise(const Bitmap& bitmap, const bool denoiseAlpha)
{
    std::lock_guard<CriticalSection> lock(criticalSection);

    if (!initialized)
    {
        cudaFree(nullptr);

        CUcontext cuCtx = nullptr;
        optixInit();
        optixDeviceContextCreate(cuCtx, nullptr, &context);

        const OptixDenoiserOptions options = { 0, 0 };
        optixDenoiserCreate(context, OPTIX_DENOISER_MODEL_KIND_HDR, &options, &denoiser);

        std::atexit([]() { optixDenoiserDestroy(denoiser); });
        std::atexit([]() { optixDeviceContextDestroy(context); });
        std::atexit([]() { cudaFree(nullptr); });

        initialized = true;
    }

    std::unique_ptr<Bitmap> denoised = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize, bitmap.type);

    OptixDenoiserSizes returnSizes;
    optixDenoiserComputeMemoryResources(denoiser, bitmap.width, bitmap.height, &returnSizes);

    CUdeviceptr intensity = 0;
    CUdeviceptr scratch = 0;
    CUdeviceptr state = 0;

    cudaMalloc((void**)&intensity, sizeof(float));
    cudaMalloc((void**)&scratch, returnSizes.withoutOverlapScratchSizeInBytes);
    cudaMalloc((void**)&state, returnSizes.stateSizeInBytes);

    const OptixDenoiserParams params = { denoiseAlpha, intensity, 0.0f, 0 };

    const OptixImage2D imgTmp = { 0, bitmap.width, bitmap.height, bitmap.width * sizeof(Color4), sizeof(Color4), OPTIX_PIXEL_FORMAT_FLOAT4 };

    OptixDenoiserLayer layer = { imgTmp, {}, imgTmp };
    const OptixDenoiserGuideLayer guideLayer = {};

    const size_t dataSize = bitmap.width * bitmap.height * sizeof(Color4);

    cudaMalloc((void**)&layer.input.data, dataSize);
    cudaMalloc((void**)&layer.output.data, dataSize);

    optixDenoiserSetup(denoiser, nullptr, bitmap.width, bitmap.height, state, returnSizes.stateSizeInBytes, scratch, returnSizes.withoutOverlapScratchSizeInBytes);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        cudaMemcpy((void*)layer.input.data, bitmap.getColors(i), dataSize, cudaMemcpyHostToDevice);

        optixDenoiserComputeIntensity(denoiser, nullptr, &layer.input, intensity, scratch, returnSizes.withoutOverlapScratchSizeInBytes);
        optixDenoiserInvoke(denoiser, nullptr, &params, state, returnSizes.stateSizeInBytes, &guideLayer, &layer, 1, 0, 0, scratch, returnSizes.withoutOverlapScratchSizeInBytes);

        cudaMemcpy(denoised->getColors(i), (void*)layer.output.data, dataSize, cudaMemcpyDeviceToHost);
    }

    cudaFree((void*)intensity);
    cudaFree((void*)scratch);
    cudaFree((void*)state);
    cudaFree((void*)layer.input.data);
    cudaFree((void*)layer.output.data);

    return denoised;
}
