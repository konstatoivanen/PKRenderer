#pragma once

namespace PK
{
    template<typename TValue, typename TKey>
    struct LinkedList;

    template<typename TValue, typename TKey>
    struct LinkedListElement
    {
        friend struct LinkedList<TValue, TKey>;

        void LinkedInsert(TValue* newValue, const TKey& key)
        {
            newValue->linkNext = linkNext;
            newValue->linkKey = key;
            linkNext = newValue;
        }

    protected:
        TKey linkKey{};
        TValue* linkNext = nullptr;
    };

    template<typename TValue, typename TKey>
    struct LinkedList
    {
        static_assert(__is_base_of(LinkedListElement<TValue, TKey>, TValue), "TValue is not derived from LinkedListElement!");

        TValue* first = nullptr;

        LinkedList() {};
        LinkedList(TValue* value) : first(value) {};

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
            ConstIterator(TValue const* value) : data(value) {}
            const TValue& operator*() const { return data; }
            const TValue* operator->() const { return data; }
            TValue const* data;

            bool operator != (const ConstIterator& iterator) const { return data != iterator.data; }
            ConstIterator operator++() { data = data->linkNext; return *this; }

        };

        struct Iterator
        {
            Iterator(TValue* value) : data(value) {}
            TValue*& operator*() { return data; }
            TValue* operator->() { return data; }
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
