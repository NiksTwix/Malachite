#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace Malachite 
{
	enum class MalachiteMessageType : uint8_t
	{
		INFO,
		WARNING,
		SYNTAX_ERROR,		
		TYPE_ERROR,
		RUNTIME_ERROR,		//MalachiteVirtualMachine::Execute()-> catchs VMError-> send message to Logger
		LOGIC_ERROR,
	};

	inline std::string GetMessageTypeString(MalachiteMessageType type)
	{
		switch (type)
		{
		case Malachite::MalachiteMessageType::INFO:
			return "Info";
		case Malachite::MalachiteMessageType::WARNING:
			return "Warning";
		case Malachite::MalachiteMessageType::SYNTAX_ERROR:
			return "Syntax error";
		case Malachite::MalachiteMessageType::TYPE_ERROR:
			return "Type error";
		case Malachite::MalachiteMessageType::RUNTIME_ERROR:
			return "Runtime error";
		case Malachite::MalachiteMessageType::LOGIC_ERROR:
			return "Logic error";
		default:
			return "Info";
		}
	}

	struct MalachiteMessage
	{
		MalachiteMessageType type;
		std::string text;
		int line;
	};


	class Logger 
	{
	private:
		uint16_t warnings_count = 0;
		uint16_t errors_count = 0;
	public:

		static Logger& Get() 
		{
			static Logger logger;
			return logger;
		}

		void ClearState() 
		{
			warnings_count = 0;
			errors_count = 0;
		}

		uint16_t GetErrorCounts() const
		{
			return errors_count;
		}
		uint16_t GetWarningCounts()	const
		{
			return warnings_count;
		}
		void PrintMessage(MalachiteMessage message) 
		{
			const char* color = "";
			switch (message.type) {
			case MalachiteMessageType::RUNTIME_ERROR: errors_count++; color = "\033[91m"; break; // red
			case MalachiteMessageType::TYPE_ERROR: errors_count++; color = "\033[91m"; break; // red
			case MalachiteMessageType::SYNTAX_ERROR: errors_count++; color = "\033[91m"; break; // red
			case MalachiteMessageType::LOGIC_ERROR: errors_count++; color = "\033[91m"; break; // red
			case MalachiteMessageType::WARNING: warnings_count++; color = "\033[93m"; break;      // yellow
			case MalachiteMessageType::INFO: color = "\033[92m"; break;         // green
			}
			std::cout << color << message.line << "|"
				<< GetMessageTypeString(message.type) << ": "
				<< message.text << "\033[0m\n";
		}

		void PrintInfo(const std::string& text, int line = 0)
		{
			PrintMessage({MalachiteMessageType::INFO, text, line});
		}

		void PrintWarning(const std::string& text, int line = 0)
		{
			PrintMessage({ MalachiteMessageType::WARNING, text, line });
		}
		void PrintSyntaxError(const std::string& text, int line = 0)
		{
			PrintMessage({ MalachiteMessageType::SYNTAX_ERROR, text, line });
		}
		void PrintRuntimeError(const std::string& text, int line = 0)
		{
			PrintMessage({ MalachiteMessageType::RUNTIME_ERROR, text, line });
		}
		void PrintLogicError(const std::string& text, int line = 0)
		{
			PrintMessage({ MalachiteMessageType::LOGIC_ERROR, text, line });
		}
		void PrintTypeError(const std::string& text, int line = 0)
		{
			PrintMessage({ MalachiteMessageType::TYPE_ERROR, text, line });
		}
	};
}