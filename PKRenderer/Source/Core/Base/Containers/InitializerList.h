#pragma once
#include <initializer_list>

namespace PK
{
    // Aliased here to make tracking std:: references easier.
    template <class _Elem>
    using initializer_list = std::initializer_list<_Elem>;
}