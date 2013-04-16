#include "Logger.h"

#ifndef PAP_NO_LOGGING
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <exception>
#endif // PAP_NO_LOGGING

#ifdef _WIN32
// For some reason Microsoft renamed these functions, so I'm defining them here
// for portability purposes.
#define va_start _crt_va_start
#define va_end _crt_va_end
#endif

namespace Logger
{
	// Internal function for logging; all other log functions call this one.
	// Adds timestamp and formatting to log message.
	void log(const char* messageType, const char* format, va_list args);

	// Pointer to the file to use for logging. Should be set to NULL whenever closed.
	FILE* logFilePtr = NULL;

	/// <summary>Opens a log file with the default naming convention for writing.</summary>
	/// <remarks>The default naming convention for files is "log-{DATE}.txt".
	/// {DATE} is the date the log is opened, as MM-DD-YYYY. For example,  a log file opened on 
	/// February 19, 2013 would be named "log-02-19-2013.txt". If the file already exists,
	/// it will be appended to.
	///
	/// If a log file is already open, a warning will be logged to the old log file and it
	/// will then be closed. If the new log file cannot be opened, an exception will be thrown. In the
	/// event that a log file is already open and a new log file cannot be opened, a warning and an error
	/// will both be logged in the old log file, but no exception will be thrown and the old log file
	/// will remain open. In any case, if a new log file is opened, a notice will be logged in that new file.</remarks>
	void openLogFile(void)
	{
#ifndef PAP_NO_LOGGING
		char dateBuffer[19]; // exactly enough room to contain the file name plus null terminator

		// Retrieve the time then convert it into a date
		time_t curTime;
		time(&curTime);
		tm* curDate = localtime(&curTime);

		// Format the date appropriately
		strftime(dateBuffer, 19, "log-%m-%d-%Y.txt", curDate);

		openLogFile(dateBuffer, false);
#endif
	}

	/// <summary>Opens a log file with the given name for writing.</summary>
	/// <param name='filePath'>The path of the file to use for logging.</param>
	/// <param name='replace'>If true, replaces the file if it exists. Otherwise, the already
	/// existing file will be appended to.</param>
	/// <remarks>If a log file is already open, a warning will be logged to the old log file and it
	/// will then be closed. If the new log file cannot be opened, an exception will be thrown. In the
	/// event that a log file is already open and a new log file cannot be opened, a warning and an error
	/// will both be logged in the old log file, but no exception will be thrown and the old log file
	/// will remain open. In any case, if a new log file is opened, a notice will be logged in that new file.</remarks>
	void openLogFile(const char* filePath, bool replace)
	{
#ifndef PAP_NO_LOGGING
		FILE* newLogFilePtr = replace ? fopen(filePath, "w") : fopen(filePath, "a");
		
		if (logFilePtr && newLogFilePtr) // Log file already open
		{
			logWarning("Attempting to open new log file (%s) without closing old log file", filePath);
			fclose(logFilePtr);
			logFilePtr = newLogFilePtr;
		}
		else if (logFilePtr) // Old log exists but new one failed to open
		{
			logWarning("Attempting to open new log file (%s) without closing old log file", filePath);
			logError("Failed to open new log file (%s)! Leaving current log file open", filePath);
		}
		else if (newLogFilePtr) // No old log file, new one opened successfully
		{
			logFilePtr = newLogFilePtr;
		}
		else // No old log file and new one failed to open
		{
			throw "Failed to open log file!";
		}
#endif
	}

	/// <summary>Closes the currently open log file. A notice will be logged to the file
	/// to indicate that closing the file was intentional. If no log file is open, this
	/// function has no effect.</summary>
	void closeLogFile()
	{
#ifndef PAP_NO_LOGGING
		if (logFilePtr)
		{
			logNotice("Deliberate closing of log file");
			fclose(logFilePtr);
			logFilePtr = NULL;
		}
#endif
	}

	/// <summary>Logs a notice to the log file. Arguments should be provided identically to using
	/// the printf family of functions. No newline should be provided at the end of the format string.
	/// If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logNotice(const char* format, ...)
	{
#ifndef PAP_NO_LOGGING
		va_list args;
		va_start(args, format);
		log("NOTICE", format, args);
		va_end(args);
#endif
	}

	/// <summary>Logs a warning to the log file. Arguments should be provided identically to using
	/// the printf family of functions. No newline should be provided at the end of the format string.
	/// If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logWarning(const char* format, ...)
	{
#ifndef PAP_NO_LOGGING
		va_list args;
		va_start(args, format);
		log("WARNING", format, args);
		va_end(args);
#endif
	}

	/// <summary>Logs an error to the log file. Arguments should be provided identically to using
	/// the printf family of functions. No newline should be provided at the end of the format string.
	/// If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logError(const char* format, ...)
	{
#ifndef PAP_NO_LOGGING
		va_list args;
		va_start(args, format);
		log("ERROR", format, args);
		va_end(args);
#endif
	}

	void log(const char* messageType, const char* format, va_list args)
	{
#ifndef PAP_NO_LOGGING
		if (!Logger::logFilePtr)
			throw "Attempted to write log with no log file open!";
		
		char dateBuffer[24];
		char logFormatBuffer[1000]; // allocate lots of buffer room because why the hell not

		// Retrieve the time then convert it into a date
		time_t curTime;
		try {
		curTime = time(NULL);
		} catch(std::exception const&  e) {
			int a  =0;
			a = 4;
		}
		tm* curDate = localtime(&curTime);

		// Format the date appropriately
		strftime(dateBuffer, 24, "(%m-%d-%Y %I:%M:%S%p)", curDate);

		// Copy date, message type and format string into this function's own format string
		sprintf(logFormatBuffer, "%-25s %-9s %s\n", dateBuffer, messageType, format);

		// Write to file using the new format string
		vfprintf(Logger::logFilePtr, logFormatBuffer, args);
#ifndef PAP_NO_STDOUT
		vfprintf(stdout, logFormatBuffer, args);
#endif
#endif
	}
}