#include "SHLightField.h"
#include "Utilities.h"

void SHLightField::save(hl::stream& stream, const std::vector<std::unique_ptr<SHLightField>>& shLightFields)
{
    const size_t headerPos = stream.tell();
    hl::bina::v2::raw_header::start_write(hl::bina::v2::ver_210, hl::bina::endian_flag::little, stream);

    const size_t dataBlockPos = stream.tell();
    hl::bina::v2::raw_data_block_header::start_write(hl::bina::endian_flag::little, stream);

    hl::str_table strTable;
    hl::off_table offTable;
    {
        const size_t dataPos = stream.tell();

        const hl::hh::needle::raw_sh_light_field header =
        {
            1, // version
            {}, // unknown
            (hl::u32)shLightFields.size(), // count
            sizeof(hl::hh::needle::raw_sh_light_field) // entries
        };

        offTable.push_back(dataPos + offsetof(hl::hh::needle::raw_sh_light_field, entries));

        stream.write_obj(header);

        for (auto& shLightField : shLightFields)
        {
            const size_t nodePos = stream.tell();

            const hl::hh::needle::raw_sh_light_field_node node =
            {
                nullptr, // name
                { (hl::u32)shLightField->resolution.x(), (hl::u32)shLightField->resolution.y(), (hl::u32)shLightField->resolution.z() }, // resolution
                { shLightField->position.x(), shLightField->position.y(), shLightField->position.z() }, // position
                { shLightField->rotation.x(), shLightField->rotation.y(), shLightField->rotation.z() }, // rotation
                { shLightField->scale.x(), shLightField->scale.y(), shLightField->scale.z() }, // scale
            };

            strTable.push_back({ shLightField->name.c_str(), nodePos + offsetof(hl::hh::needle::raw_sh_light_field_node, name) });

            stream.write_obj(node);
        }
    }

    hl::bina::v2::raw_data_block_header::finish_write64(dataBlockPos, hl::bina::endian_flag::little, strTable, offTable, stream);
    hl::bina::v2::raw_header::finish_write(headerPos, 1, hl::bina::endian_flag::little, stream);
}

void SHLightField::save(const std::string& filePath, const std::vector<std::unique_ptr<SHLightField>>& shLightFields)
{
    hl::file_stream stream(toNchar(filePath.c_str()).data(), hl::file::mode::write);
    save(stream, shLightFields);
}

Matrix3 SHLightField::getRotationMatrix() const
{
    return (Eigen::AngleAxisf(rotation.x(), Vector3::UnitX()) *
        Eigen::AngleAxisf(rotation.y(), Vector3::UnitY()) *
        Eigen::AngleAxisf(rotation.z(), Vector3::UnitZ())).matrix();
}

void SHLightField::setFromRotationMatrix(const Matrix3& matrix)
{
    rotation = matrix.eulerAngles(0, 1, 2);
}

Matrix4 SHLightField::getMatrix() const
{
    Affine3 affine =
        Eigen::Translation3f(position) *
        getRotationMatrix() *
        Eigen::Scaling(scale);

    return affine.matrix();
}
