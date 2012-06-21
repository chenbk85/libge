// Process.h

#ifndef PROCESS_H
#define PROCESS_H

#include <ge/common.h>
#include <ge/SystemException.h>
#include <ge/data/List.h>
#include <ge/io/FileInputStream.h>
#include <ge/io/FileOutputStream.h>
#include <ge/text/String.h>

#include <Windows.h>

/*
 * Windows process implementation. Used to create child processes with
 * redirected IO.
 */
class EXPORT Process
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

	int32 waitFor();

	OutputStream* getStdIn() const;
	InputStream* getStdOut() const;
	InputStream* getStdErr() const;

private:
	Process(const Process& other) DELETED;
	Process& operator=(const Process& other) DELETED;

	void internalExec(const String command,
					  const List<String>& args,
					  const List<String>& env,
					  bool mergeOutput);

	static HANDLE launchProcess(const String programName,
								const List<String>& args,
								const List<String>& env,
								HANDLE hChildStdIn,
								HANDLE hChildStdOut,
								HANDLE hChildStdErr);

	static void closePipe(HANDLE& readHandle, HANDLE& writeHandle);
	static String makeShellPath();
	static void makeParameterString(const String command,
                                    const List<String>& args,
                                    List<wchar_t>* result);
	static void makeEnvArray(const List<String>& env,
                             List<wchar_t>* envDest);

private:
	bool m_hasStarted;
	bool m_hasStopped;
	int32 m_return;

	HANDLE m_processHandle;

	FileOutputStream* m_stdin;
	FileInputStream* m_stdout;
	FileInputStream* m_stderr;
};

#endif // PROCESS_H
