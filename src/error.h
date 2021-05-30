#include <iostream>	//only included while it uses cerr

enum Error_Severity
{
	HIGH_SEVERITY,
	LOW_SEVERITY
};

struct Error
{
	Error_Severity severity;
	const char* error;
};

class ErrorHandler
{
private:
	Error* errors;
	size_t size = 0;
	size_t capacity = 0;
	void memcpy(void* source, void* dest, size_t num_bytes)
	{
		char* cdest = (char*)dest;
		char* csource = (char*)source;
		for (size_t i = 0; i < num_bytes; ++i)
		{
			cdest[i] = csource[i];
		}
	}
	void errorResponse()
	{
		//the more you flesh out this function, better quality error response
		std::cerr << "ERROR: "<<errors[size - 1].error;
		if (errors[size - 1].severity == HIGH_SEVERITY)
		{
			exit(222);
		}
		std::cerr << '\n';
	}
public:
	ErrorHandler()
	{
		capacity = 1;
		errors = new Error[1];
	}
	void addError(Error error)
	{
		if (size == capacity)
		{
			Error* errors_new = new Error[capacity + 1];
			this->memcpy(errors, errors_new,size*sizeof(Error));
			delete[](errors);
			errors = errors_new;
			capacity += 1;
		}
		errors[size] = error;
		size += 1;
		errorResponse();
	}
	void printAllErrors()
	{
		for (size_t i = 0;i < size;++i)
		{
			std::cout << errors[i].error;
		}
	}
};

static ErrorHandler ENGINE_2_ERROR_HANDLER;

void ENGINE2_THROW_ERROR(const char* error_message, Error_Severity severity = LOW_SEVERITY)
{
	Error error;
	error.error = error_message;
	error.severity = severity;
	ENGINE_2_ERROR_HANDLER.addError(error);
}