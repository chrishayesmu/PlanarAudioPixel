// -------------------------------------------------------------------
// This file defines a number of functions which are used for logging
// system messages. Messages are automatically dated and timestamped
// when logged. There are three types of messages:
//
// Notices  - These are messages of routine events, such as a client
//            connection or disconnection, or of playback starting.
//
// Warnings - These are messages of events which are unusual or dangerous
//            but not entirely unexpected. This includes events such as
//            a client disconnecting via timeout, or the system trying
//            to open an audio or volume file which does not exist.
//
// Errors   - These are messages for when the system encounters truly
//            anomalous conditions and cannot (or should not) continue.
//            This could include if the system suddenly loses its internet
//            connection, or if the system runs out of memory.
//
// Each type of message has its own logging function. Before any logging is
// performed, either openLogFile(void) or openLogFile(filePath) must be called
// to open the desired output file. There is a default naming convention for
// log files which is described above openLogFile(void). Note that only one log
// file may be open at a time; separate logs do not exist for each type of message.
// Opening a new log file will automatically close the current file, if any.
//
// For production use, if logging is not desired, then the symbol PAP_NO_LOGGING
// may be defined to disable logging. All functions will continue to be defined 
// but will have no effect (and likely be compiled out).
//
// Author: Chris Hayes
// --------------------------------------

#pragma once

namespace Logger
{
	/// <summary>Opens a log file with the default naming convention for writing. If a log file is 
	/// already open, a warning will be logged to the old log file and it will then be closed.</summary>
	/// <remarks>The default naming convention for files is "log-{DATE}-{NUM}.txt".
	/// {DATE} is the date the log is opened, as MM-DD-YYYY. {NUM} is a sequential number
	/// beginning at 1 which notes which log the file represents for the given date. For example,
	/// the third file opened on February 19, 2013 would be named "log-02-19-2013-3".</remarks>
	void openLogFile(void);

	/// <summary>Opens a log file with the given name for writing. If a log file is already open,
	/// a warning will be logged to the old log file and it will then be closed.</summary>
	/// <param name='filePath'>The path of the file to use for logging.</param>
	/// <param name='replace'>If true, replaces the file if it exists. Otherwise, the already
	/// existing file will be appended to.</param>
	void openLogFile(const char* filePath, bool replace);

	/// <summary>Closes the currently open log file. A notice will be logged to the file
	/// to indicate that closing the file was intentional.</summary>
	void closeLogFile();

	/// <summary>Logs a notice to the log file. Arguments should be provided identically to using
	/// the printf family of functions. If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logNotice(const char* format, ...);

	/// <summary>Logs a warning to the log file. Arguments should be provided identically to using
	/// the printf family of functions. If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logWarning(const char* format, ...);

	/// <summary>Logs an error to the log file. Arguments should be provided identically to using
	/// the printf family of functions. If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logError(const char* format, ...);
}