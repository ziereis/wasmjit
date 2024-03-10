#pragma once
#include <cinttypes>
#include <sys/mman.h>
#include <sys/stat.h>
#include <new>
#include <span>
#include <cassert>

template <uint32_t blockSize>
struct MemoryBlock {
    MemoryBlock* next;
    uint8_t mem[blockSize];
};


template<uint32_t blockSize>
class DynamicArenaAllocator {
public:
    DynamicArenaAllocator() :
    head{nullptr}, curAddr{nullptr}, curEnd{nullptr} {}

    DynamicArenaAllocator(DynamicArenaAllocator const&) = delete;
    DynamicArenaAllocator& operator=(DynamicArenaAllocator const&) = delete;
    DynamicArenaAllocator(DynamicArenaAllocator&&) = delete;
    DynamicArenaAllocator& operator=(DynamicArenaAllocator&&) = delete;

    ~DynamicArenaAllocator() {
        MemoryBlock<blockSize>* block = head;
        while (block != nullptr) {
            MemoryBlock<blockSize>* next = block->next;
            munmap(block, blockSize);
            block = next;
        }
    }

    void* allocate(uint32_t size) {
        assert(size <= blockSize && "Requested size is larger than block size");
        if (curAddr == nullptr || curAddr + size > curEnd) {
            MemoryBlock<blockSize>* newBlock = getNewBlock();
            if (newBlock == nullptr) {
                return nullptr;
            }
            newBlock->next = head;
            head = newBlock;
            curAddr = newBlock->mem;
            curEnd = newBlock->mem + blockSize;
        }
        void* result = curAddr;
        curAddr += size;
        return result;
    }

    MemoryBlock<blockSize>* getNewBlock() {
        void* mem = mmap(nullptr, blockSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) {
            return nullptr;
        }
        return static_cast<MemoryBlock<blockSize>*>(mem);
    }

    template<class T, class... Args>
    T* construct(Args&&... args)  {
        void* mem = allocate(sizeof(T));
        if (mem == nullptr) {
            return nullptr;
        }
        return new (mem) T(std::forward<Args>(args)...);
    }

    template<class T>
    std::span<typename std::remove_all_extents<T>::type> constructSpan(std::size_t count) {
        using U =  typename std::remove_all_extents<T>::type;
        void* mem = allocate(sizeof(U) * count);
        if (mem == nullptr) {
            throw std::bad_alloc();
        }
        return std::span<U>(static_cast<U*>(mem), count);
    }


private:
    MemoryBlock<blockSize>* head;
    uint8_t* curAddr;
    uint8_t* curEnd;
};
