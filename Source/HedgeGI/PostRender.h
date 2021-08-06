#pragma once

enum class TargetEngine;

class PostRender
{
    class TextureNode;

    static void processInstancesRecursively(std::function<void(const std::string&)> function,
        hl::hh::mirage::raw_gi_texture_group* group, hl::hh::mirage::raw_gi_texture_group_info_v2* groupInfo);

public:
    struct Texture
    {
        std::string name;
        size_t width {};
        size_t height {};
        size_t x {};
        size_t y {};
    };

    struct Atlas
    {
        std::string name;
        std::vector<Texture> textures;
        size_t width {};
        size_t height {};
    };

    static void createAtlasesRecursively(std::list<Texture>& textures, std::vector<Atlas>& atlases);
    static std::vector<Atlas> createAtlases(std::list<Texture>& textures);

    static hl::archive createArchive(const std::string& inputDirectoryPath, TargetEngine targetEngine,
        hl::hh::mirage::raw_gi_texture_group* group, hl::hh::mirage::raw_gi_texture_group_info_v2* groupInfo, const hl::nchar* tempName);

    static void process(const std::string& stageDirectoryPath, const std::string& inputDirectoryPath, TargetEngine targetEngine);
};
