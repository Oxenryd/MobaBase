#ifndef ERRORCODES_HPP
#define ERRORCODES_HPP

#include <cstdint>

#define EC_FAILED(ec) ((ec) != ErrorCode::OK)
#define EC_CHECK(ecVar, expr) (ecVar) = (expr); if (EC_FAILED(ecVar)) return (ecVar);

enum class ErrorCode : uint32_t
{
	OK = 0,

	// EXECUTION
	COMMAND_FAILED_EXECUTION			= 2500,

	// FILESYSTEM
	FILE_NOT_FOUND						= 5000,
	READ_ERROR							= 5001,

	// GRAPHICS
	SHADER_COMPILE_FAILED				= 10000,
	SHADER_COULD_NOT_READ_FILE			= 10001,
	SHADER_COULD_NOT_READ_BYTECODE		= 10002,

	// MEMORY
	IS_NULL								= 20000
};

#endif