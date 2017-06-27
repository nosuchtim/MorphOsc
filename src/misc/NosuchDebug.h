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

#pragma once

#include <string>

extern int NosuchDebugLevel;
extern bool NosuchDebugToConsole;
extern bool NosuchDebugToLog;
extern bool NosuchDebugTimeTag;
extern bool NosuchDebugThread;
extern bool NosuchDebugAutoFlush;
extern std::string NosuchDebugLogPath;
extern std::string NosuchDebugLogFile;
extern std::string NosuchDebugLogDir;
extern std::string NosuchDebugPrefix;
extern std::string NosuchCurrentDir;
extern std::string NosuchConfigDir;

std::string NosuchSnprintf(const char *fmt, ...);

void NosuchDebugInit();
void NosuchDebugCleanup();

void NosuchDebugSetLogDirFile(std::string logdir, std::string logfile);
void NosuchDebugDumpLog();
void NosuchDebug(char const *fmt, ... );
void NosuchDebug(int level, char const *fmt, ... );
void NosuchErrorOutput(const char *fmt, ...);
std::string NosuchFullPath(std::string file);
std::string NosuchConfigPath(std::string file);
std::string NosuchForwardSlash(std::string filepath);

#define DBGLEVEL 0

#if DBGLEVEL >= 0
#define DEBUGPRINT(x) NosuchDebug x
#else
#define DEBUGPRINT(x)
#endif

#if DBGLEVEL >= 1
#define DEBUGPRINT1(x) NosuchDebug x
#else
#define DEBUGPRINT1(x)
#endif

#if DBGLEVEL >= 2
#define DEBUGPRINT2(x) NosuchDebug x
#else
#define DEBUGPRINT2(x)
#endif

#define NosuchAssert(expr) if(!(expr)){ throw NosuchException("NosuchAssert (%s) failed at %s:%d",#expr,__FILE__,__LINE__);}

typedef void (*ErrorPopupFuncType)(const char* msg); 
extern ErrorPopupFuncType NosuchErrorPopup;