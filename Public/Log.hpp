#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>

#ifdef LOGGING
	#define LOGLINE(type, mod, msg) Log::logLine(type, mod, msg)
	#define LOGLINE_IND(type, mod, msg, ind) Log::logLine(type, mod, msg, ind)
	#define LOG(type, msg) Log::log(type, msg)
#else
	#define LOGLINE(type, mod, msg) ((void)0)
	#define LOGLINE_IND(type, mod, msg, ind) ((void)0)
	#define LOG(type, msg) ((void)0)
#endif

enum class LogMod : uint8_t
{
	Memory = 0,
	Engine,
	Vulkan,
	DirectX,
	Rendering,
	Window,
	Input,
	Log,
	Assets
};

constexpr const char* MODULE_STRINGS[]{
	"MEMORY\t",
	"ENGINE\t",
	"VULKAN\t",
	"DIRECTX\t",
	"RENDER\t",
	"WINDOW\t",
	"INPUT\t",
	"LOGGER\t",
	"ASSETS\t"
};

enum class TermColor : uint8_t
{
	Reset,
	Black,
	Red,
	Green,
	Yellow,
	Blue,
	Magenta,
	Cyan,
	White
};

constexpr const char* CON_COL_FG[]
{
	"\033[0m",
	"\033[030m",
	"\033[031m",
	"\033[032m",
	"\033[033m",
	"\033[034m",
	"\033[035m",
	"\033[036m",
	"\033[037m",
};

constexpr const char* CON_COL_BG[]
{
	"\033[0m",
	"\033[040m",
	"\033[041m",
	"\033[042m",
	"\033[043m",
	"\033[044m",
	"\033[045m",
	"\033[046m",
	"\033[047m",
};

enum class LogType
{
	Info,
	Warning,
	Success,
	Error,
	Remark,
	InProgress
};

class LoggerType
{
public:
	virtual ~LoggerType() {};
	virtual void logImpl(const LogType& type, const std::string_view& msg) = 0;
	virtual void logLineImpl(const LogType& type, const LogMod& module, const std::string_view& msg, const int8_t ind) = 0;
};

class DefaultTerminalLogger : public LoggerType
{
private:
	uint8_t m_indent = 0;
	constexpr const char* _col(TermColor color) const {
		return CON_COL_FG[static_cast<uint8_t>(color)];
	}
public:
	virtual ~DefaultTerminalLogger() override {}
	inline void logLineImpl(const LogType& type, const LogMod& module, const std::string_view& msg, const int8_t indent) override {
		
		if (indent < 0) {
			m_indent--;
			if (m_indent >= 0xf1)
				m_indent = 0;
		}

		std::string colStr;			
	
		switch (type)
		{
			case LogType::Error:
				colStr = std::string{ _col(TermColor::Red) }; break;
			case LogType::Warning:
				colStr = std::string{ _col(TermColor::Yellow) }; break;
			case LogType::Success:
				colStr = std::string{ _col(TermColor::Green) }; break;
			case LogType::Remark:
				colStr = std::string{ _col(TermColor::Cyan) }; break;
			case LogType::InProgress:
				colStr = std::string{ _col(TermColor::White) }; break;
			default:
				colStr = std::string{ _col(TermColor::Reset) }; break;
		}
		std::string indentStr{};
		for (size_t i = 0; i < m_indent; ++i) {
			indentStr.append("   -> ");
		}
		std::cout << '\n' << std::chrono::system_clock::now() << ":\t"
		<< MODULE_STRINGS[static_cast<uint8_t>(module)] << indentStr << colStr
		<< msg << _col(TermColor::Reset) << std::flush;
	
		if (indent > 0) {
			m_indent++;
			if (m_indent >= 0xf0)
				m_indent = 0xf0;
		}
	}
	inline void logImpl(const LogType& type, const std::string_view& msg) override {
		std::string colStr;
		switch (type) {
			case LogType::Error:
				colStr = std::string{ _col(TermColor::Red) }; break;
			case LogType::Warning:
				colStr = std::string{ _col(TermColor::Yellow) }; break;
			case LogType::Success:
				colStr = std::string{ _col(TermColor::Green) }; break;
			case LogType::Remark:
				colStr = std::string{ _col(TermColor::Cyan) }; break;
			case LogType::InProgress:
				colStr = std::string{ _col(TermColor::White) }; break;
			default:
				colStr = std::string{ _col(TermColor::Reset) }; break;
		}

		std::cout << colStr << msg << _col(TermColor::Reset) << std::flush;
	}
};

class Log
{
public:
	template<typename... LoggerTs>
	inline static void init() {

#ifndef LOGGING
		return;
#endif
		static_assert((std::is_base_of_v<LoggerType, LoggerTs> && ...),
					  "All LoggerTs must derive from LoggerType");
		if (!s_loggers.empty()) {
			logLine(LogType::Remark, LogMod::Log, "Reinit...\n");
			s_loggers.clear();
		}
		(_init<LoggerTs>(), ...);
		
		logLine(LogType::Info, LogMod::Log, "Initialized.");
	}
	inline static void logLine(const LogType& type, const LogMod& module, const std::string_view& msg) {
		for (auto* logger : s_loggers)
			logger->logLineImpl(type, module, msg, 0);
	}
	inline static void logLine(const LogType& type, const LogMod& module, const std::string_view& msg, int8_t ind) {
		for (auto* logger : s_loggers)
			logger->logLineImpl(type, module, msg, ind);
	}
	inline static void log(const LogType& type, const std::string_view& msg) {
		for (auto* logger : s_loggers)
			logger->logImpl(type, msg);
	}

	inline static void deInit() {
#ifndef LOGGING
		return;
#endif
		logLine(LogType::Info, LogMod::Log, "Cleaning up...\n");
		for (auto* logger : s_loggers) {
			delete logger;
			logger = nullptr;
		}
		s_loggers.clear();
		s_loggers.~vector();
	}
private:
	inline static std::vector<LoggerType*> s_loggers;
	template <typename T>
	static void _init() {
		s_loggers.emplace_back(new T{});
	}
};

#endif