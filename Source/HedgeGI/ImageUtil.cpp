#include "ImageUtil.h"

DirectX::ScratchImage ImageUtil::load(const int rc)
{
    const HRSRC hResInfo = FindResource(nullptr, MAKEINTRESOURCE(rc), TEXT("TEXT"));
    const HGLOBAL hResData = LoadResource(nullptr, hResInfo);

    DirectX::ScratchImage image;
    DirectX::LoadFromWICMemory(LockResource(hResData), SizeofResource(nullptr, hResInfo), DirectX::WIC_FLAGS_NONE, nullptr, image);

    if (image.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        DirectX::ScratchImage tmpImage;
        DirectX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
            DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, tmpImage);

        std::swap(image, tmpImage);
    }

    FreeResource(hResData);

    return image;
}
