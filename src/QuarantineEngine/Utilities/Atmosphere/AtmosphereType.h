#pragma once

enum AtmosphereType
{
    CUBEMAP = 0,
    SPHERICALMAP = 1 << 0,
    PHYSICALLY_BASED_SKY = 2 << 0,
};


namespace QE
{
    using ::AtmosphereType;
} // namespace QE
// QE namespace aliases
