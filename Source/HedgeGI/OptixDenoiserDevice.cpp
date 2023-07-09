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
OptixDeviceContext OptixDenoiserDevice::context;
OptixDenoiser OptixDenoiserDevice::denoiser;

const bool OptixDenoiserDevice::available = []() 
{
    int count;

    if (cudaGetDeviceCount(&count) != cudaSuccess || count < 1)
        return false;

    cudaFree(nullptr);

    CUcontext cuCtx = nullptr;
    if (optixInit() != OPTIX_SUCCESS)
        return false;

    if (optixDeviceContextCreate(cuCtx, nullptr, &context) != OPTIX_SUCCESS)
        return false;

    const OptixDenoiserOptions options = {0, 0};
    optixDenoiserCreate(context, OPTIX_DENOISER_MODEL_KIND_HDR, &options, &denoiser);

    std::atexit([] { if (denoiser != nullptr) optixDenoiserDestroy(denoiser); });
    std::atexit([] { if (context != nullptr) optixDeviceContextDestroy(context); });
    std::atexit([] { cudaFree(nullptr); });

    return true;
}();

std::unique_ptr<Bitmap> OptixDenoiserDevice::denoise(const Bitmap& bitmap, const bool denoiseAlpha)
{
    std::lock_guard lock(criticalSection);

    std::unique_ptr<Bitmap> denoised = std::make_unique<Bitmap>(bitmap, false);

    OptixDenoiserSizes returnSizes;
    optixDenoiserComputeMemoryResources(denoiser, (unsigned)bitmap.width, (unsigned)bitmap.height, &returnSizes);

    CUdeviceptr intensity = 0;
    CUdeviceptr scratch = 0;
    CUdeviceptr state = 0;

    cudaMalloc((void**)&intensity, sizeof(float));
    cudaMalloc((void**)&scratch, returnSizes.withoutOverlapScratchSizeInBytes);
    cudaMalloc((void**)&state, returnSizes.stateSizeInBytes);

    const OptixDenoiserParams params = { (OptixDenoiserAlphaMode) denoiseAlpha, intensity, 0.0f, 0 };

    const OptixImage2D imgTmp = { 0, (unsigned)bitmap.width, (unsigned)bitmap.height, (unsigned)(bitmap.width * sizeof(Color4)), sizeof(Color4), OPTIX_PIXEL_FORMAT_FLOAT4 };

    OptixDenoiserLayer layer = { imgTmp, {}, imgTmp };
    const OptixDenoiserGuideLayer guideLayer = {};

    const size_t dataSize = bitmap.width * bitmap.height * sizeof(Color4);

    cudaMalloc((void**)&layer.input.data, dataSize);
    cudaMalloc((void**)&layer.output.data, dataSize);

    optixDenoiserSetup(denoiser, nullptr, (unsigned)bitmap.width, (unsigned)bitmap.height, state, returnSizes.stateSizeInBytes, scratch, returnSizes.withoutOverlapScratchSizeInBytes);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        cudaMemcpy((void*)layer.input.data, bitmap.getColorPtr(bitmap.width * bitmap.height * i), dataSize, cudaMemcpyHostToDevice);

        optixDenoiserComputeIntensity(denoiser, nullptr, &layer.input, intensity, scratch, returnSizes.withoutOverlapScratchSizeInBytes);
        optixDenoiserInvoke(denoiser, nullptr, &params, state, returnSizes.stateSizeInBytes, &guideLayer, &layer, 1, 0, 0, scratch, returnSizes.withoutOverlapScratchSizeInBytes);

        cudaMemcpy(denoised->getColorPtr(denoised->width * denoised->height * i), (void*)layer.output.data, dataSize, cudaMemcpyDeviceToHost);
    }

    cudaFree((void*)intensity);
    cudaFree((void*)scratch);
    cudaFree((void*)state);
    cudaFree((void*)layer.input.data);
    cudaFree((void*)layer.output.data);

    return denoised;
}
