#pragma once

namespace PK
{
    struct NotSerialized {};
}

#if defined(_MSC_VER)
    #define PK_ECS_PRIVATE_FIELDS [[msvc::no_unique_address]] PK::NotSerialized _pknsz;
#else
    #define PK_ECS_PRIVATE_FIELDS [[no_unique_address]] PK::NotSerialized _pknsz;
#endif

// No unique address cannot be used before an enum.
#define PK_ECS_PRIVATE_FIELDS_FLAGS PK::NotSerialized _pknsz;
