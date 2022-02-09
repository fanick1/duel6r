
#include "Malloc.h"
#include "../Perf.h"
namespace Duel6 {
    namespace Memory {

        void *customMalloc(size_t size) {

            static size_t counter = 0;
            counter ++;
            if(counter % 5000 == 0){
                cleanup();
            }
            constexpr const size_t offset = sizeof(Bucket);
            const size_t alignedSize = std::bit_ceil(size);

            size_t bucket = std::countl_zero(alignedSize);
            if(mempool[bucket] != nullptr){
                Bucket * b = mempool[bucket];
                void * result = (char *)b + offset;
//                std::cout << " reusing id " << b->id << " of size " << b->size << " from bucket " << bucket << "\n";
                if(mempool[bucket]->next != nullptr){
                    mempool[bucket] = mempool[bucket]->next;
                } else {
                    mempool[bucket] = nullptr;
                }
                b->used = true;
                return result;
            }
            performance::traceEvent t("customMalloc");

            static size_t idCnt = 0;
            void * mem = malloc(alignedSize + offset);
            if(mem == nullptr){
//                std::cerr << "bad malloc of size " << size << "\n";
                return nullptr;
            }
            Bucket * b = reinterpret_cast<Bucket *>(mem);
            b->id = idCnt++;
            b->size = alignedSize;
            b->next = nullptr;
            b->used = true;
//            std::cout << "Malloc (size" << size << ") id:"  << b->id << " " << b->size << " B" << " bucket " << bucket << "\n";
            return (char *) mem + offset;
        }
        void cleanup() {
            performance::traceEvent t("cleanup");

            size_t bucket = 0;
            for(Bucket * b : mempool){
                if(b == nullptr) {
                    bucket++;
                    continue;
                }
                Bucket * del = b->next;
                while(del != nullptr){
                    Bucket * next = del->next;
                    del->next = nullptr;
//                    std::cout << "Freeing id:" << del->id << " " << del->size << " B" << " bucket " << bucket << "\n";

                    free(del);
                    del = next;
                }
//                std::cout << "Freeing id:" << b->id << " " << b->size << " B" << " bucket " << bucket << "\n";
                b->next = nullptr;
                free(b);
                mempool[bucket++] = nullptr;
            }
        }
        void customFree(void *ptr) {
            constexpr const size_t offset = sizeof(Bucket);
            if(ptr == nullptr){
//                std::cerr << "bad free of ptr " << ptr << "\n";
                return;
            }
            Bucket * b = reinterpret_cast<Bucket *>((char *) ptr - offset);
            if(!b->used){
//                std::cerr << "Free id: " << b->id << " double freeed! Skipping\n";
                return;
            }
            size_t bucket = std::countl_zero(b->size);
            b->next = mempool[bucket];
            mempool[bucket] = b;
            b->used = false;
        }
    }
}