#include "ReflectShader.h"
#include <iostream>
#include <cassert>

ReflectShader::ReflectShader()
{
}

std::string ReflectShader::ToStringDescriptorType(SpvReflectDescriptorType value) {
    switch (value) {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return "VK_DESCRIPTOR_TYPE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
    }
    // unhandled SpvReflectDescriptorType enum value
    return "VK_DESCRIPTOR_TYPE_???";
}

void ReflectShader::PrintModuleInfo(std::ostream& os, const SpvReflectShaderModule& obj) {
    os << "entry point     : " << obj.entry_point_name << "\n";
    os << "source lang     : " << spvReflectSourceLanguage(obj.source_language)
        << "\n";
    os << "source lang ver : " << obj.source_language_version << "\n";
    if (obj.source_language == SpvSourceLanguageGLSL) {
        os << "stage           : ";
        switch (obj.shader_stage) {
        default:
            break;
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
            os << "VS";
            break;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            os << "HS";
            break;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            os << "DS";
            break;
        case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
            os << "GS";
            break;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
            os << "PS";
            break;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
            os << "CS";
            break;
        }
    }
}

void ReflectShader::PrintDescriptorSet(std::ostream& os, const SpvReflectDescriptorSet& obj,
    const char* indent) {
    const char* t = indent;
    std::string tt = std::string(indent) + "  ";
    std::string ttttt = std::string(indent) + "    ";

    os << t << "set           : " << obj.set << "\n";
    os << t << "binding count : " << obj.binding_count;
    os << "\n";
    for (uint32_t i = 0; i < obj.binding_count; ++i) {
        const SpvReflectDescriptorBinding& binding = *obj.bindings[i];
        os << tt << i << ":"
            << "\n";
        PrintDescriptorBinding(os, binding, false, ttttt.c_str());
        if (i < (obj.binding_count - 1)) {
            os << "\n";
        }
    }
}

void ReflectShader::PrintDescriptorBinding(std::ostream& os,
    const SpvReflectDescriptorBinding& obj,
    bool write_set, const char* indent) {
    const char* t = indent;
    os << t << "binding : " << obj.binding << "\n";
    if (write_set) {
        os << t << "set     : " << obj.set << "\n";
    }
    os << t << "type    : " << ToStringDescriptorType(obj.descriptor_type)
        << "\n";

    // array
    if (obj.array.dims_count > 0) {
        os << t << "array   : ";
        for (uint32_t dim_index = 0; dim_index < obj.array.dims_count;
            ++dim_index) {
            os << "[" << obj.array.dims[dim_index] << "]";
        }
        os << "\n";
    }

    // counter
    if (obj.uav_counter_binding != nullptr) {
        os << t << "counter : ";
        os << "(";
        os << "set=" << obj.uav_counter_binding->set << ", ";
        os << "binding=" << obj.uav_counter_binding->binding << ", ";
        os << "name=" << obj.uav_counter_binding->name;
        os << ");";
        os << "\n";
    }

    os << t << "name    : " << obj.name;
    if ((obj.type_description->type_name != nullptr) &&
        (strlen(obj.type_description->type_name) > 0)) {
        os << " "
            << "(" << obj.type_description->type_name << ")";
    }
}

void ReflectShader::CheckStage(DescriptorSetReflect& descripReflect, const SpvReflectShaderModule& obj)
{
    switch (obj.shader_stage) {
    default:
        break;
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
        descripReflect.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        descripReflect.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        break;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        descripReflect.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        break;
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
        descripReflect.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
        break;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        descripReflect.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
        descripReflect.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    }
}

void ReflectShader::CheckDescriptorSet(DescriptorSetReflect& descripReflect, const SpvReflectDescriptorSet& obj, const char* indent)
{
    const char* t = indent;
    std::string tt = std::string(indent) + "  ";
    std::string ttttt = std::string(indent) + "    ";

    descripReflect.set = obj.set;
    descripReflect.bindingCount = obj.binding_count;
    descripReflect.bindings = new DescriptorBindingReflect[obj.binding_count];

    for (uint32_t i = 0; i < obj.binding_count; ++i) {
        const SpvReflectDescriptorBinding& binding = *obj.bindings[i];
        descripReflect.bindings[i] = GetDescriptorBinding(binding, false, ttttt.c_str());
    }
}

DescriptorBindingReflect ReflectShader::GetDescriptorBinding(const SpvReflectDescriptorBinding& obj, bool write_set, const char* indent)
{
    DescriptorBindingReflect descriptor = DescriptorBindingReflect();

    descriptor.binding = obj.binding;
    descriptor.type = (VkDescriptorType) obj.descriptor_type;

    if(obj.array.dims_count > 0)
        descriptor.arraySize = obj.array.dims[0];

    if (obj.type_description->type_name != NULL)
        descriptor.name = obj.type_description->type_name;
    else
        descriptor.name = "sampler2D";

    return descriptor;
}

void ReflectShader::Output(VkShaderModuleCreateInfo createInfo)
{
    auto k_sample_spv = createInfo.pCode;
    SpvReflectShaderModule module = {};
    SpvReflectResult result =
        spvReflectCreateShaderModule(sizeof(uint32_t) * createInfo.codeSize, k_sample_spv, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

#if defined(SPIRV_REFLECT_HAS_VULKAN_H)
    // Demonstrates how to generate all necessary data structures to create a
    // VkDescriptorSetLayout for each descriptor set in this shader.
    std::vector<DescriptorSetLayoutData> set_layouts(sets.size(),
        DescriptorSetLayoutData{});
    for (size_t i_set = 0; i_set < sets.size(); ++i_set) {
        const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
        DescriptorSetLayoutData& layout = set_layouts[i_set];
        layout.bindings.resize(refl_set.binding_count);
        for (uint32_t i_binding = 0; i_binding < refl_set.binding_count;
            ++i_binding) {
            const SpvReflectDescriptorBinding& refl_binding =
                *(refl_set.bindings[i_binding]);
            VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
            layout_binding.binding = refl_binding.binding;
            layout_binding.descriptorType =
                static_cast<VkDescriptorType>(refl_binding.descriptor_type);
            layout_binding.descriptorCount = 1;
            for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
                layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
            }
            layout_binding.stageFlags =
                static_cast<VkShaderStageFlagBits>(module.shader_stage);
        }
        layout.set_number = refl_set.set;
        layout.create_info.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout.create_info.bindingCount = refl_set.binding_count;
        layout.create_info.pBindings = layout.bindings.data();
    }
    // Nothing further is done with set_layouts in this sample; in a real
    // application they would be merged with similar structures from other shader
    // stages and/or pipelines to create a VkPipelineLayout.
#endif

  // Log the descriptor set contents to stdout
    const char* t = "  ";
    const char* tt = "    ";

    PrintModuleInfo(std::cout, module);
    std::cout << "\n\n";

    std::cout << "Descriptor sets:"
        << "\n";
    for (size_t index = 0; index < sets.size(); ++index) {
        auto p_set = sets[index];

        // descriptor sets can also be retrieved directly from the module, by set
        // index
        auto p_set2 = spvReflectGetDescriptorSet(&module, p_set->set, &result);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        assert(p_set == p_set2);
        (void)p_set2;

        std::cout << t << index << ":"
            << "\n";
        PrintDescriptorSet(std::cout, *p_set, tt);
        std::cout << "\n\n";
    }

    spvReflectDestroyShaderModule(&module);
}

void ReflectShader::PerformReflect(VkShaderModuleCreateInfo createInfo)
{
    auto k_sample_spv = createInfo.pCode;
    SpvReflectShaderModule module = {};
    SpvReflectResult result =
        spvReflectCreateShaderModule(sizeof(uint32_t) * createInfo.codeSize, k_sample_spv, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    const char* tt = "    ";

    DescriptorSetReflect descripReflect = {};

    CheckStage(descripReflect, module);

    for (size_t index = 0; index < sets.size(); ++index) {
        auto p_set = sets[index];
        auto p_set2 = spvReflectGetDescriptorSet(&module, p_set->set, &result);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        assert(p_set == p_set2);
        (void)p_set2;

        CheckDescriptorSet(descripReflect, *p_set, tt);
    }

    this->descriptorSetReflect.push_back(descripReflect);

    spvReflectDestroyShaderModule(&module);
}

void ReflectShader::CheckMixStageBindings()
{
    int idStage = 0;

    for (int i = 0; i < this->descriptorSetReflect.size(); i++)
    {
        for (int j = 0; j < this->descriptorSetReflect.size(); j++)
        {
            if (i == j)
                continue;

            for (int bi = 0; bi < this->descriptorSetReflect[i].bindingCount; bi++)
            {
                if (this->descriptorSetReflect[j].Exist(this->descriptorSetReflect[i].bindings[bi]))
                {
                    if (!this->IsMixBinding(this->descriptorSetReflect[i].bindings[bi].name))
                    {
                        mixBindings[this->descriptorSetReflect[i].bindings[bi].name] = this->descriptorSetReflect[i].bindings[bi];
                    }
                }
            }
        }
    }
}

bool ReflectShader::IsMixBinding(std::string name)
{
    if (mixBindings.empty())
        return false;

    auto finder = mixBindings.find(name);

    return finder != mixBindings.end();
}
