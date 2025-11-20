#pragma once
#include "vm.h"


namespace MalachiteCore 
{
	//Frequently used - are inline
	//Rarely used - arent inline

	inline ValueType get_register_type(uint8_t reg_id) {
		if (reg_id < 64) return ValueType::INT;
		if (reg_id < 128) return ValueType::DOUBLE;
		if (reg_id < 192) return ValueType::PTR;
		return ValueType::ANY;  // tagged
	}

	VMError execute(VMState* state, VMCommand* commands, size_t commands_size);

    inline VMError arithmetic_handler(VMState* state, VMCommand* command)
    {

		switch (command->operation)
		{
		case OP_IADD_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i + state->registers[command->source1].i;
			break;
		case OP_ISUB_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i - state->registers[command->source1].i;
			break;
		case OP_IMUL_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i * state->registers[command->source1].i;
			break;
		case OP_IDIV_RRR:
			if (state->registers[command->source1].i == 0) return VMError::ZERO_DIVISION;
			state->registers[command->destination].i = state->registers[command->source0].i / state->registers[command->source1].i;
			break;
		case OP_IMOD_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i % state->registers[command->source1].i;
			break;
		case OP_INEG_RR:
			state->registers[command->destination].i = -state->registers[command->source0].i;
			break;
		case OP_DADD_RRR:
			state->registers[command->destination].d = state->registers[command->source0].d + state->registers[command->source1].d;
			break;
		case OP_DSUB_RRR:
			state->registers[command->destination].d = state->registers[command->source0].d - state->registers[command->source1].d;
			break;
		case OP_DMUL_RRR:
			state->registers[command->destination].d = state->registers[command->source0].d * state->registers[command->source1].d;
			break;
		case OP_DDIV_RRR:
			if (state->registers[command->source1].d == 0) return VMError::ZERO_DIVISION;
			state->registers[command->destination].d = state->registers[command->source0].d / state->registers[command->source1].d;
			break;
		case OP_DNEG_RR:
			state->registers[command->destination].d = -state->registers[command->source0].d;
			break;

		default:
			break;
		}

        return VMError::NO_ERROR;
    }

	inline VMError memory_handler(VMState* state, VMCommand* command)
	{
		switch (command->operation)
		{
		case OpCode::OP_LOAD_RM:
			if (command->source0 > MAX_MEMORY_SIZE - command->source1 ||
				command->source1 == 0 || command->source1 > REGISTER_SIZE) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			state->registers[command->destination].u = 0;
			for (int i = 0; i < command->source1; i++)
			{
				// Takes byte from memory
				uint8_t byte = state->memory[command->source0 + i];
				// Little-endian: little byte in little address
				state->registers[command->destination].u |= (static_cast<uint64_t>(byte) << (i * 8));
			}
			break;

		case OpCode::OP_STORE_MR:
		{
			if (command->destination > MAX_MEMORY_SIZE - command->source1 ||
				command->source1 == 0 || command->source1 > REGISTER_SIZE) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			for (int i = 0; i < command->source1; i++)
			{
				// Little-endian: takes bytes from little to big
				uint8_t byte = (state->registers[command->source0].u >> (i * 8)) & 0xFF;
				state->memory[command->destination + i] = byte;
			}
			break;
		}
		case OpCode::OP_MOV_RR:
			state->registers[command->destination] = state->registers[command->source0];
			break;
		case OpCode::OP_MOV_RI_INT:
			state->registers[command->destination].i = command->immediate.i;
			break;
		case OpCode::OP_MOV_RI_UINT:
			state->registers[command->destination].u = command->immediate.u;
			break;
		case OpCode::OP_MOV_RI_DOUBLE:
			state->registers[command->destination].d = command->immediate.d;
			break;
		case OpCode::OP_CREATE_FRAME:
		{
			state->data_stack.push(DataFrame{ state->fp, state->sp });
			state->fp = state->sp;
			break;
		}
		case OpCode::OP_DESTROY_FRAME:
		{
			if (state->data_stack.empty()) {
				return VMError::STACK_UNDERFLOW;
			}
			DataFrame df = state->data_stack.pop();
			state->fp = df.fp;
			state->sp = df.sp;
			break;
		}
		case OpCode::OP_PUSH: //dest-offset source0-reg_from 
		{
			if (state->sp < command->destination + STACK_END || state->sp - command->destination < STACK_END) {
				return VMError::STACK_OVERFLOW;
			}
			state->sp -= command->destination;

			if (state->sp < STACK_END || state->sp + command->destination >= STACK_START) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			*reinterpret_cast<uint64_t*>(&state->memory[state->sp]) =
				state->registers[command->source0].u;
			break;
		}

		case OpCode::OP_POP:		//dest-reg_to source0-offset
		{
			if (state->sp >= STACK_START || state->sp + command->source0 -1 > STACK_START) {
				return VMError::STACK_UNDERFLOW;
			}

			state->registers[command->destination].u =
				*reinterpret_cast<uint64_t*>(&state->memory[state->sp]);
			state->sp += command->source0;

			// Проверяем, что SP остался в границах стека
			if (state->sp >= STACK_START) {  
				return VMError::MEMORY_ACCESS_VIOLATION;
			}
			break;
		}
		case OpCode::OP_LOAD_LOCAL:
		{
			uint64_t offset = command->source0;
			// Проверяем, что offset не слишком большой
			if (offset > state->fp) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}
			uint64_t address = state->fp - offset;
			uint64_t size = command->source1;  // или command->source1, если переменный размер
			if (address + size > MAX_MEMORY_SIZE || address + size < address) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}
			state->registers[command->destination].u = 0;
			for (int i = 0; i < size; i++) {
				uint8_t byte = state->memory[address + i];
				state->registers[command->destination].u |=
					(static_cast<uint64_t>(byte) << (i * 8));
			}
			break;
		}
		case OpCode::OP_STORE_LOCAL:
		{
			uint64_t offset = command->destination;
			if (offset > state->fp || state->fp - offset < STACK_END) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}
			uint64_t size = command->source1;
			uint64_t address = state->fp - offset;
			if (address < STACK_END || address + size >= STACK_START) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			for (int i = 0; i < size; i++) {
				uint8_t byte = (state->registers[command->source0].u >> (i * 8)) & 0xFF;
				state->memory[address + i] = byte;
			}
			break;
		}
		case OpCode::OP_STORE_ENCLOSING_A:
		{
			// destination[memory-offset] source0[register] source1[depth]
			uint64_t offset_val = command->destination;    // смещение в целевом фрейме
			uint64_t src_reg = command->source0;           // регистр-источник данных  
			uint64_t size_and_depth = command->source1;             // размер переменной и глубина (сколько фреймов от начала)

			uint64_t size = size_and_depth >> 32;
			uint64_t depth = size_and_depth & 0xFFFFFFFF;  //0b0000000000000000000000000000000011111111111111111111111111111111;
			// Находим нужный фрейм через data_stack
			if (depth >= state->data_stack.size()) {
				return VMError::STACK_UNDERFLOW;
			}

			const DataFrame& target_frame = state->data_stack.at(depth);
			uint64_t target_fp = target_frame.fp;

			// Проверяем границы стека для целевого фрейма
			if (offset_val > target_fp || target_fp - offset_val < STACK_END) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			uint64_t address = target_fp - offset_val;

			if (address < STACK_END || address + size >= STACK_START) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			// Записываем данные из регистра-источника
			uint64_t value = state->registers[src_reg].u;
			for (int i = 0; i < size; i++) {
				uint8_t byte = (value >> (i * 8)) & 0xFF;
				state->memory[address + i] = byte;
			}
			break;
		}

		case OpCode::OP_LOAD_ENCLOSING_A:
		{
			// destination[register] source0[memory-offset] source1[depth]
			uint64_t dest_reg = command->destination;      // регистр-назначение
			uint64_t offset_val = command->source0;        // смещение в целевом фрейме
			uint64_t size_and_depth = command->source1;             // размер переменной и глубина (сколько фреймов от начала)

			uint64_t size = size_and_depth >> 32;
			uint64_t depth = size_and_depth & 0xFFFFFFFF;  //0b0000000000000000000000000000000011111111111111111111111111111111;
			if (depth >= state->data_stack.size()) {
				return VMError::STACK_UNDERFLOW;
			}

			const DataFrame& target_frame = state->data_stack.at(depth);
			uint64_t target_fp = target_frame.fp;

			if (offset_val > target_fp || target_fp - offset_val < STACK_END) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			uint64_t address = target_fp - offset_val;

			if (address < STACK_END || address + size >= STACK_START) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			// Читаем данные в регистр-назначение
			uint64_t value = 0;
			for (int i = 0; i < size; i++) {
				uint8_t byte = state->memory[address + i];
				value |= static_cast<uint64_t>(byte) << (i * 8);
			}
			state->registers[dest_reg].u = value;
			break;
		}
		case OpCode::OP_STORE_ENCLOSING_R:
		{
			// destination[memory-offset] source0[register] source1[depth]
			uint64_t offset_val = command->destination;    // смещение в целевом фрейме
			uint64_t src_reg = command->source0;           // регистр-источник данных  
			uint64_t size_and_depth = command->source1;             // размер переменной и глубина (сколько фреймов вверх)

			uint64_t size = size_and_depth >> 32;
			uint64_t depth = size_and_depth & 0xFFFFFFFF;  //0b0000000000000000000000000000000011111111111111111111111111111111;
			// Находим нужный фрейм через data_stack
			if (depth >= state->data_stack.size()) {
				return VMError::STACK_UNDERFLOW;
			}

			const DataFrame& target_frame = state->data_stack.at(state->data_stack.size() - 1 - depth);
			uint64_t target_fp = target_frame.fp;

			// Проверяем границы стека для целевого фрейма
			if (offset_val > target_fp || target_fp - offset_val < STACK_END) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			uint64_t address = target_fp - offset_val;

			if (address < STACK_END || address + size >= STACK_START) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			// Записываем данные из регистра-источника
			uint64_t value = state->registers[src_reg].u;
			for (int i = 0; i < size; i++) {
				uint8_t byte = (value >> (i * 8)) & 0xFF;
				state->memory[address + i] = byte;
			}
			break;
		}

		case OpCode::OP_LOAD_ENCLOSING_R:
		{
			// destination[register] source0[memory-offset] source1[depth]
			uint64_t dest_reg = command->destination;      // регистр-назначение
			uint64_t offset_val = command->source0;        // смещение в целевом фрейме
			uint64_t size_and_depth = command->source1;             // размер переменной и глубина (сколько фреймов вверх)

			uint64_t size = size_and_depth >> 32;
			uint64_t depth = size_and_depth & 0xFFFFFFFF;  //0b0000000000000000000000000000000011111111111111111111111111111111;
			if (depth >= state->data_stack.size()) {
				return VMError::STACK_UNDERFLOW;
			}

			const DataFrame& target_frame = state->data_stack.at(state->data_stack.size() - 1 - depth);
			uint64_t target_fp = target_frame.fp;

			if (offset_val > target_fp || target_fp - offset_val < STACK_END) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			uint64_t address = target_fp - offset_val;

			if (address < STACK_END || address + size >= STACK_START) {
				return VMError::MEMORY_ACCESS_VIOLATION;
			}

			// Читаем данные в регистр-назначение
			uint64_t value = 0;
			for (int i = 0; i < size; i++) {
				uint8_t byte = state->memory[address + i];
				value |= static_cast<uint64_t>(byte) << (i * 8);
			}
			state->registers[dest_reg].u = value;
			break;
		}
		default:
			break;
		}

		return VMError::NO_ERROR;
	}
	inline VMError logic_handler(VMState* state, VMCommand* command)
	{
		switch (command->operation)
		{
		case OpCode::OP_AND_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i != 0 && state->registers[command->source1].i != 0;		//AND first bit
			break;
		case OpCode::OP_OR_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i != 0 || state->registers[command->source1].i != 0;      //OR first bit
			break;
		case OpCode::OP_NOT_RR:
			state->registers[command->destination].i = !(state->registers[command->source0].i != 0);
			break;

		case OpCode::OP_BIT_AND_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i & state->registers[command->source1].i;      //OR first bit
			break;
		case OpCode::OP_BIT_OR_RRR:
			state->registers[command->destination].i = state->registers[command->source0].i | state->registers[command->source1].i;
			break;
		case OpCode::OP_BIT_NOT_RR:
			state->registers[command->destination].i = ~(state->registers[command->source0].i);
			break;
		case OpCode::OP_BIT_OFFSET_LEFT_RRR:
		{
			state->registers[command->destination].i = state->registers[command->source0].i << state->registers[command->source1].i;
			break;
		}
		case OpCode::OP_BIT_OFFSET_RIGHT_RRR:
		{
			state->registers[command->destination].i = state->registers[command->source0].i >> state->registers[command->source1].i;
			break;
		}
		case OpCode::OP_CMP_RR:
			//Reset logic flags state
			state->flags &= ~(EQUAL_FLAG | LESS_FLAG | GREATER_FLAG);

			if (state->registers[command->source0].i == state->registers[command->source1].i) {
				state->flags |= EQUAL_FLAG;
			}
			else if (state->registers[command->source0].i > state->registers[command->source1].i)
			{
				state->flags |= GREATER_FLAG;
			}
			else // < CASE
			{
				state->flags |= LESS_FLAG;
			}
			break;
		case OpCode::OP_DCMP_RR:
			//Reset logic flags state
			state->flags &= ~(EQUAL_FLAG | LESS_FLAG | GREATER_FLAG);

			auto is_nan = [](double value) -> bool {
				uint64_t bits = *reinterpret_cast<const uint64_t*>(&value);
				uint64_t exponent = (bits >> 52) & 0x7FF;  // 11 bits for exponent
				uint64_t mantissa = bits & 0xFFFFFFFFFFFFF; // 52 bits for mantissa
				return (exponent == 0x7FF) && (mantissa != 0);
				};
			if (is_nan(state->registers[command->source0].d) || is_nan(state->registers[command->source1].d)) {
				// NaN detected - don't set comparison flags
				return VMError::NAN_FLOAT_VALUE;
			}
			if (state->registers[command->source0].d == state->registers[command->source1].d) {
				state->flags |= EQUAL_FLAG;
			}
			else if (state->registers[command->source0].d > state->registers[command->source1].d)
			{
				state->flags |= GREATER_FLAG;
			}
			else // < CASE
			{
				state->flags |= LESS_FLAG;
			}
			break;
		}
		return VMError::NO_ERROR;
	}

	inline VMError control_flow_handler(VMState* state, VMCommand* command)
	{
		switch (command->operation)
		{
		case OpCode::OP_JMP:
			state->ip = command->destination;
			state->flags |= JUMPED_FLAG;
			break;

		case OpCode::OP_JE:
			if (state->flags & EQUAL_FLAG) {
				state->ip = command->destination;
				state->flags |= JUMPED_FLAG;
			}
			break;

		case OpCode::OP_JNE:
			if (!(state->flags & EQUAL_FLAG)) {  
				state->ip = command->destination;
				state->flags |= JUMPED_FLAG;
			}
			break;
		case OpCode::OP_JL:
			if (state->flags & LESS_FLAG) {
				state->ip = command->destination;
				state->flags |= JUMPED_FLAG;
			}
			break;

		case OpCode::OP_JG:
			if (state->flags & GREATER_FLAG) {
				state->ip = command->destination;
				state->flags |= JUMPED_FLAG;
			}
			break;
		case OpCode::OP_JEL:
			if (state->flags & LESS_FLAG || state->flags & EQUAL_FLAG) {
				state->ip = command->destination;
				state->flags |= JUMPED_FLAG;
			}
			break;

		case OpCode::OP_JEG:
			if (state->flags & GREATER_FLAG || state->flags & EQUAL_FLAG) {
				state->ip = command->destination;
				state->flags |= JUMPED_FLAG;
			}
			break;
		case OpCode::OP_CALL:
			// Используем только call_stack для управления вызовами
			if (state->call_stack.size() >= CALL_STACK_SIZE) {
				return VMError::STACK_OVERFLOW;
			}

			// Сохраняем возвратный адрес в call_stack
			state->call_stack.push(CallFrame{ state->ip + 1 });

			// Переходим к функции
			state->ip = command->destination;
			state->flags |= JUMPED_FLAG;
			break;

		case OpCode::OP_RET:
		{
			if (state->call_stack.empty()) {
				return VMError::STACK_UNDERFLOW;
			}

			CallFrame frame = state->call_stack.pop();
			state->ip = frame.return_ip;
			state->flags |= JUMPED_FLAG;
			break;
		}
		case OpCode::OP_HALT:
			return VMError::EXIT;
			break;
		default:
			break;
		}

		return VMError::NO_ERROR;
	}


	VMError syscalls_handler(VMState* state, VMCommand* command);
	VMError special_handler(VMState* state, VMCommand* command);
}