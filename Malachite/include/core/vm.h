#pragma once
#include "operations.h"
#include "errors.h"
#include "vmstructs.h"

namespace MalachiteCore {
   
    // Section boundaries
    constexpr size_t HEAP_START = 0;
    constexpr size_t HEAP_END = HEAP_START + HEAP_SIZE - 1;

    constexpr size_t STACK_START = MAX_MEMORY_SIZE - 1;  // Top of memory
    constexpr size_t STACK_END = STACK_START - STACK_SIZE + 1;

    static constexpr uint32_t ZERO_FLAG = 1 << 0;
    static constexpr uint32_t JUMPED_FLAG = 1 << 1;
    static constexpr uint32_t STOPPED_FLAG = 1 << 2;     //External flag

    struct CallFrame {
        uint64_t return_ip;
        uint64_t base_ptr;  // Optionaly
    };
    struct DataFrame {
        uint64_t fp;
        uint64_t sp;  
    };

    using CallStack = Stack<CallFrame, CALL_STACK_SIZE>;
    using DataStack = Stack<DataFrame, CALL_STACK_SIZE>;

    using ErrorStack = Stack<ErrorFrame, ERROR_STACK_SIZE>;

    struct TypesTable 
    {
    
    };
    struct Register 
    {
        union {
            double d;
            int64_t i;
            uint64_t u;
        };
        explicit Register(): d(0){}
        explicit Register(double value) : d(value) {}
        explicit Register(int64_t value) : i(value) {}
        explicit Register(uint64_t value) : u(value) {}     //for pointers
    };
    struct VMCommand 
    {
        OpCode operation;   //RR,RRR, RM and MR types
        uint64_t destination;   //register or pointer
        uint64_t source0;       //register or pointer
        uint64_t source1;       //register or pointer
        Register immediate;     //raw data

        VMCommand(OpCode oper, uint64_t dest, uint64_t src0, uint64_t src1) : operation(oper), destination(dest), source0(src0), source1(src1) {}
        VMCommand(OpCode oper, uint64_t dest, uint64_t src0) : operation(oper), destination(dest), source0(src0) {}
        VMCommand(OpCode oper, uint64_t dest) : operation(oper), destination(dest){}
        VMCommand(OpCode oper, uint64_t dest, Register imm) : operation(oper), destination(dest), immediate(imm) {}
    };

   


    struct VMState {
        // Registers [cash friendly]
        Register registers[REGISTER_COUNT];
        uint64_t ip;           // Instruction Pointer
        uint64_t sp;           // Stack Pointer → grows to bottom
        uint64_t hp;           // Heap Pointer → grows to top
        uint64_t fp;           // Frame Pointer
        uint32_t flags;  //Special flags


        uint8_t memory[MAX_MEMORY_SIZE];    

        
        VMState() : ip(0),sp(MAX_MEMORY_SIZE - 1), hp(0), fp(MAX_MEMORY_SIZE - 1) {
            memset(memory, 0, MAX_MEMORY_SIZE);
        }
        // Стек вызовов
        CallStack call_stack;
        DataStack data_stack;       //<- Main DS
        DataStack temp_data_stack;  //<- DS for up/down frame transition 
        TypesTable types_table; //Таблица типов: классы и структуры
        ErrorStack error_stack;
    };

    inline bool is_valid_heap_address(uint64_t addr) {
        return addr >= HEAP_START && addr <= HEAP_END;
    }

    inline bool is_valid_stack_address(uint64_t addr) {
        return addr >= STACK_END && addr <= STACK_START;
    }
    /*
    OP_NEW_OBJECT, R1, PlayerClass
    OP_LOAD_INT, R2, 10
    OP_SET_FIELD, R1, Player_x_offset, R2
    */
}