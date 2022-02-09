
#ifndef DUEL6_MALLOC_H
#define DUEL6_MALLOC_H

#include <memory>
#include <malloc.h>
#include <iostream>
#include <bit>
#include <array>
namespace Duel6 {
    namespace Memory {
        struct Bucket {
            Bucket * next = nullptr;
            size_t id = 0;
            size_t size = 0;
            bool used = false;
        };

        static std::array<Bucket*, 64> mempool;

        void *customMalloc(size_t size);

        void customFree(void *ptr);

        void cleanup();

    }
}
#endif