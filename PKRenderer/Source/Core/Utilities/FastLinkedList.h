#pragma once
#include <iterator>

namespace PK
{
    template<typename TValue, typename TKey>
    struct FastLinkedListRoot;

    template<typename TValue, typename TKey>
    struct FastLinkedListElement
    {
        friend struct FastLinkedListRoot<TValue, TKey>;

    protected:
        TKey linkKey{};
        TValue* linkNext = nullptr;
    public:

        static bool LinkedFind(TValue** first, const TKey& key)
        {
            auto iter = *first;
            TValue* prev = nullptr;

            while (iter)
            {
                if (iter->linkKey == key)
                {
                    break;
                }

                prev = iter;
                iter = iter->linkNext;
            }

            if (iter == nullptr)
            {
                return false;
            }

            if (prev)
            {
                // Swap found to the front
                prev->linkNext = iter->linkNext;
                iter->linkNext = *first;
                *first = iter;
            }

            return true;
        }

        static void LinkedInsert(TValue** first, TValue* newValue, const TKey& key)
        {
            newValue->linkNext = *first;
            *first = newValue;
            (*first)->linkKey = key;
        }
    };

    template<typename TValue, typename TKey>
    struct FastLinkedListRoot
    {
        TValue* first = nullptr;

        FastLinkedListRoot() {};
        FastLinkedListRoot(TValue* value) : first(value) {};

        operator TValue* () { return first; }
        operator const TValue* () const { return first; }
        const TValue* operator -> () const { return first; }
        TValue* operator -> () { return first; }

        bool FindAndSwapFirst(const TKey& key)
        {
            auto iter = first;
            TValue* prev = nullptr;

            while (iter)
            {
                if (iter->linkKey == key)
                {
                    break;
                }

                prev = iter;
                iter = iter->linkNext;
            }

            if (iter == nullptr)
            {
                return false;
            }

            if (prev)
            {
                // Swap found to the front
                prev->linkNext = iter->linkNext;
                iter->linkNext = first;
                first = iter;
            }

            return true;
        }

        void Insert(TValue* newValue, const TKey& key)
        {
            newValue->linkNext = first;
            first = newValue;
            first->linkKey = key;
        }

        struct ConstIterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = TValue*;
            using pointer = const TValue*;
            using reference = const TValue&;
            ConstIterator(TValue const* value) : data(value) {}
            reference operator*() const { return data; }
            pointer operator->() const { return data; }
            TValue const* data;

            bool operator != (const ConstIterator& iterator) const { return data != iterator.data; }
            ConstIterator operator++() { data = data->linkNext; return *this; }

        };

        struct Iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = TValue*;
            using pointer = TValue*;
            using reference = TValue*&;
            Iterator(TValue* value) : data(value) {}
            reference operator*() { return data; }
            pointer operator->() { return data; }
            TValue* data;

            bool operator != (const Iterator& iterator) const { return data != iterator.data; }
            Iterator operator++() { data = data->linkNext; return *this; }
        };

        ConstIterator begin() const { return ConstIterator(first); }
        ConstIterator end() const { return ConstIterator(nullptr); }

        Iterator begin() { return Iterator(first); }
        Iterator end() { return Iterator(nullptr); }
    };
}
