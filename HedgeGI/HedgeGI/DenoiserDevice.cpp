#include "DenoiserDevice.h"
#include "Bitmap.h"

// cuda
#include <cuda.h>
#include <cuda_runtime.h>

// optix
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

std::mutex DenoiserDevice::mutex;
bool DenoiserDevice::initialized;
OptixDeviceContext DenoiserDevice::context;
OptixDenoiser DenoiserDevice::denoiser;

std::unique_ptr<Bitmap> DenoiserDevice::denoise(const Bitmap& bitmap)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!initialized)
    {
        cudaFree(nullptr);

        CUcontext cuCtx = nullptr;
        optixInit();
        optixDeviceContextCreate(cuCtx, nullptr, &context);

        const OptixDenoiserOptions options = { OPTIX_DENOISER_INPUT_RGB };
        optixDenoiserCreate(context, &options, &denoiser);
        optixDenoiserSetModel(denoiser, OPTIX_DENOISER_MODEL_KIND_HDR, nullptr, 0);

        std::atexit([]() { optixDenoiserDestroy(denoiser); });
        std::atexit([]() { optixDeviceContextDestroy(context); });
        std::atexit([]() { cudaFree(nullptr); });

        initialized = true;
    }

    std::unique_ptr<Bitmap> denoised = std::make_unique<Bitmap>(bitmap.width, bitmap.height, bitmap.arraySize);

    OptixDenoiserSizes returnSizes;
    optixDenoiserComputeMemoryResources(denoiser, bitmap.width, bitmap.height, &returnSizes);

    CUdeviceptr intensity = 0;
    CUdeviceptr scratch = 0;
    CUdeviceptr state = 0;

    cudaMalloc((void**)&intensity, sizeof(float));
    cudaMalloc((void**)&scratch, returnSizes.withoutOverlapScratchSizeInBytes);
    cudaMalloc((void**)&state, returnSizes.stateSizeInBytes);

    const OptixDenoiserParams params = { false, intensity, 0.0f, 0 };

    OptixImage2D srcImg = { 0, bitmap.width, bitmap.height, bitmap.width * sizeof(Eigen::Array4f), sizeof(Eigen::Array4f), OPTIX_PIXEL_FORMAT_FLOAT4 };
    OptixImage2D dstImg = srcImg;

    const size_t dataSize = bitmap.width * bitmap.height * sizeof(Eigen::Array4f);

    cudaMalloc((void**)&srcImg.data, dataSize);
    cudaMalloc((void**)&dstImg.data, dataSize);

    optixDenoiserSetup(denoiser, nullptr, bitmap.width, bitmap.height, state, returnSizes.stateSizeInBytes, scratch, returnSizes.withoutOverlapScratchSizeInBytes);

    for (size_t i = 0; i < bitmap.arraySize; i++)
    {
        cudaMemcpy((void*)srcImg.data, bitmap.getColors(i), dataSize, cudaMemcpyHostToDevice);

        optixDenoiserComputeIntensity(denoiser, nullptr, &srcImg, intensity, scratch, returnSizes.withoutOverlapScratchSizeInBytes);
        optixDenoiserInvoke(denoiser, nullptr, &params, state, returnSizes.stateSizeInBytes, &srcImg, 1, 0, 0, &dstImg, scratch, returnSizes.withoutOverlapScratchSizeInBytes);

        cudaMemcpy(denoised->getColors(i), (void*)dstImg.data, dataSize, cudaMemcpyDeviceToHost);
    }

    cudaFree((void*)intensity);
    cudaFree((void*)scratch);
    cudaFree((void*)state);
    cudaFree((void*)srcImg.data);
    cudaFree((void*)dstImg.data);

    return denoised;
}
