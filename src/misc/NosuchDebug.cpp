/*
	Copyright (c) 2011-2013 Tim Thompson <me@timthompson.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <windows.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>

#include <list>
#include "NosuchDebug.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
// #include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace std;

int NosuchDebugLevel = 0;
bool NosuchDebugToConsole = false;
bool NosuchDebugTimeTag = true;
bool NosuchDebugThread = true;
bool NosuchDebugToLog = true;
bool NosuchDebugToLogWarned = false;
bool NosuchDebugAutoFlush = true;

typedef void (*ErrorPopupFuncType)(const char* msg); 
ErrorPopupFuncType NosuchErrorPopup = NULL;

std::string NosuchDebugPrefix = "";
std::string NosuchDebugLogFile = "debug.txt";
std::string NosuchDebugLogDir = ".";
std::string NosuchDebugLogPath;
std::string NosuchCurrentDir = ".";
// std::string NosuchConfigDir = "config";

#ifdef DEBUG_TO_BUFFER
bool NosuchDebugToBuffer = true;
size_t NosuchDebugBufferSize = 8;
static std::list<std::string> DebugBuffer;
#endif

std::list<std::string> DebugLogBuffer;
bool DebugInitialized = FALSE;
long NosuchTime0;

HANDLE dMutex;

void
NosuchDebugSetLogDirFile(std::string logdir, std::string logfile)
{
	// If it's a full path starting with a drive letter or /)
	if ( logfile.find(":")!=logfile.npos 
		|| logfile.find("/")!=logfile.npos
		|| logfile.find("\\")!=logfile.npos ) {
		NosuchDebugLogFile = logfile;
		NosuchDebugLogDir = logdir;
		NosuchDebugLogPath = logfile;
	} else {
		NosuchDebugLogFile = logfile;
		NosuchDebugLogDir = logdir;
		NosuchDebugLogPath = logdir + "/" + logfile;
	}
}

void
RealDebugDumpLog() {
	std::ofstream f(NosuchDebugLogPath.c_str(),ios::app);
	if ( ! f.is_open() ) {
		NosuchDebugLogPath = "c:/windows/temp/debug.txt";
		f.open(NosuchDebugLogPath.c_str(),ios::app);
		if ( ! f.is_open() ) {
			return;
		}
	}

	while (!DebugLogBuffer.empty()) {
		std::string s = DebugLogBuffer.front();
	    f << s;
		DebugLogBuffer.pop_front();
	}
	f.close();
}

void
NosuchDebugDumpLog()
{
	DWORD wait = WaitForSingleObject( dMutex, INFINITE);
	if ( wait == WAIT_ABANDONED )
		return;

	RealDebugDumpLog();

	ReleaseMutex(dMutex);
}

void
NosuchDebugInit() {
	if ( ! DebugInitialized ) {
		dMutex = CreateMutex(NULL, FALSE, NULL);
		NosuchTime0 = timeGetTime();
		DebugInitialized = TRUE;
	}
}

void
NosuchDebugCleanup() {
	if ( DebugInitialized ) {
		ReleaseMutex(dMutex);
	}
}

bool NosuchDebugHack = false;

void
RealNosuchDebug(int level, char const *fmt, va_list args)
{
	NosuchDebugInit();
	if ( level > NosuchDebugLevel )
		return;

	DWORD wait = WaitForSingleObject( dMutex, INFINITE);
	if ( wait == WAIT_ABANDONED )
		return;

    // va_list args;
    char msg[10000];
	char* pmsg = msg;
	int msgsize = sizeof(msg)-2;

	if ( NosuchDebugPrefix != "" ) {
		int nchars = _snprintf_s(pmsg,msgsize,_TRUNCATE,"%s",NosuchDebugPrefix.c_str());
		pmsg += nchars;
		msgsize -= nchars;
	}
	if ( NosuchDebugTimeTag ) {
		int nchars;
		double secs = (timeGetTime()-NosuchTime0) / 1000.0f;
		nchars = _snprintf_s(msg,msgsize,_TRUNCATE,"[%.3f] ",secs);
		pmsg += nchars;
		msgsize -= nchars;
	}

    // va_start(args, fmt);
    vsprintf_s(pmsg,msgsize,fmt,args);

	char *p = strchr(msg,'\0');
	if ( p != NULL && p != msg && *(p-1) != '\n' ) {
		strcat_s(msg,msgsize,"\n");
	}

	if ( NosuchDebugToConsole ) {
		OutputDebugStringA(msg);
	}
	if ( NosuchDebugToLog ) {
		DebugLogBuffer.push_back(msg);
		if ( NosuchDebugAutoFlush )
			RealDebugDumpLog();
	}

#ifdef DEBUG_TO_BUFFER
	if ( NosuchDebugToBuffer ) {
		// We want the entries in the DebugBuffer to be single lines,
		// so that someone can request a specific number of lines.
		std::istringstream iss(msg);
		std::string line;
		while (std::getline(iss, line)) {
			DebugBuffer.push_back(line+"\n");
		}
		while ( DebugBuffer.size() >= NosuchDebugBufferSize ) {
			DebugBuffer.pop_front();
		}
	}
#endif

    // va_end(args);

	ReleaseMutex(dMutex);
}

void
NosuchDebug(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	RealNosuchDebug(0,fmt,args);
    va_end(args);
}

void
NosuchDebug(int level, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	RealNosuchDebug(level,fmt,args);
    va_end(args);
}

void
NosuchErrorOutput(const char *fmt, ...)
{
	NosuchDebugInit();

	if ( fmt == NULL ) {
		// Yes, this is recursive, but we're passing in a non-NULL fmt...
		NosuchErrorOutput("fmt==NULL in NosuchErrorOutput!?\n");
		return;
	}

    va_list args;
    va_start(args, fmt);

    char msg[10000];
    vsprintf_s(msg,sizeof(msg)-2,fmt,args);
    va_end(args);

	char *p = strchr(msg,'\0');
	if ( p != NULL && p != msg && *(p-1) != '\n' ) {
		strcat_s(msg,sizeof(msg),"\n");
	}

	if ( NosuchErrorPopup != NULL ) {
		NosuchErrorPopup(msg);
	}

	OutputDebugStringA(msg);

	NosuchDebug("NosuchErrorOutput: %s",msg);
}

std::string
NosuchFullPath(std::string filepath)
{
	if ( filepath == "." ) {
		return NosuchCurrentDir;
	} else {
		return NosuchCurrentDir + "/" + filepath;
	}
}

std::string
NosuchSnprintf(const char *fmt, ...)
{
	static char *msg = NULL;
	static int msglen = 4096;
	va_list args;

	if ( msg == NULL ) {
		msg = (char*)malloc(msglen);
	}

	while (1) {
		va_start(args, fmt);
		int written = vsnprintf_s(msg,msglen,_TRUNCATE,fmt,args);
		va_end(args);
		if ( written < msglen ) {
			return std::string(msg);
		}
		free(msg);
		msglen *= 2;
		msg = (char*)malloc(msglen);
	}
}

std::string
NosuchForwardSlash(std::string filepath) {
	size_t i;
	while ( (i=filepath.find("\\")) != filepath.npos ) {
		filepath.replace(i,1,"/");
	}
	return filepath;
}

BOOL DirectoryExists(const char* path)
{
  DWORD dwAttrib = GetFileAttributesA(path);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::string
MmttPath(std::string fn)
{
	static std::string savedroot = "";
	
	if ( savedroot == "" ) {
		// The MMTT environment variable overrides everything
		char* dir = getenv("MMTT");
		if ( dir != NULL ) {
			savedroot = std::string(dir);
			NosuchErrorOutput("Using MMTT environment value for root directory");
		}
		else {
			// Resort to \Users\Public
			char* userdir = "c:\\Users\\Public";
			savedroot = NosuchSnprintf("%s\\Documents\\Nosuch Media\\MultiMultiTouchTouch", userdir);
			NosuchErrorOutput("Using root directory: %s", savedroot.c_str());
		}
	}
	return NosuchSnprintf("%s/%s",savedroot.c_str(),fn.c_str());
}

std::string
MmttLogDir()
{
	return MmttPath("log");
}

std::string
NosuchConfigPath(std::string filepath)
{
	return MmttPath("config") + "/" + filepath;
}
