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
// Logging is potentially vulnerable to buffer overflows, especially if some user
// attempts to open a file with an incredibly long name and that file's name is
// logged. It isn't worth the effort to deal with but if some weird behavior
// occurs, that could be the reason why.
//
// Author: Chris Hayes
// --------------------------------------

#pragma once

namespace Logger
{
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
	void openLogFile(void);

	/// <summary>Opens a log file with the given name for writing.</summary>
	/// <param name='filePath'>The path of the file to use for logging.</param>
	/// <param name='replace'>If true, replaces the file if it exists. Otherwise, the already
	/// existing file will be appended to.</param>
	/// <remarks>If a log file is already open, a warning will be logged to the old log file and it
	/// will then be closed. If the new log file cannot be opened, an exception will be thrown. In the
	/// event that a log file is already open and a new log file cannot be opened, a warning and an error
	/// will both be logged in the old log file, but no exception will be thrown and the old log file
	/// will remain open. In any case, if a new log file is opened, a notice will be logged in that new file.</remarks>
	void openLogFile(const char* filePath, bool replace);

	/// <summary>Closes the currently open log file. A notice will be logged to the file
	/// to indicate that closing the file was intentional. If no log file is open, this
	/// function has no effect.</summary>
	void closeLogFile();

	/// <summary>Logs a notice to the log file. Arguments should be provided identically to using
	/// the printf family of functions. No newline should be provided at the end of the format string.
	/// If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logNotice(const char* format, ...);

	/// <summary>Logs a warning to the log file. Arguments should be provided identically to using
	/// the printf family of functions. No newline should be provided at the end of the format string.
	/// If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logWarning(const char* format, ...);

	/// <summary>Logs an error to the log file. Arguments should be provided identically to using
	/// the printf family of functions. No newline should be provided at the end of the format string.
	/// If no log file is currently open, an exception is thrown.</summary>
	/// <param name='format'>A format string for the output.</param>
	void logError(const char* format, ...);
}