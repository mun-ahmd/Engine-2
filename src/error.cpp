#include "error.h"

static ErrorHandler ENGINE_2_ERROR_HANDLER;

void ENGINE2_THROW_ERROR(const char* error_message, Error_Severity severity)
{
	Error error;
	error.error = error_message;
	error.severity = severity;
	ENGINE_2_ERROR_HANDLER.addError(error);
}