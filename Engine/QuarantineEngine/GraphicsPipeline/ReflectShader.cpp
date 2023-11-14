#include "ReflectShader.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <algorithm>

bool compareByLocation(const InputVars& a, const InputVars& b)
{
    return a.location < b.location;
}

ReflectShader::ReflectShader()
{
    this->nativesVariables.push_back("gl_VertexIndex");
}

std::string ReflectShader::ToStringFormat(SpvReflectFormat fmt) {
    switch (fmt) {
    case SPV_REFLECT_FORMAT_UNDEFINED:
        return "VK_FORMAT_UNDEFINED";
    case SPV_REFLECT_FORMAT_R16_UINT:
        return "VK_FORMAT_R16_UINT";
    case SPV_REFLECT_FORMAT_R16_SINT:
        return "VK_FORMAT_R16_SINT";
    case SPV_REFLECT_FORMAT_R16_SFLOAT:
        return "VK_FORMAT_R16_SFLOAT";
    case SPV_REFLECT_FORMAT_R16G16_UINT:
        return "VK_FORMAT_R16G16_UINT";
    case SPV_REFLECT_FORMAT_R16G16_SINT:
        return "VK_FORMAT_R16G16_SINT";
    case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
        return "VK_FORMAT_R16G16_SFLOAT";
    case SPV_REFLECT_FORMAT_R16G16B16_UINT:
        return "VK_FORMAT_R16G16B16_UINT";
    case SPV_REFLECT_FORMAT_R16G16B16_SINT:
        return "VK_FORMAT_R16G16B16_SINT";
    case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
        return "VK_FORMAT_R16G16B16_SFLOAT";
    case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
        return "VK_FORMAT_R16G16B16A16_UINT";
    case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
        return "VK_FORMAT_R16G16B16A16_SINT";
    case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
        return "VK_FORMAT_R16G16B16A16_SFLOAT";
    case SPV_REFLECT_FORMAT_R32_UINT:
        return "VK_FORMAT_R32_UINT";
    case SPV_REFLECT_FORMAT_R32_SINT:
        return "VK_FORMAT_R32_SINT";
    case SPV_REFLECT_FORMAT_R32_SFLOAT:
        return "VK_FORMAT_R32_SFLOAT";
    case SPV_REFLECT_FORMAT_R32G32_UINT:
        return "VK_FORMAT_R32G32_UINT";
    case SPV_REFLECT_FORMAT_R32G32_SINT:
        return "VK_FORMAT_R32G32_SINT";
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
        return "VK_FORMAT_R32G32_SFLOAT";
    case SPV_REFLECT_FORMAT_R32G32B32_UINT:
        return "VK_FORMAT_R32G32B32_UINT";
    case SPV_REFLECT_FORMAT_R32G32B32_SINT:
        return "VK_FORMAT_R32G32B32_SINT";
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
        return "VK_FORMAT_R32G32B32_SFLOAT";
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
        return "VK_FORMAT_R32G32B32A32_UINT";
    case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
        return "VK_FORMAT_R32G32B32A32_SINT";
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
        return "VK_FORMAT_R32G32B32A32_SFLOAT";
    case SPV_REFLECT_FORMAT_R64_UINT:
        return "VK_FORMAT_R64_UINT";
    case SPV_REFLECT_FORMAT_R64_SINT:
        return "VK_FORMAT_R64_SINT";
    case SPV_REFLECT_FORMAT_R64_SFLOAT:
        return "VK_FORMAT_R64_SFLOAT";
    case SPV_REFLECT_FORMAT_R64G64_UINT:
        return "VK_FORMAT_R64G64_UINT";
    case SPV_REFLECT_FORMAT_R64G64_SINT:
        return "VK_FORMAT_R64G64_SINT";
    case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
        return "VK_FORMAT_R64G64_SFLOAT";
    case SPV_REFLECT_FORMAT_R64G64B64_UINT:
        return "VK_FORMAT_R64G64B64_UINT";
    case SPV_REFLECT_FORMAT_R64G64B64_SINT:
        return "VK_FORMAT_R64G64B64_SINT";
    case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
        return "VK_FORMAT_R64G64B64_SFLOAT";
    case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
        return "VK_FORMAT_R64G64B64A64_UINT";
    case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
        return "VK_FORMAT_R64G64B64A64_SINT";
    case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
        return "VK_FORMAT_R64G64B64A64_SFLOAT";
    }
    // unhandled SpvReflectFormat enum value
    return "VK_FORMAT_???";
}

void ReflectShader::RemoveInputNativeVariables()
{
    for (int i = 0; i < this->nativesVariables.size(); i++)
    {
        int j = 0;
        while (j < this->inputVariables.size())
        {
            if (this->inputVariables.at(j).name == this->nativesVariables.at(i))
            {
                this->inputVariables.erase(this->inputVariables.begin() + j);
                break;
            }
            j++;
        }
    }
}

uint32_t ReflectShader::ToSizeFormat(SpvReflectFormat fmt) {
    switch (fmt) {
    case SPV_REFLECT_FORMAT_UNDEFINED:
        return 0;
    case SPV_REFLECT_FORMAT_R16_UINT:
        return 2;
    case SPV_REFLECT_FORMAT_R16_SINT:
        return 2;
    case SPV_REFLECT_FORMAT_R16_SFLOAT:
        return 2;
    case SPV_REFLECT_FORMAT_R16G16_UINT:
        return 4;
    case SPV_REFLECT_FORMAT_R16G16_SINT:
        return 4;
    case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
        return 4;
    case SPV_REFLECT_FORMAT_R16G16B16_UINT:
        return 6;
    case SPV_REFLECT_FORMAT_R16G16B16_SINT:
        return 6;
    case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
        return 6;
    case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
        return 8;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
        return 8;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
        return 8;
    case SPV_REFLECT_FORMAT_R32_UINT:
        return 4;
    case SPV_REFLECT_FORMAT_R32_SINT:
        return 4;
    case SPV_REFLECT_FORMAT_R32_SFLOAT:
        return 4;
    case SPV_REFLECT_FORMAT_R32G32_UINT:
        return 8;
    case SPV_REFLECT_FORMAT_R32G32_SINT:
        return 8;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
        return 8;
    case SPV_REFLECT_FORMAT_R32G32B32_UINT:
        return 12;
    case SPV_REFLECT_FORMAT_R32G32B32_SINT:
        return 12;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
        return 12;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
        return 16;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
        return 16;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    case SPV_REFLECT_FORMAT_R64_UINT:
        return 8;
    case SPV_REFLECT_FORMAT_R64_SINT:
        return 8;
    case SPV_REFLECT_FORMAT_R64_SFLOAT:
        return 8;
    case SPV_REFLECT_FORMAT_R64G64_UINT:
        return 16;
    case SPV_REFLECT_FORMAT_R64G64_SINT:
        return 16;
    case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
        return 16;
    case SPV_REFLECT_FORMAT_R64G64B64_UINT:
        return 24;
    case SPV_REFLECT_FORMAT_R64G64B64_SINT:
        return 24;
    case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
        return 24;
    case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
        return 32;
    case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
        return 32;
    case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
        return 32;
    }
    // unhandled SpvReflectFormat enum value
    return 0;
}

std::string ReflectShader::ToStringScalarType(const SpvReflectTypeDescription& type) {
    switch (type.op) {
    case SpvOpTypeVoid: {
        return "void";
        break;
    }
    case SpvOpTypeBool: {
        return "bool";
        break;
    }
    case SpvOpTypeInt: {
        if (type.traits.numeric.scalar.signedness)
            return "int";
        else
            return "uint";
    }
    case SpvOpTypeFloat: {
        switch (type.traits.numeric.scalar.width) {
        case 32:
            return "float";
        case 64:
            return "double";
        default:
            break;
        }
        break;
    }
    case SpvOpTypeStruct: {
        return "struct";
    }
    case SpvOpTypePointer: {
        return "ptr";
    }
    default: {
        break;
    }
    }
    return "";
}

std::string ReflectShader::ToStringGlslType(const SpvReflectTypeDescription& type) {
    switch (type.op) {
    case SpvOpTypeVector: {
        switch (type.traits.numeric.scalar.width) {
        case 32: {
            switch (type.traits.numeric.vector.component_count) {
            case 2:
                return "vec2";
            case 3:
                return "vec3";
            case 4:
                return "vec4";
            }
        } break;

        case 64: {
            switch (type.traits.numeric.vector.component_count) {
            case 2:
                return "dvec2";
            case 3:
                return "dvec3";
            case 4:
                return "dvec4";
            }
        } break;
        }
    } break;
    default:
        break;
    }
    return ToStringScalarType(type);
}

std::string ReflectShader::ToStringHlslType(const SpvReflectTypeDescription& type) {
    switch (type.op) {
    case SpvOpTypeVector: {
        switch (type.traits.numeric.scalar.width) {
        case 32: {
            switch (type.traits.numeric.vector.component_count) {
            case 2:
                return "float2";
            case 3:
                return "float3";
            case 4:
                return "float4";
            }
        } break;

        case 64: {
            switch (type.traits.numeric.vector.component_count) {
            case 2:
                return "double2";
            case 3:
                return "double3";
            case 4:
                return "double4";
            }
        } break;
        }
    } break;

    default:
        break;
    }
    return ToStringScalarType(type);
}

std::string ReflectShader::ToStringType(SpvSourceLanguage src_lang,
    const SpvReflectTypeDescription& type) {
    if (src_lang == SpvSourceLanguageHLSL) {
        return ToStringHlslType(type);
    }

    return ToStringGlslType(type);
}

std::string ReflectShader::ToStringSpvBuiltIn(SpvBuiltIn built_in) {
    switch (built_in) {
    case SpvBuiltInPosition:
        return "Position";
    case SpvBuiltInPointSize:
        return "PointSize";
    case SpvBuiltInClipDistance:
        return "ClipDistance";
    case SpvBuiltInCullDistance:
        return "CullDistance";
    case SpvBuiltInVertexId:
        return "VertexId";
    case SpvBuiltInInstanceId:
        return "InstanceId";
    case SpvBuiltInPrimitiveId:
        return "PrimitiveId";
    case SpvBuiltInInvocationId:
        return "InvocationId";
    case SpvBuiltInLayer:
        return "Layer";
    case SpvBuiltInViewportIndex:
        return "ViewportIndex";
    case SpvBuiltInTessLevelOuter:
        return "TessLevelOuter";
    case SpvBuiltInTessLevelInner:
        return "TessLevelInner";
    case SpvBuiltInTessCoord:
        return "TessCoord";
    case SpvBuiltInPatchVertices:
        return "PatchVertices";
    case SpvBuiltInFragCoord:
        return "FragCoord";
    case SpvBuiltInPointCoord:
        return "PointCoord";
    case SpvBuiltInFrontFacing:
        return "FrontFacing";
    case SpvBuiltInSampleId:
        return "SampleId";
    case SpvBuiltInSamplePosition:
        return "SamplePosition";
    case SpvBuiltInSampleMask:
        return "SampleMask";
    case SpvBuiltInFragDepth:
        return "FragDepth";
    case SpvBuiltInHelperInvocation:
        return "HelperInvocation";
    case SpvBuiltInNumWorkgroups:
        return "NumWorkgroups";
    case SpvBuiltInWorkgroupSize:
        return "WorkgroupSize";
    case SpvBuiltInWorkgroupId:
        return "WorkgroupId";
    case SpvBuiltInLocalInvocationId:
        return "LocalInvocationId";
    case SpvBuiltInGlobalInvocationId:
        return "GlobalInvocationId";
    case SpvBuiltInLocalInvocationIndex:
        return "LocalInvocationIndex";
    case SpvBuiltInWorkDim:
        return "WorkDim";
    case SpvBuiltInGlobalSize:
        return "GlobalSize";
    case SpvBuiltInEnqueuedWorkgroupSize:
        return "EnqueuedWorkgroupSize";
    case SpvBuiltInGlobalOffset:
        return "GlobalOffset";
    case SpvBuiltInGlobalLinearId:
        return "GlobalLinearId";
    case SpvBuiltInSubgroupSize:
        return "SubgroupSize";
    case SpvBuiltInSubgroupMaxSize:
        return "SubgroupMaxSize";
    case SpvBuiltInNumSubgroups:
        return "NumSubgroups";
    case SpvBuiltInNumEnqueuedSubgroups:
        return "NumEnqueuedSubgroups";
    case SpvBuiltInSubgroupId:
        return "SubgroupId";
    case SpvBuiltInSubgroupLocalInvocationId:
        return "SubgroupLocalInvocationId";
    case SpvBuiltInVertexIndex:
        return "VertexIndex";
    case SpvBuiltInInstanceIndex:
        return "InstanceIndex";
    case SpvBuiltInSubgroupEqMaskKHR:
        return "SubgroupEqMaskKHR";
    case SpvBuiltInSubgroupGeMaskKHR:
        return "SubgroupGeMaskKHR";
    case SpvBuiltInSubgroupGtMaskKHR:
        return "SubgroupGtMaskKHR";
    case SpvBuiltInSubgroupLeMaskKHR:
        return "SubgroupLeMaskKHR";
    case SpvBuiltInSubgroupLtMaskKHR:
        return "SubgroupLtMaskKHR";
    case SpvBuiltInBaseVertex:
        return "BaseVertex";
    case SpvBuiltInBaseInstance:
        return "BaseInstance";
    case SpvBuiltInDrawIndex:
        return "DrawIndex";
    case SpvBuiltInDeviceIndex:
        return "DeviceIndex";
    case SpvBuiltInViewIndex:
        return "ViewIndex";
    case SpvBuiltInBaryCoordNoPerspAMD:
        return "BaryCoordNoPerspAMD";
    case SpvBuiltInBaryCoordNoPerspCentroidAMD:
        return "BaryCoordNoPerspCentroidAMD";
    case SpvBuiltInBaryCoordNoPerspSampleAMD:
        return "BaryCoordNoPerspSampleAMD";
    case SpvBuiltInBaryCoordSmoothAMD:
        return "BaryCoordSmoothAMD";
    case SpvBuiltInBaryCoordSmoothCentroidAMD:
        return "BaryCoordSmoothCentroidAMD";
    case SpvBuiltInBaryCoordSmoothSampleAMD:
        return "BaryCoordSmoothSampleAMD";
    case SpvBuiltInBaryCoordPullModelAMD:
        return "BaryCoordPullModelAMD";
    case SpvBuiltInFragStencilRefEXT:
        return "FragStencilRefEXT";
    case SpvBuiltInViewportMaskNV:
        return "ViewportMaskNV";
    case SpvBuiltInSecondaryPositionNV:
        return "SecondaryPositionNV";
    case SpvBuiltInSecondaryViewportMaskNV:
        return "SecondaryViewportMaskNV";
    case SpvBuiltInPositionPerViewNV:
        return "PositionPerViewNV";
    case SpvBuiltInViewportMaskPerViewNV:
        return "ViewportMaskPerViewNV";
    case SpvBuiltInLaunchIdKHR:
        return "InLaunchIdKHR";
    case SpvBuiltInLaunchSizeKHR:
        return "InLaunchSizeKHR";
    case SpvBuiltInWorldRayOriginKHR:
        return "InWorldRayOriginKHR";
    case SpvBuiltInWorldRayDirectionKHR:
        return "InWorldRayDirectionKHR";
    case SpvBuiltInObjectRayOriginKHR:
        return "InObjectRayOriginKHR";
    case SpvBuiltInObjectRayDirectionKHR:
        return "InObjectRayDirectionKHR";
    case SpvBuiltInRayTminKHR:
        return "InRayTminKHR";
    case SpvBuiltInRayTmaxKHR:
        return "InRayTmaxKHR";
    case SpvBuiltInInstanceCustomIndexKHR:
        return "InInstanceCustomIndexKHR";
    case SpvBuiltInObjectToWorldKHR:
        return "InObjectToWorldKHR";
    case SpvBuiltInWorldToObjectKHR:
        return "InWorldToObjectKHR";
    case SpvBuiltInHitTNV:
        return "InHitTNV";
    case SpvBuiltInHitKindKHR:
        return "InHitKindKHR";
    case SpvBuiltInIncomingRayFlagsKHR:
        return "InIncomingRayFlagsKHR";
    case SpvBuiltInRayGeometryIndexKHR:
        return "InRayGeometryIndexKHR";

    case SpvBuiltInMax:
    default:
        break;
    }
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

void ReflectShader::PrintInterfaceVariable(std::ostream& os, SpvSourceLanguage src_lang, const SpvReflectInterfaceVariable& obj, const char* indent)
{
    const char* t = indent;
    os << t << "location  : ";
    if (obj.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
        os << ToStringSpvBuiltIn(obj.built_in) << " "
            << "(built-in)";
    }
    else {
        os << obj.location;
    }
    os << "\n";
    if (obj.semantic != nullptr) {
        os << t << "semantic  : " << obj.semantic << "\n";
    }
    os << t << "type      : " << ToStringType(src_lang, *obj.type_description)
        << "\n";
    os << t << "format    : " << ToStringFormat(obj.format) << "\n";
    os << t << "qualifier : ";
    if (obj.decoration_flags & SPV_REFLECT_DECORATION_FLAT) {
        os << "flat";
    }
    else if (obj.decoration_flags & SPV_REFLECT_DECORATION_NOPERSPECTIVE) {
        os << "noperspective";
    }
    os << "\n";

    os << t << "name      : " << obj.name;
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
        descripReflect.bindings[i].set = obj.set;
        descripReflect.bindings[i].stage = descripReflect.stage;

        auto bindingIdx = this->bindings.find(descripReflect.bindings[i].name);

        if (bindingIdx == this->bindings.end())
        {
            this->bindings[descripReflect.bindings[i].name] = descripReflect.bindings[i];
        }
        else
        {
            this->bindings[descripReflect.bindings[i].name].stage = (VkShaderStageFlagBits)(descripReflect.bindings[i].stage | this->bindings[descripReflect.bindings[i].name].stage);
        }
    }
}

void ReflectShader::CheckUBOMaterial(SpvReflectDescriptorSet* set)
{
    if (!this->isUBOMaterial)
    {
        for (int b = 0; b < set->binding_count && !this->isUBOMaterial; b++)
        {
            if (set->bindings[b]->block.name != NULL)
            {
                if (strcmp(set->bindings[b]->block.name, "uboMaterial") == 0)
                {
                    this->isUBOMaterial = true;
                    this->materialBufferSize = set->bindings[b]->block.size;
                    for (int m = 0; m < set->bindings[b]->block.member_count; m++)
                    {
                        materialUBOComponents.push_back(set->bindings[b]->block.members[m].name);
                    }
                }
            }
        }
    }
}

void ReflectShader::CheckUBOAnimation(SpvReflectDescriptorSet* set)
{
    if (!this->isUboAnimation)
    {
        for (int b = 0; b < set->binding_count && !this->isUboAnimation; b++)
        {
            if (set->bindings[b]->block.name != NULL)
            {
                if (strcmp(set->bindings[b]->block.name, "uboAnimation") == 0)
                {
                    this->isUboAnimation = true;
                    this->animationBufferSize = set->bindings[b]->block.size;
                    for (int m = 0; m < set->bindings[b]->block.member_count; m++)
                    {
                        animationUBOComponents.push_back(set->bindings[b]->block.members[m].name);
                    }
                }
            }
        }
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
    else if (strcmp(obj.name, "texSampler") == 0)
    {
        descriptor.name = "Texture2DArray";
    }
    else
        descriptor.name = obj.name;

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


    uint32_t count2 = 0;
    result = spvReflectEnumerateInputVariables(&module, &count2, NULL);
    //assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectInterfaceVariable*> input_vars(count2);
    result =
        spvReflectEnumerateInputVariables(&module, &count2, input_vars.data());
    //assert(result == SPV_REFLECT_RESULT_SUCCESS);

    if (result == SPV_REFLECT_RESULT_SUCCESS)
    {
        std::cout << "Input variables:"
            << "\n";
        for (size_t index = 0; index < input_vars.size(); ++index) {
            auto p_var = input_vars[index];

            // input variables can also be retrieved directly from the module, by
            // location (unless the location is (uint32_t)-1, as is the case with
            // built-in inputs)
            auto p_var2 =
                spvReflectGetInputVariableByLocation(&module, p_var->location, &result);
            if (p_var->location == UINT32_MAX) {
                assert(result == SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND);
                assert(p_var2 == nullptr);
            }
            else {
                assert(result == SPV_REFLECT_RESULT_SUCCESS);
                assert(p_var == p_var2);
            }
            (void)p_var2;

            // input variables can also be retrieved directly from the module, by
            // semantic (if present)
            p_var2 =
                spvReflectGetInputVariableBySemantic(&module, p_var->semantic, &result);
            if (!p_var->semantic) {
                assert(result == SPV_REFLECT_RESULT_ERROR_NULL_POINTER);
                assert(p_var2 == nullptr);
            }
            else if (p_var->semantic[0] != '\0') {
                assert(result == SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND);
                assert(p_var2 == nullptr);
            }
            else {
                assert(result == SPV_REFLECT_RESULT_SUCCESS);
                assert(p_var == p_var2);
            }
            (void)p_var2;

            std::cout << t << index << ":"
                << "\n";
            PrintInterfaceVariable(std::cout, module.source_language, *p_var, tt);
            std::cout << "\n\n";
        }
    }
    spvReflectDestroyShaderModule(&module);
}

void ReflectShader::PerformReflect(VkShaderModuleCreateInfo createInfo)
{
    SpvReflectShaderModule module = {};
    SpvReflectResult result = {};
    result = spvReflectCreateShaderModule(createInfo.codeSize, createInfo.pCode, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Descriptors
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
        this->CheckUBOMaterial(p_set);
        this->CheckUBOAnimation(p_set);
        auto p_set2 = spvReflectGetDescriptorSet(&module, p_set->set, &result);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        assert(p_set == p_set2);
        (void)p_set2;

        CheckDescriptorSet(descripReflect, *p_set, tt);
    }

    this->descriptorSetReflect.push_back(descripReflect);

    if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
    {
        uint32_t count2 = 0;
        result = spvReflectEnumerateInputVariables(&module, &count2, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectInterfaceVariable*> input_vars(count2);
        result =
            spvReflectEnumerateInputVariables(&module, &count2, input_vars.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        bool isBoneInput = false;
        bool isWeightInput = false;

        this->inputVariables.resize(input_vars.size());

        for (size_t index = 0; index < input_vars.size(); ++index)
        {
            uint32_t varSize = ToSizeFormat(input_vars[index]->format);
            std::string type = ToStringType(module.source_language, *(input_vars[index]->type_description));
            inputVariables.at(index) = InputVars(input_vars[index]->location, input_vars[index]->name, type, (VkFormat) input_vars[index]->format, varSize);

            std::string name = input_vars[index]->name;
            if (name == "inBoneIds")
            {
                isBoneInput = true;
            }
            if (name == "inWeights")
            {
                isWeightInput = true;
            }

            this->inputStrideSize += varSize;
        }

        this->RemoveInputNativeVariables();
        std::sort(this->inputVariables.begin(), this->inputVariables.end(), compareByLocation);
        this->isAnimationShader = isBoneInput && isWeightInput;
    }
    
    spvReflectDestroyShaderModule(&module);
    this->isShaderReflected = true;
}
