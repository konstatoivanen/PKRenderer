#pragma once
#include "NoCopy.h"
#include <vector>
#include <exception>

namespace PK::Utilities
{
    template<typename T, size_t capacity>
    class Pool : NoCopy
    {
        using alloc_rebind = typename std::allocator_traits<std::allocator<T>>::template rebind_alloc<T>;
        using alloc_traits = std::allocator_traits<alloc_rebind>; 

        public:
            Pool()
            {
                m_data = alloc_traits::allocate(m_alloc, capacity);

                for (auto i = 0u; i < capacity; ++i)
                {
                    m_freelist.push_back(i);
                }
            }

            ~Pool()
            {
                std::sort(m_freelist.begin(), m_freelist.end());

                auto c = m_freelist.data();
                auto e = m_freelist.data() + m_freelist.size();

                for (auto i = 0u; i < capacity; ++i)
                {
                    if (c != e && *c == i)
                    {
                        c++;
                        continue;
                    }

                    alloc_traits::destroy(m_alloc, m_data + i);
                }

                alloc_traits::deallocate(m_alloc, m_data, capacity);
            }

            template<typename ... Args>
            T* New(Args&& ... args)
            {
                if (m_freelist.size() == 0)
                {
                    throw std::exception("Pool capacity exceeded!");
                }

                auto ptr = m_data + m_freelist.back();
                m_freelist.pop_back();
                alloc_traits::construct(m_alloc, ptr, std::forward<Args>(args)...);
                return ptr;
            }

            void Delete(const T* ptr)
            {
                if (ptr < m_data || ptr >= (m_data + capacity))
                {
                    throw std::exception("Trying to delete an element that doesn't belong to the pool");
                }

                auto index = ptr - m_data;
                m_freelist.push_back(index);
                alloc_traits::destroy(m_alloc, ptr);
            }

        private:
            T* m_data;
            std::vector<size_t> m_freelist;
            alloc_rebind m_alloc;
    };


}