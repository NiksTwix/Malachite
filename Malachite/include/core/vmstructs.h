#pragma once
#include "string"

namespace MalachiteCore 
{
    constexpr size_t REGISTER_COUNT = 255;
    constexpr uint8_t REGISTER_SIZE = sizeof(uint64_t);
    constexpr size_t CALL_STACK_SIZE = 256;

    constexpr size_t MAX_MEMORY_SIZE = 65536; //64KB
    constexpr size_t STACK_SIZE = MAX_MEMORY_SIZE * 4 / 8;  // 32KB  
    constexpr size_t HEAP_SIZE = MAX_MEMORY_SIZE * 4 / 8;   // 32KB

    constexpr size_t MAX_MEMORY_RANGES_SIZES_COUNT = 512;
    constexpr size_t MAX_MEMORY_RANGES_COUNT = 1024;
    // Check (64KB = 16KB + 24KB + 24KB)
    static_assert(STACK_SIZE + HEAP_SIZE == MAX_MEMORY_SIZE,
        "Memory sections should sum to MAX_MEMORY_SIZE");

    enum ValueType : uint8_t 
    {
        INT     =   0,      // R0-R63: int
        DOUBLE  =  0x40,   // R64-R127: float
        PTR     =   0x80,   // R128-191: ptr
        ANY     = 0xC0,    // R192-R255: tagged values
    };


    struct Value {
        uint64_t data;
        ValueType type;  // TYPE_INT, TYPE_FLOAT, TYPE_PTR
    };


    template<typename T, size_t MAX_SIZE>
    class Stack {
    private:
        T m_data[MAX_SIZE];
        size_t m_top = 0;

    public:
        void push(const T& value) {
            if (m_top >= MAX_SIZE) {
                throw std::runtime_error("Malachite: Stack overflow");
            }
            m_data[m_top++] = value;
        }

        T pop() {
            if (m_top == 0) {
                throw std::runtime_error("Malachite: Stack underflow");
            }
            return m_data[--m_top];
        }

        T& top() {
            if (m_top == 0) throw std::runtime_error("Malachite: Stack empty");
            return m_data[m_top - 1];
        }

        const T& top() const {
            if (m_top == 0) throw std::runtime_error("Malachite: Stack empty");
            return m_data[m_top - 1];
        }

        const T& at(size_t index) const 
        {
            if (m_top == 0 || index >= size()) throw std::runtime_error("Malachite: Stack index " + std::to_string(index) + " out of range (size=" + std::to_string(size()) + ")");
            return m_data[index];
        }

        size_t size() const { return m_top; }
        bool empty() const { return m_top == 0; }
        void clear() { m_top = 0; }

        // Итераторы для отладки
        const T* begin() const { return m_data; }
        const T* end() const { return m_data + m_top; }
    };

    struct HeapMemoryAllocator    //
    {
    private:
        const uint8_t* space;
        const size_t heap_size;
        //Low level map
        uint8_t free_bitmap[HEAP_SIZE/8];  //sectors with alignas = 8 bytes

    public:
        HeapMemoryAllocator(uint8_t* memory, size_t size) : space(memory), heap_size(size) {}

        uint64_t allocate(size_t size)  //return memory pointer, if memory part with this size isnt defined -> error. pointer + size <= HEAP_END
        {
            return 0;
        }
        void free(uint64_t pointer, size_t size)
        {
            return;
        }
        /*
            __builtin_clzll(x)  // Count Leading Zeros
            __builtin_ctzll(x)  // Count Trailing Zeros  
            __builtin_ffsll(x)  // Find First Set (1-based)
            __builtin_popcountll(x) // Population Count
        
        */
    };

}