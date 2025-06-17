#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>

#ifdef LOGGING
	#define LOGLINE(type, mod, msg) Log::logLine(type, mod, msg)
	#define LOG(type, msg) Log::log(type, msg)
#else
	#define LOGLINE(type, msg) ((void)0)
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
	Input
};

constexpr const char* MODULE_STRINGS[]{
	"MEMORY\t",
	"ENGINE\t",
	"VULKAN\t",
	"DIRECTX\t",
	"RENDER\t",
	"WINDOW\t",
	"INPUT\t"
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
	Remark
};

class LoggerType
{
public:
	virtual ~LoggerType() {};
	virtual void logImpl(const LogType& type, const std::string& msg) const = 0;
	virtual void logLineImpl(const LogType& type, const LogMod& module, const std::string& msg) const = 0;
};

class DefaultTerminalLogger : public LoggerType
{
private:
	constexpr const char* _col(TermColor color) const {
		return CON_COL_FG[static_cast<uint8_t>(color)];
	}
public:
	~DefaultTerminalLogger() {}
	inline void logLineImpl(const LogType& type, const LogMod& module, const std::string& msg) const override {
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
			default:
				colStr = std::string{ _col(TermColor::Reset) }; break;
		}

		std::cout << '\n' << std::chrono::system_clock::now() << ":\t" << MODULE_STRINGS[static_cast<uint8_t>(module)] << colStr << msg << _col(TermColor::Reset);
	}
	inline void logImpl(const LogType& type, const std::string& msg) const override {
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
			default:
				colStr = std::string{ _col(TermColor::Reset) }; break;
		}

		std::cout << colStr << msg << _col(TermColor::Reset);
	}
};

class Log
{
public:
	template <typename T>
	inline static void init() {
		s_loggers.emplace_back(new T{});
	}
	inline static void logLine(const LogType& type, const LogMod& module, const std::string& msg) {
		for (auto* logger : s_loggers)
			logger->logLineImpl(type, module, msg);
	}
	inline static void log(const LogType& type, const std::string& msg) {
		for (auto* logger : s_loggers)
			logger->logImpl(type, msg);
	}

	inline static void deInit() {
		for (auto* logger : s_loggers) {
			delete logger;
			logger = nullptr;
		}
	}
private:
	inline static std::vector<LoggerType*> s_loggers;
};

#endif