#pragma once
#include <cstdint>
#include <cstdlib>
#include <iostream>
namespace MalachiteCore 
{
    enum FLAG : uint32_t
    {
        EQUAL_FLAG = 1 << 0,
        NOT_EQUAL_FLAG = 1 << 1,
        GREATER_FLAG = 1 << 2,
        LESS_FLAG = 1 << 3,
        JUMPED_FLAG = 1 << 4,
        STOPPED_FLAG = 1 << 5,
    };

    enum OpCode : uint16_t  //little-endian!
    {
        OP_NOP = 0,

        // Arithmetic [1-30]
        OP_IADD_RRR = 1,
        OP_ISUB_RRR,
        OP_IMUL_RRR,
        OP_IDIV_RRR,
        OP_IMOD_RRR,
        OP_INEG_RR,
        OP_UADD_RRR,
        OP_USUB_RRR,
        OP_UMUL_RRR,
        OP_UDIV_RRR,
        OP_UMOD_RRR,
        OP_DADD_RRR,
        OP_DSUB_RRR,
        OP_DMUL_RRR,
        OP_DDIV_RRR,
        OP_DNEG_RR,
        // ... 30

        // Logic [31-60]
        OP_AND_RRR = 31,
        OP_OR_RRR,
        OP_NOT_RR,
        OP_BIT_OR_RRR,
        OP_BIT_NOT_RR,
        OP_BIT_AND_RRR,
        OP_BIT_OFFSET_LEFT_RRR,
        OP_BIT_OFFSET_RIGHT_RRR,

        OP_CMP_RR,          //destination - null, source0 - first, source1 - second
        OP_DCMP_RR,     //destination - null, source0 - first, source1 - second; For double with nan checking
        OP_GET_FLAG,    //destination - register, source0 - flag type (check FLAG enum)

        //OP_GT
        //OP_LS
        // ... 60

        // Memory [61-80]
        OP_LOAD_RM = 61,    //register-destination,             address loading from - source0, size [1-8 bytes] - source1
        OP_STORE_MR,        //Address saving to-destination,    register - source0,             size [1-8 bytes] - source1
        OP_MOV_RR,
        OP_MOV_RI_INT,          //Integer
        OP_MOV_RI_UINT,         //Unsigned integer
        OP_MOV_RI_DOUBLE,       //Double
        OP_CREATE_FRAME,
        OP_DESTROY_FRAME,
        OP_PUSH,                // destination[size (not register)], source0[register from]
        OP_POP,                 // destination[register to], source0[size (not register)]
        OP_LOAD_LOCAL,          //destination[register]            source[memory-offset]          source1[size in bytes]  
        OP_STORE_LOCAL,         //destination[memory-offset]       source[register]               source1[size in bytes]  
        // A - absolute, r - relatively
        OP_STORE_ENCLOSING_A,     //destination[memory-offset]       source0[register]              source1[size and depth] size - 32 little bits, depth - 32 big bits       we store variable to n frame at start
        OP_LOAD_ENCLOSING_A,      //destination[register]            source0[memory-offset]         source1[size and depth] size - 32 little bits, depth - 32 big bits       we load variable from n frame at start
        OP_STORE_ENCLOSING_R,     //destination[memory-offset]       source0[register]              source1[size and depth] size - 32 little bits, depth - 32 big bits       we store variable to n frame from top
        OP_LOAD_ENCLOSING_R,      //destination[register]            source0[memory-offset]         source1[size and depth] size - 32 little bits, depth - 32 big bits       we load variable from n frame  from top

        OP_ALLOCATE_MEMORY,
        OP_FREE_MEMORY,

        // Control flow [91-120]  destination = where
        OP_JMP = 91,
        OP_JMP_CV,      //CV- Condition Valid - destination[where], source0[condition register]
        OP_JMP_CNV,     //CNV - Condition Not Valid - destination[where], source0[condition register]
        OP_CALL,
        OP_RET,
        OP_HALT,
        // ... 120

        // System Calls [121]
        OP_SYSTEM_CALL = 121,   //destination[SysCall], source0[param0], source1[param1]
        // Other: Types Convertion [122-127]
        OP_TC_ITD_R,    //Type Convertion Integer To Double
        OP_TC_DTI_R,    //Type Convertion Double To Integer

        OP_TC_UITD_R,    //Type Convertion Unsigned Integer To Double
        OP_TC_UITI_R,    //Type Convertion Unsigned Integer To Integer
        OP_TC_DTUI_R,    //Type Convertion Double To Unsigned Integer
        OP_TC_ITUI_R,    //Type Convertion Integer To Unsigned Integer
    };

    enum SysCall 
    {
        //PRINT
        PRINT_INT = 0,        // source0[param0-register],
        PRINT_UINT,        // source0[param0-register],
        PRINT_DOUBLE,        // source0[param0-register],
        PRINT_CHAR,         // source0[param0-register]
        PRINT_CHAR_ARRAY,   // source0[param0-pointer], source1[param1-size of string]
    };


    namespace OperationListBlock {
        constexpr uint16_t NOP = 0;
        constexpr uint16_t ARITHMETIC_START = 1;
        constexpr uint16_t ARITHMETIC_END = 30;
        constexpr uint16_t LOGIC_START = 31;
        constexpr uint16_t LOGIC_END = 60;
        constexpr uint16_t MEMORY_START = 61;
        constexpr uint16_t MEMORY_END = 80;
        constexpr uint16_t CONTROL_FLOW_START = 91;
        constexpr uint16_t CONTROL_FLOW_END = 120;
        constexpr uint16_t SYSTEM_CALLS_START = 121;
        constexpr uint16_t SYSTEM_CALLS_END = 121;

        inline bool IsOperationInInterval(OpCode value, uint16_t min, uint16_t max) 
        {
            return value <= max && value >= min;
        }
    }

}