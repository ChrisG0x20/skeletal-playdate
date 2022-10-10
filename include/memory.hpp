//
// Copyright (c) 2022 Christopher Gassib
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLGMEMORY_HPP
#define CLGMEMORY_HPP

#include "pd.hpp"

namespace clg
{
    inline void* do_realloc(void* ptr, size_t byte_count)
    {
        auto result = pd::realloc(ptr, byte_count);
        pd::logToConsole("realloc(%p, %u) => %p", ptr, byte_count, result);
        return result;
    }

    // makes a best effort to return the requested amount of memory or as much as possible
    // when the return value != null, byte_count is set to the memory size returned
    inline void* allocate_up_to(size_t& byte_count)
    {
        const size_t requested_count = byte_count;
        // get the requested byte count or divide the requested byte count in half until allocation succeeds
        auto ptr = do_realloc(nullptr, byte_count);
        if (nullptr != ptr) // if (the requested memory was allocated) return the memory
        {
            return ptr;
        }

        while (nullptr == ptr)
        {
            byte_count /= 2;
            if (0 == byte_count)
            {
                return nullptr;
            }

            ptr = do_realloc(nullptr, byte_count);
        }

        // at this point some amount of memory was allocated; push the amount UP
        size_t above_limit = (byte_count * 2) >= requested_count ? requested_count : byte_count * 2;
        size_t step = (above_limit - byte_count) / 2;
        if (0 == step)
        {
            return ptr;
        }

        pd::realloc(ptr, 0);
        for (;;)
        {
            // deallocate and attempt to increase memory allocations until memory allocation fails
            ptr = do_realloc(nullptr, byte_count + step);
            if (nullptr == ptr) // pushed passed the limit
            {
                above_limit = byte_count + step;
                step = (above_limit - byte_count) / 2; // try a smaller increase
                continue;
            }

            // successfully allocated more memory; keep pushing up
            byte_count += step;
            step = (above_limit - byte_count) / 2;
            if (0 == step) // if (this is the ceiling) return largest amount of memory possible
            {
                return ptr;
            }

            pd::realloc(ptr, 0);
        }
    }

    class memory_arena
    {
        public:
        memory_arena()
            : p_pool(nullptr)
            , p_next(nullptr)
            , total_size(0)
            , is_owned(false)
        {
        }

        ~memory_arena()
        {
            deallocate_owned_pool();
        }

        // have the arena allocate and manage its own memory pool
        size_t initialize(size_t size)
        {
            deallocate_owned_pool();
            total_size = size;
            p_pool = allocate_up_to(total_size);
            if (nullptr == p_pool)
            {
                total_size = 0;
                return 0;
            }

            p_next = p_pool;
            is_owned = true;
            return total_size;
        }

        // have the arena allocate and manage an external memory pool
        size_t initialize(void* ptr, size_t size)
        {
            deallocate_owned_pool();
            p_pool = ptr;
            p_next = ptr;
            total_size = size;
            is_owned = false;
            return size;
        }

        // reset the memory arena usage tracking
        void reset()
        {
            p_next = p_pool;
        }

        // get the remaining bytes in this arena
        size_t get_free_count() const
        {
            return static_cast<uint8_t*>(p_pool) + total_size - static_cast<uint8_t*>(p_next);
        }

        // get the bytes currently used by this arena
        size_t get_used_count() const
        {
            return static_cast<uint8_t*>(p_next) - static_cast<uint8_t*>(p_pool);
        }

        // allocate bytes in this arena
        void* alloc(size_t size)
        {
            if (size > get_free_count())
            {
                pd::error("attempted to allocate more memory than arena has available");
                return nullptr;
            }

            auto p_result = p_next;
            p_next = static_cast<uint8_t*>(p_next) + size;
            return p_result;
        }

        // allocate aligned bytes in this arena
        template<int byte_alignment>
        void* aligned_alloc(size_t size)
        {
            const auto p_next_aligned = static_cast<uint8_t*>(clg::align_pointer<byte_alignment>(p_next));
            const auto additional_byte_count = p_next_aligned - static_cast<uint8_t*>(p_next);

            if (size + additional_byte_count > get_free_count())
            {
                pd::error("attempted to allocate more memory than arena has available");
                return nullptr;
            }

            p_next = static_cast<uint8_t*>(p_next_aligned) + size;
            return p_next_aligned;
        }

        private:
        // only deallocates if it allocated the memory pool
        void deallocate_owned_pool()
        {
            if (is_owned && nullptr != p_pool)
            {
                pd::realloc(p_pool, 0);
            }
        }

        void* p_pool;
        void* p_next;
        size_t total_size;
        bool is_owned;
    };
} // namespace clg

#endif // CLGMEMORY_HPP
