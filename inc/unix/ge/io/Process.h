// Process.h

#ifndef PROCESS_H
#define PROCESS_H

#include <ge/common.h>

#include <ge/data/List.h>
#include <ge/io/FileInputStream.h>
#include <ge/io/FileOutputStream.h>
#include <ge/text/String.h>

/*
 * Unix Process implementation. Allows for executing both programs and shell
 * commands. Shell commands interpreted with the Bourne shell (or whatever
 * is named "/bin/sh")
 *
 * Processes not waited for will leak as zombie processes. Failure to read
 * process output can result in deadlock or corruption when the output buffer
 * space is limited by the OS.
 *
 * Not safe for access by multiple threads.
 */
class Process
{
public:
	Process();
	~Process();

	void execProgram(const String programName,
					 const List<String>& args);

	void execProgram(const String programName,
					 const List<String>& args,
					 bool mergeOutput);

	void execProgram(const String programName,
					 const List<String>& args,
					 const List<String>& env);

	void execProgram(const String programName,
					 const List<String>& args,
					 const List<String>& env,
					 bool mergeOutput);

	void execCommand(const String command);
	void execCommand(const String command, bool mergeOutput);

	bool isRunning();
	int32 waitFor();

	OutputStream* getStdIn() const;
	InputStream* getStdOut() const;
	InputStream* getStdErr() const;

private:
	Process(const Process& other) DELETED;
	Process& operator=(const Process& other) DELETED;

	void internalExec(const String& program,
					  const List<String>& args,
					  const List<String>& env,
					  bool mergeOutput);

	static void closePipe(int32* aPipe);
	static char** allocExecArray(const List<String>& args);

private:
	bool m_hasStarted;
	bool m_hasStopped;
	int32 m_return;
	pid_t m_pid;

	FileOutputStream* m_stdin;
	FileInputStream* m_stdout;
	FileInputStream* m_stderr;
};

#endif // UNIX_PROCESS_H
