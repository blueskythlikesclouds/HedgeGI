#include "PostRender.h"

#include "BakeParams.h"
#include "CabinetCompression.h"
#include "D3D11Device.h"
#include "Logger.h"
#include "Utilities.h"

class PostRender::TextureNode
{
public:
    size_t x{};
    size_t y{};
    size_t width{};
    size_t height{};
    Texture* texture{};
    std::unique_ptr<TextureNode> left;
    std::unique_ptr<TextureNode> right;

    TextureNode* insert(Texture* texToInsert)
    {
        if (left && right)
        {
            TextureNode* node = left->insert(texToInsert);
            return node != nullptr ? node : right->insert(texToInsert);
        }

        if (texture != nullptr || texToInsert->width > width || texToInsert->height > height)
            return nullptr;

        if (texToInsert->width == width && texToInsert->height == height)
        {
            texture = texToInsert;
            return this;
        }

        left = std::make_unique<TextureNode>();
        right = std::make_unique<TextureNode>();

        if (width - texToInsert->width > height - texToInsert->height)
        {
            left->x = x;
            left->y = y;
            left->width = texToInsert->width;
            left->height = height;

            right->x = x + texToInsert->width;
            right->y = y;
            right->width = width - texToInsert->width;
            right->height = height;
        }
        else
        {
            left->x = x;
            left->y = y;
            left->width = width;
            left->height = texToInsert->height;

            right->x = x;
            right->y = y + texToInsert->height;
            right->width = width;
            right->height = height - texToInsert->height;
        }

        return left->insert(texToInsert);
    }

    size_t getValidPixelCount() const
    {
        if (texture) 
            return texture->width * texture->height;

        if (left && right)
            return left->getValidPixelCount() + right->getValidPixelCount();

        return 0;
    }

    void updateTextures() const
    {
        if (texture)
        {
            texture->x = x;
            texture->y = y;
        }
        else if (left && right)
        {
            left->updateTextures();
            right->updateTextures();
        }
    }

    const TextureNode* find(Texture* texToFind) const
    {
        if (texture != nullptr)
            return texture == texToFind ? this : nullptr;

        if (!left || !right)
            return nullptr;

        const TextureNode* node = left->find(texToFind);
        return node != nullptr ? node : right->find(texToFind);
    }
};

void PostRender::processInstancesRecursively(std::function<void(const std::string&)> function,
    hl::hh::mirage::raw_gi_texture_group* group, hl::hh::mirage::raw_gi_texture_group_info_v2* groupInfo)
{
    if (group->level == 0)
    {
        for (auto index : group->indices)
            function(groupInfo->instanceNames[index].get());
    }
    else
    {
        for (auto index : group->indices)
            processInstancesRecursively(function, groupInfo->groups[index].get(), groupInfo);
    }
}

void PostRender::createAtlasesRecursively(std::list<Texture>& textures, std::vector<Atlas>& atlases)
{
    if (textures.empty()) return;

    std::stable_sort(textures.begin(), textures.end(), [](const Texture& lhs, const Texture& rhs)
    {
        return lhs.width * lhs.height > rhs.width * rhs.height;
    });

    size_t maxAtlasSize = 2048;
    size_t currentWidth = maxAtlasSize;
    size_t currentHeight = maxAtlasSize;

    for (auto& texture : textures)
    {
        maxAtlasSize = std::max(texture.width, std::max(texture.height, maxAtlasSize));
        currentWidth = std::min(currentWidth, texture.width);
        currentHeight = std::min(currentHeight, texture.height);
    }
    
    TextureNode atlasNode;

    while (true)
    {
        TextureNode newAtlasNode;
        newAtlasNode.width = currentWidth;
        newAtlasNode.height = currentHeight;

        bool allInserted = true;

        for (auto& texture : textures)
            allInserted &= newAtlasNode.insert(&texture) != nullptr;

        if (allInserted)
        {
            const size_t totalPixelSize = atlasNode.width * atlasNode.height;
            const size_t newTotalPixelSize = newAtlasNode.width * newAtlasNode.height;

            const double validPixelRatio = (double)atlasNode.getValidPixelCount() / (double)std::max<size_t>(1, totalPixelSize);
            const double newValidPixelRatio = (double)newAtlasNode.getValidPixelCount() / (double)std::max<size_t>(1, newTotalPixelSize);

            // Pick this atlas resolution if it's more efficient
            if (newValidPixelRatio == 1.0 || newValidPixelRatio > validPixelRatio)
                atlasNode = std::move(newAtlasNode);

            // We don't need to try above resolutions since we're guaranteed to waste space.
            break;
        }

        atlasNode = std::move(newAtlasNode);

        if (currentWidth >= maxAtlasSize && currentHeight >= maxAtlasSize)
            break;

        if (currentWidth < currentHeight)
            currentWidth *= 2;
        else
            currentHeight *= 2;
    }

    atlasNode.updateTextures();

    Atlas atlas;
    atlas.width = atlasNode.width;
    atlas.height = atlasNode.height;

    for (auto it = textures.begin(); it != textures.end();)
    {
        // Skip this texture if it's not in the atlas
        if (!atlasNode.find(&(*it)))
        {
            ++it;
            continue;
        }

        atlas.textures.push_back(std::move(*it));
        it = textures.erase(it);
    }

    atlases.push_back(std::move(atlas));

    // Process remaining textures
    createAtlasesRecursively(textures, atlases);
}

hl::archive PostRender::createArchive(const std::string& inputDirectoryPath, TargetEngine targetEngine,
    hl::hh::mirage::raw_gi_texture_group* group, hl::hh::mirage::raw_gi_texture_group_info_v2* groupInfo)
{
    const std::string levelSuffix = "-level" + std::to_string(group->level);

    std::unordered_map<std::string, std::unique_ptr<DirectX::ScratchImage>> images;

    std::list<Texture> giTextures;
    std::list<Texture> sgTextures;
    std::list<Texture> occlusionTextures;

    processInstancesRecursively([&](const std::string& name)
    {
        if (targetEngine == TargetEngine::HE2)
        {
            std::string lightMapFilePath = inputDirectoryPath + "/" + name + "_sg.dds";
            const std::string shadowMapFilePath = inputDirectoryPath + "/" + name + "_occlusion.dds";

            if (!std::filesystem::exists(lightMapFilePath))
                lightMapFilePath = inputDirectoryPath + "/" + name + ".dds";

            if (std::filesystem::exists(lightMapFilePath) && std::filesystem::exists(shadowMapFilePath))
            {
                std::unique_ptr<DirectX::ScratchImage> lightMapImage = std::make_unique<DirectX::ScratchImage>();
                std::unique_ptr<DirectX::ScratchImage> shadowMapImage = std::make_unique<DirectX::ScratchImage>();

                DirectX::TexMetadata lightMapMetadata;
                DirectX::TexMetadata shadowMapMetadata;

                {
                    wchar_t filePathWideChar[1024];
                    MultiByteToWideChar(CP_UTF8, 0, lightMapFilePath.c_str(), -1, filePathWideChar, 1024);
                    DirectX::LoadFromDDSFile(filePathWideChar, DirectX::DDS_FLAGS_NONE, &lightMapMetadata, *lightMapImage);
                }

                if (DirectX::IsCompressed(lightMapMetadata.format))
                {
                    std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                    DirectX::Decompress(lightMapImage->GetImages(), lightMapImage->GetImageCount(), lightMapImage->GetMetadata(), DXGI_FORMAT_R16G16B16A16_FLOAT, *tmpImage);
                    lightMapImage.swap(tmpImage);
                }
                else if (lightMapMetadata.format != DXGI_FORMAT_R16G16B16A16_FLOAT)
                {
                    std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                    DirectX::Convert(lightMapImage->GetImages(), lightMapImage->GetImageCount(), lightMapImage->GetMetadata(), DXGI_FORMAT_R16G16B16A16_FLOAT, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *tmpImage);
                    lightMapImage.swap(tmpImage);
                }

                {
                    wchar_t filePathWideChar[1024];
                    MultiByteToWideChar(CP_UTF8, 0, shadowMapFilePath.c_str(), -1, filePathWideChar, 1024);
                    DirectX::LoadFromDDSFile(filePathWideChar, DirectX::DDS_FLAGS_NONE, &shadowMapMetadata, *shadowMapImage);
                }

                if (DirectX::IsCompressed(shadowMapMetadata.format))
                {
                    std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                    DirectX::Decompress(shadowMapImage->GetImages(), shadowMapImage->GetImageCount(), shadowMapImage->GetMetadata(), DXGI_FORMAT_R8_UNORM, *tmpImage);
                    shadowMapImage.swap(tmpImage);
                }
                else if (shadowMapMetadata.format != DXGI_FORMAT_R8_UNORM)
                {
                    std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                    DirectX::Convert(shadowMapImage->GetImages(), shadowMapImage->GetImageCount(), shadowMapImage->GetMetadata(), DXGI_FORMAT_R8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *tmpImage);
                    shadowMapImage.swap(tmpImage);
                }

                const size_t width = std::max((size_t)4, std::max(lightMapMetadata.width, shadowMapMetadata.width) >> group->level);
                const size_t height = std::max((size_t)4, std::max(lightMapMetadata.height, shadowMapMetadata.height) >> group->level);

                if (lightMapMetadata.width != width || lightMapMetadata.height != height)
                {
                    std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                    DirectX::Resize(lightMapImage->GetImages(), lightMapImage->GetImageCount(), lightMapImage->GetMetadata(), width, height, DirectX::TEX_FILTER_BOX, *tmpImage);
                    lightMapImage.swap(tmpImage);
                }

                if (shadowMapMetadata.width != width || shadowMapMetadata.height != height)
                {
                    std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                    DirectX::Resize(shadowMapImage->GetImages(), shadowMapImage->GetImageCount(), shadowMapImage->GetMetadata(), width, height, DirectX::TEX_FILTER_BOX, *tmpImage);
                    shadowMapImage.swap(tmpImage);
                }

                lightMapMetadata = lightMapImage->GetMetadata();
                shadowMapMetadata = shadowMapImage->GetMetadata();

                const bool isSg = lightMapMetadata.arraySize == 4;

                const std::string lightMapName = isSg ? name + "_sg" + levelSuffix : name + levelSuffix;
                const std::string shadowMapName = name + "_occlusion" + levelSuffix;

                images[lightMapName] = std::move(lightMapImage);
                images[shadowMapName] = std::move(shadowMapImage);

                if (isSg)
                    sgTextures.push_back({lightMapName, lightMapMetadata.width, lightMapMetadata.height});
                else
                    giTextures.push_back({lightMapName, lightMapMetadata.width, lightMapMetadata.height});

                occlusionTextures.push_back({shadowMapName, shadowMapMetadata.width, shadowMapMetadata.height});
            }
            else
            {
                Logger::logFormatted(LogType::Warning, "Couldn't find lightmap textures for instance \"%s\"", name.c_str());
            }
        }
        else if (targetEngine == TargetEngine::HE1)
        {
            const std::string lightMapFilePath = inputDirectoryPath + "/" + name + "_lightmap.png";
            const std::string shadowMapFilePath = inputDirectoryPath + "/" + name + "_shadowmap.png";

            if (std::filesystem::exists(lightMapFilePath) && std::filesystem::exists(shadowMapFilePath))
            {
                std::unique_ptr<DirectX::ScratchImage> image = nullptr;
                {
                    std::unique_ptr<DirectX::ScratchImage> lightMapImage = std::make_unique<DirectX::ScratchImage>();
                    std::unique_ptr<DirectX::ScratchImage> shadowMapImage = std::make_unique<DirectX::ScratchImage>();

                    {
                        wchar_t filePathWideChar[1024];
                        MultiByteToWideChar(CP_UTF8, 0, lightMapFilePath.c_str(), -1, filePathWideChar, 1024);
                        DirectX::LoadFromWICFile(filePathWideChar, DirectX::WIC_FLAGS_NONE, nullptr, *lightMapImage);
                    }

                    {
                        wchar_t filePathWideChar[1024];
                        MultiByteToWideChar(CP_UTF8, 0, shadowMapFilePath.c_str(), -1, filePathWideChar, 1024);
                        DirectX::LoadFromWICFile(filePathWideChar, DirectX::WIC_FLAGS_NONE, nullptr, *shadowMapImage);
                    }

                    const DirectX::TexMetadata lightMapMetadata = lightMapImage->GetMetadata();
                    const DirectX::TexMetadata shadowMapMetadata = shadowMapImage->GetMetadata();

                    const size_t width = std::max((size_t)4, std::max(lightMapMetadata.width, shadowMapMetadata.width) >> group->level);
                    const size_t height = std::max((size_t)4, std::max(lightMapMetadata.height, shadowMapMetadata.height) >> group->level);

                    if (lightMapMetadata.width != width || lightMapMetadata.height != height)
                    {
                        std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                        DirectX::Resize(*lightMapImage->GetImage(0, 0, 0), width, height, DirectX::TEX_FILTER_BOX, *tmpImage);
                        lightMapImage.swap(tmpImage);
                    }

                    if (shadowMapMetadata.width != width || shadowMapMetadata.height != height)
                    {
                        std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                        DirectX::Resize(*shadowMapImage->GetImage(0, 0, 0), width, height, DirectX::TEX_FILTER_BOX, *tmpImage);
                        shadowMapImage.swap(tmpImage);
                    }

                    if (lightMapMetadata.format != DXGI_FORMAT_B8G8R8A8_UNORM)
                    {
                        std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                        DirectX::Convert(*lightMapImage->GetImage(0, 0, 0), DXGI_FORMAT_B8G8R8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *tmpImage);
                        lightMapImage.swap(tmpImage);
                    }

                    if (shadowMapMetadata.format != DXGI_FORMAT_R8_UNORM)
                    {
                        std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
                        DirectX::Convert(*shadowMapImage->GetImage(0, 0, 0), DXGI_FORMAT_R8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *tmpImage);
                        shadowMapImage.swap(tmpImage);
                    }

                    image = std::make_unique<DirectX::ScratchImage>();
                    image->Initialize2D(DXGI_FORMAT_B8G8R8A8_UNORM, width, height, 1, 1);

                    for (size_t i = 0; i < width * height; i++)
                    {
                        struct RGBA
                        {
                            uint8_t b : 8;
                            uint8_t g : 8;
                            uint8_t r : 8;
                            uint8_t a : 8;
                        };

                        RGBA color = *(RGBA*)(lightMapImage->GetPixels() + i * 4);
                        color.a = *(uint8_t*)(shadowMapImage->GetPixels() + i);

                        *(RGBA*)(image->GetPixels() + i * 4) = color;
                    }
                }

                const DirectX::TexMetadata metadata = image->GetMetadata();

                const std::string lightMapName = name + levelSuffix;
                images[lightMapName] = std::move(image);

                giTextures.push_back({ lightMapName, metadata.width, metadata.height });
            }
            else
            {
                Logger::logFormatted(LogType::Warning, "Couldn't find lightmap textures for instance \"%s\"", name.c_str());
            }
        }
    }, group, groupInfo);

    std::vector<Atlas> giAtlases = createAtlases(giTextures);
    std::vector<Atlas> sgAtlases = createAtlases(sgTextures);
    std::vector<Atlas> occlusionAtlases = createAtlases(occlusionTextures);

    hl::archive archive;

    size_t atlasIndex = 0;

    auto processAtlas = [&](Atlas& atlas, const bool isBc4, const size_t arraySize)
    {
        std::unique_ptr<DirectX::ScratchImage> atlasImage = std::make_unique<DirectX::ScratchImage>();
        atlasImage->Initialize2D(
            isBc4
                ? DXGI_FORMAT_R8_UNORM
                : targetEngine == TargetEngine::HE2 ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM,
            atlas.width, atlas.height, arraySize, 1);

        for (size_t i = 0; i < arraySize; i++)
        {
            const auto atlasMip = atlasImage->GetImage(0, i, 0);
            memset(atlasMip->pixels, 0, atlasMip->width * atlasMip->height * (isBc4 ? 1 : targetEngine == TargetEngine::HE2 ? 8 : 4));
        }

        for (auto& texture : atlas.textures)
        {
            const auto& img = images[texture.name];

            for (size_t i = 0; i < arraySize; i++)
            {
                const auto mip = img->GetImage(0, i, 0);

                DirectX::CopyRectangle(
                    *mip,
                    { 0, 0, mip->width, mip->height },
                    *atlasImage->GetImage(0, i, 0),
                    DirectX::TEX_FILTER_POINT,
                    texture.x,
                    texture.y);
            }
        }

        {
            std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();

            DirectX::GenerateMipMaps(
                atlasImage->GetImages(),
                atlasImage->GetImageCount(),
                atlasImage->GetMetadata(),
                DirectX::TEX_FILTER_BOX | DirectX::TEX_FILTER_FORCE_NON_WIC | DirectX::TEX_FILTER_SEPARATE_ALPHA,
                0,
                *tmpImage);

            atlasImage.swap(tmpImage);
        }

        if (targetEngine == TargetEngine::HE2 && !isBc4)
        {
            std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();
            {
                const auto lock = D3D11Device::lock();

                DirectX::Compress(
                    D3D11Device::get(),
                    atlasImage->GetImages(),
                    atlasImage->GetImageCount(),
                    atlasImage->GetMetadata(),
                    DXGI_FORMAT_BC6H_UF16,
                    DirectX::TEX_COMPRESS_PARALLEL,
                    DirectX::TEX_THRESHOLD_DEFAULT,
                    *tmpImage);
            }

            atlasImage.swap(tmpImage);
        }

        else
        {
            std::unique_ptr<DirectX::ScratchImage> tmpImage = std::make_unique<DirectX::ScratchImage>();

            DirectX::Compress(
                atlasImage->GetImages(),
                atlasImage->GetImageCount(),
                atlasImage->GetMetadata(),
                isBc4 ? DXGI_FORMAT_BC4_UNORM : DXGI_FORMAT_BC3_UNORM,
                DirectX::TEX_COMPRESS_PARALLEL,
                DirectX::TEX_THRESHOLD_DEFAULT,
                *tmpImage);

            atlasImage.swap(tmpImage);
        }


        DirectX::Blob blob;
        DirectX::SaveToDDSMemory(atlasImage->GetImages(), atlasImage->GetImageCount(), atlasImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, blob);

        std::string textureName;

        // Skip creating an atlas if it has only one texture
        if (atlas.textures.size() == 1 && targetEngine == TargetEngine::HE1)
            textureName = atlas.textures[0].name + ".dds";

        else 
        {
            char atlasName[8];
            sprintf(atlasName, "a%04d", (int)(atlasIndex++));
            atlas.name = atlasName;
            textureName = atlas.name + ".dds";
        }

        archive.push_back(std::move(hl::archive_entry::make_regular_file(toNchar(textureName.c_str()).data(), blob.GetBufferSize(), blob.GetBufferPointer())));
    };

    for (auto& atlas : giAtlases) processAtlas(atlas, false, 1);
    for (auto& atlas : sgAtlases) processAtlas(atlas, false, 4);
    for (auto& atlas : occlusionAtlases) processAtlas(atlas, true, 1);

    hl::hh::mirage::atlas_info atlasInfo;

    auto addAtlas = [&](const Atlas& atlas)
    {
        // Skip this atlas if it has only one texture
        if (atlas.textures.size() == 1 && targetEngine == TargetEngine::HE1) 
            return;

        hl::hh::mirage::atlas hhAtlas;
        hhAtlas.name = atlas.name;

        for (auto& texture : atlas.textures)
        {
            hhAtlas.textures.push_back(
            {
                texture.name,
                (float)texture.width / (float)atlas.width,
                (float)texture.height / (float)atlas.height,
                (float)texture.x / (float)atlas.width,
                (float)texture.y / (float)atlas.height,
            });
        }

        atlasInfo.atlases.push_back(std::move(hhAtlas));
    };

    for (auto& atlas : giAtlases) addAtlas(atlas);
    for (auto& atlas : sgAtlases) addAtlas(atlas);
    for (auto& atlas : occlusionAtlases) addAtlas(atlas);

    hl::mem_stream stream;
    atlasInfo.write(stream);

    archive.push_back(std::move(hl::archive_entry::make_regular_file(HL_NTEXT("atlasinfo"), stream.get_size(), stream.get_data_ptr())));

    return archive;
}

std::vector<PostRender::Atlas> PostRender::createAtlases(std::list<Texture>& textures)
{
    std::vector<Atlas> atlases;
    createAtlasesRecursively(textures, atlases);

    return atlases;
}

void PostRender::process(const std::string& stageDirectoryPath, const std::string& inputDirectoryPath, TargetEngine targetEngine)
{
    const std::string stageName = getFileName(stageDirectoryPath);

    const std::string resourcesFilePath = stageDirectoryPath + "/" + stageName + ".ar.00";
    if (!std::filesystem::exists(resourcesFilePath))
    {
        Logger::logFormatted(LogType::Error, "Failed to find %s.ar.00", stageName.c_str());
        return;
    }

    auto nResourcesFilePath = toNchar(resourcesFilePath.c_str());

    hl::archive resourcesArchive = hl::hh::ar::load(nResourcesFilePath.data());

    std::unique_ptr<hl::u8[]> groupInfoData;
    hl::u8* groupInfoOriginalData = nullptr;

    hl::hh::mirage::raw_gi_texture_group_info_v2* groupInfo = nullptr;
    for (auto& entry : resourcesArchive)
    {
        if (!hl::text::iequal(entry.name(), HL_NTEXT("gi-texture.gi-texture-group-info")))
            continue;

        // We have to copy to our own buffer otherwise when we resave the archive,
        // the contents are going to be corrupted due to the "fix" function.
        groupInfoData = std::make_unique<hl::u8[]>(entry.size());
        memcpy(groupInfoData.get(), entry.file_data(), entry.size());

        // At the same time, store the original pointer to fix memory sizes later on.
        groupInfoOriginalData = entry.file_data<hl::u8>();
        break;
    }

    if (!groupInfoData)
    {
        Logger::log(LogType::Error, "Failed to find gi-texture.gi-texture-group-info");
        return;
    }

    hl::hh::mirage::fix(groupInfoData.get());

    groupInfo = hl::hh::mirage::get_data<hl::hh::mirage::raw_gi_texture_group_info_v2>(groupInfoData.get());
    groupInfo->fix();

    const std::string stagePfdFilePath = stageDirectoryPath + "/Stage.pfd";
    if (!std::filesystem::exists(stagePfdFilePath))
    {
        Logger::log(LogType::Error, "Failed to find Stage.pfd");
        return;
    }

    auto nStagePfdFilePath = toNchar(stagePfdFilePath.c_str());

    hl::archive stageArchive = hl::hh::pfd::load(nStagePfdFilePath.data());

    // Clean GI archives
    for (auto it = stageArchive.begin(); it != stageArchive.end();)
    {
        if (hl::text::strstr((*it).name(), HL_NTEXT("tg-")))
        {
            ++it;
            continue;
        }

        it = stageArchive.erase(it);
    }

    hl::archive stageAddArchive;

    CriticalSection criticalSection;

    std::for_each(std::execution::par_unseq, &groupInfo->groups[0], &groupInfo->groups[groupInfo->groups.count], [&](auto& group)
    {
        const size_t index = std::distance(&groupInfo->groups[0], &group);

        char name[16];
        sprintf(name, "gia-%d.ar", (int)index);

        hl::mem_stream stream;

        hl::u32 memorySize = 0;
        {
            const hl::archive archive = createArchive(inputDirectoryPath, targetEngine, group.get(), groupInfo);

            for (auto& entry : archive)
                memorySize += (hl::u32)entry.size();

            CabinetCompression::save(archive, stream, name);
        }

        Logger::logFormatted(LogType::Normal, "Saved %s", name);

        // HACK: We update the memory size in the original file by getting the offset of the value from the fixed data.
        // The modifications are going to be reflected to the resaved resources archive.
        hl::endian_swap(memorySize);
        const size_t memorySizeOffset = (size_t)((hl::u8*)&group->memorySize - groupInfoData.get());
        *(hl::u32*)(groupInfoOriginalData + memorySizeOffset) = memorySize;

        hl::archive_entry entry = hl::archive_entry::make_regular_file(toNchar(name).data(), stream.get_size(), stream.get_data_ptr());

        std::lock_guard lock(criticalSection);
        {
            if (group->level == 2)
                stageArchive.push_back(std::move(entry));
            else
                stageAddArchive.push_back(std::move(entry));
        }
    });

    const std::string stageAddPfdFilePath = stageDirectoryPath + "/Stage-Add.pfd";
    auto nStageAddPfdFilePath = toNchar(stageAddPfdFilePath.c_str());

    {
        hl::packed_file_info stagePfi;
        hl::hh::pfd::save(stageArchive, nStagePfdFilePath.data(), hl::hh::pfd::default_alignment, &stagePfi);

        hl::mem_stream stream;
        savePfi(stagePfi, stream);

        addOrReplace(resourcesArchive, HL_NTEXT("Stage.pfi"), stream.get_data());

        Logger::log(LogType::Normal, "Saved Stage.pfd");
    }

    {
        hl::packed_file_info stageAddPfi;
        hl::hh::pfd::save(stageAddArchive, nStageAddPfdFilePath.data(), hl::hh::pfd::default_alignment, &stageAddPfi);

        hl::mem_stream stream;
        savePfi(stageAddPfi, stream);

        addOrReplace(resourcesArchive, HL_NTEXT("Stage-Add.pfi"), stream.get_data());

        Logger::log(LogType::Normal, "Saved Stage-Add.pfd");
    }

    hl::hh::ar::save(resourcesArchive, nResourcesFilePath.data());

    Logger::logFormatted(LogType::Normal, "Saved %s.ar.00", stageName.c_str());
}
