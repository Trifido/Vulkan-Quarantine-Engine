#pragma once

#ifndef REFLECT_SHADER_H
#define REFLECT_SHADER_H

#include <ostream>
#include <vector>
#include <SPIRV-Reflect/spirv_reflect.h>
#include <vulkan/vulkan.h>
#include <unordered_map>

struct DescriptorBindingReflect
{
    VkShaderStageFlagBits stage;
    uint32_t set;
    uint32_t binding;
    VkDescriptorType type;
    std::string name;
    uint32_t arraySize = 1;

    bool operator == (const DescriptorBindingReflect& c)
    {
        return (this->binding == c.binding && this->type == c.type && this->name == c.name && this->arraySize == c.arraySize);
    }
};

struct DescriptorSetReflect
{
    VkShaderStageFlagBits stage;
    uint32_t set;
    uint32_t bindingCount;
    DescriptorBindingReflect* bindings;

    bool Exist(const DescriptorBindingReflect& d)
    {
        for (int id = 0; id < bindingCount; id++)
        {
            if (bindings[id] == d)
            {
                return true;
            }
        }
        return false;
    }
};

class ReflectShader
{
public:
    std::unordered_map<std::string, DescriptorBindingReflect> bindings;
    //std::unordered_map<std::string, DescriptorBindingReflect> mixBindings;
    std::vector<DescriptorSetReflect> descriptorSetReflect;
    bool isAnimationShader = false;
private:
    std::string ToStringFormat(SpvReflectFormat fmt);
    static std::string ToStringScalarType(const SpvReflectTypeDescription& type);
    static std::string ToStringGlslType(const SpvReflectTypeDescription& type);
    static std::string ToStringHlslType(const SpvReflectTypeDescription& type);
    std::string ToStringType(SpvSourceLanguage src_lang, const SpvReflectTypeDescription& type);
    std::string ToStringSpvBuiltIn(SpvBuiltIn built_in);
    std::string ToStringDescriptorType(SpvReflectDescriptorType value);
    void PrintModuleInfo(std::ostream& os, const SpvReflectShaderModule& obj);
    void PrintDescriptorSet(std::ostream& os, const SpvReflectDescriptorSet& obj, const char* indent);
    void PrintDescriptorBinding(std::ostream& os, const SpvReflectDescriptorBinding& obj, bool write_set, const char* indent);
    void PrintInterfaceVariable(std::ostream& os, SpvSourceLanguage src_lang, const SpvReflectInterfaceVariable& obj, const char* indent);

    void CheckStage(DescriptorSetReflect& descripReflect, const SpvReflectShaderModule& obj);
    void CheckDescriptorSet(DescriptorSetReflect& descripReflect, const SpvReflectDescriptorSet& obj, const char* indent);
    DescriptorBindingReflect GetDescriptorBinding(const SpvReflectDescriptorBinding& obj, bool write_set, const char* indent);
public:
    ReflectShader();
    void Output(VkShaderModuleCreateInfo createInfo);
    void PerformReflect(VkShaderModuleCreateInfo createInfo);
    //void CheckMixStageBindings();
    //bool IsMixBinding(std::string name);
};

#endif // !REFLECT_SHADER_H
