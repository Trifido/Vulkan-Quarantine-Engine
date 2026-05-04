#pragma once

#ifndef SHADOW_MAPPING_MODE_H
#define SHADOW_MAPPING_MODE_H

enum ShadowMappingMode
{
    DIRECTIONAL_SHADOW,
    OMNI_SHADOW,
    CASCADE_SHADOW,
    NONE
};



namespace QE
{
    using ::ShadowMappingMode;
} // namespace QE
// QE namespace aliases
#endif
