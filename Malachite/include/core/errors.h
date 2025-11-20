#pragma once
#include <cstdint>
#include <stdexcept>


namespace MalachiteCore
{
    enum VMError : uint8_t
    {
        NO_ERROR = 0,
        EXIT,
        VMS_PTR_INVALID,
        VMCS_INVALID,
        
        //Arithmetic
        ZERO_DIVISION,
        NAN_FLOAT_VALUE,
        //MEMORY
        MEMORY_ACCESS_VIOLATION,
        STACK_OVERFLOW,
        STACK_UNDERFLOW
    };

    constexpr uint8_t ERROR_STACK_SIZE = 255;

    struct ErrorFrame {
        VMError error;
        uint64_t ip;
    };
}