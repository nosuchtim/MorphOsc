/*
* This code is derived from sample code included in TouchDesigner.
*/

#ifdef WIN32
#else
#include <string.h>
#define _strdup(s) strdup(s) 
#endif 
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "NosuchUtil.h"
#include "NosuchException.h"
#include "TuioServer.h"
#include "MMTT_SharedMem.h"

bool
MMTT_SharedMem::open(const char *name, unsigned int size, bool supportInfo)
{
	mySize = size;
	myMemory = 0;
	myMapping = 0;
	myName = 0;
	mySharedMemInfo = NULL;
	memset(myNamePostFix, 0, MMTT_SHM_MAX_POST_FIX_SIZE);
	myShortName = _strdup(name);

	mySupportInfo = supportInfo;

	size_t len = strlen(myShortName);

	createName();
	char *m = new char[strlen(myName) + 5 + 1];
	strcpy(m, myName);
	strcat(m, "Mutex");
	myMutex = new MMTT_Mutex(m);
	delete m;

	if (size > 0)
		myAmOwner = true;
	else
		myAmOwner = false;

	if (supportInfo)
	{
		if (!createInfo())
		{
			return false;
		}
	}
	else
	{
		mySharedMemInfo = NULL;
	}

	if (size > 0)
	{
		if (!createSharedMem())
		{
			myErrorState = MMTT_SHM_ERR_ALREADY_EXIST;
			return false;
		}
	}
	else
	{
		if (!openSharedMem())
		{
			myErrorState = MMTT_SHM_ERR_DOESNT_EXIST;
			return false;
		}
	}
	myErrorState = MMTT_SHM_ERR_NONE;
	return true;
}

MMTT_SharedMem::MMTT_SharedMem(const char *name)
{
	open(name);
}

MMTT_SharedMem::MMTT_SharedMem(const char *name, unsigned int size)
{
	open(name, size);
}

MMTT_SharedMem::MMTT_SharedMem(const char *name, unsigned int size, bool supportInfo)
{
	open(name, size, supportInfo);
}

MMTT_SharedMem::~MMTT_SharedMem()
{
	detach();
	delete myShortName;
	delete myName;
	delete mySharedMemInfo;
	delete myMutex;
}

bool
MMTT_SharedMem::checkInfo()
{
	if (mySupportInfo)
	{
		// If we are looking for an info and can't find it,
		// then release the segment also
		if (!createInfo())
		{
			detach();
			myErrorState = MMTT_SHM_ERR_INFO_DOESNT_EXIST;
			return false;
		}
	}

	if (mySharedMemInfo && mySharedMemInfo->getErrorState() == MMTT_SHM_ERR_NONE && !myAmOwner)
	{
		mySharedMemInfo->lock();
		MMTT_SharedMemInfo *info = (MMTT_SharedMemInfo*)mySharedMemInfo->getMemory();

		if (info->version > 1)
		{
			if (info->detach)
			{
				mySharedMemInfo->unlock();
				detach();
				myErrorState = MMTT_SHM_ERR_INFO_DOESNT_EXIST;
				return false;
			}
		}

		char pn[MMTT_SHM_MAX_POST_FIX_SIZE];
		memcpy(pn, info->namePostFix, MMTT_SHM_MAX_POST_FIX_SIZE);

		if (strcmp(pn, myNamePostFix) != 0)
		{
			memcpy(myNamePostFix, pn, MMTT_SHM_MAX_POST_FIX_SIZE);
			detachInternal();
		}
		mySharedMemInfo->unlock();

	}
	return true;
}

void
MMTT_SharedMem::resize(unsigned int s)
{

	// This can't be called by someone that didn't create it in the first place
	// Also you can't resize it if you arn't using the info feature
	// Finally, don't set the size to 0, just delete this object if you want to clean it
	if (mySize > 0 && mySharedMemInfo && myAmOwner)
	{
		mySharedMemInfo->lock();
		MMTT_SharedMemInfo *info = (MMTT_SharedMemInfo*)mySharedMemInfo->getMemory();
		if (info && info->supported)
		{
			detachInternal();
			mySize = s;
			// Keep trying until we find a name that works
			do
			{
				randomizePostFix();
				createName();
			} while (!createSharedMem());
			memcpy(info->namePostFix, myNamePostFix, MMTT_SHM_MAX_POST_FIX_SIZE);
		}
		else // Otherwise, just try and detach and resize, if it fails give up
		{
			detachInternal();
			mySize = s;
			if (!createSharedMem())
			{
				myErrorState = MMTT_SHM_ERR_ALREADY_EXIST;
			}

		}
		mySharedMemInfo->unlock();
	}
}

void
MMTT_SharedMem::randomizePostFix()
{
	for (int i = 0; i < MMTT_SHM_MAX_POST_FIX_SIZE - 1; i++)
	{
		int r = rand() % 26;
		char ch = 'a' + r;
		myNamePostFix[i] = ch;
	}
}

void
MMTT_SharedMem::createName()
{
	if (!myName)
		myName = new char[strlen(myShortName) + 10 + MMTT_SHM_MAX_POST_FIX_SIZE];

	strcpy(myName, "TouchSHM");
	strcat(myName, myShortName);
	strcat(myName, myNamePostFix);
}

bool
MMTT_SharedMem::createSharedMem()
{
	if (myMapping)
		return true;

#ifdef WIN32 
	myMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		mySize,
		s2ws(myName).c_str());

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		detach();
		return false;
	}
#else
	assert(false);
#endif 

	if (myMapping)
		return true;
	else
		return false;
}

bool
MMTT_SharedMem::openSharedMem()
{
	if (myMapping)
		return true;
	createName();
#ifdef WIN32 
	myMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, s2ws(myName).c_str());
#else
	assert(false);
#endif 

	if (!myMapping)
		return false;


	return true;
}

bool
MMTT_SharedMem::detachInternal()
{
	if (myMemory)
	{
#ifdef WIN32 
		UnmapViewOfFile(myMemory);
#else
		assert(false);
#endif 
		myMemory = 0;
	}
	if (myMapping)
	{
#ifdef WIN32 
		CloseHandle(myMapping);
#else
		assert(false);
#endif 
		myMapping = 0;
	}


	// Try to open the file again, if it works then someone else is still holding onto the file
	if (openSharedMem())
	{
#ifdef WIN32
		CloseHandle(myMapping);
#else
		assert(false);
#endif 
		myMapping = 0;
		return false;
	}


	return true;
}


bool
MMTT_SharedMem::detach()
{
	if (mySharedMemInfo)
	{
		if (mySharedMemInfo->getErrorState() == MMTT_SHM_ERR_NONE)
		{
			mySharedMemInfo->lock();
			MMTT_SharedMemInfo *info = (MMTT_SharedMemInfo*)mySharedMemInfo->getMemory();
			if (info && myAmOwner)
			{
				info->detach = true;
			}
			mySharedMemInfo->unlock();
		}
		delete mySharedMemInfo;
		mySharedMemInfo = NULL;
	}
	memset(myNamePostFix, 0, sizeof(myNamePostFix));
	return detachInternal();
}

bool
MMTT_SharedMem::createInfo()
{
	if (!mySupportInfo)
		return true;
	if (mySharedMemInfo)
	{
		return mySharedMemInfo->getErrorState() == MMTT_SHM_ERR_NONE;
	}

	srand((unsigned int)time(NULL));
	char *infoName = new char[strlen(myName) + strlen(MMTT_SHM_INFO_DECORATION) + 1];
	strcpy(infoName, myName);
	strcat(infoName, MMTT_SHM_INFO_DECORATION);
	mySharedMemInfo = new MMTT_SharedMem(infoName,
		myAmOwner ? sizeof(MMTT_SharedMemInfo) : 0, false);
	delete infoName;
	if (myAmOwner)
	{
		if (mySharedMemInfo->getErrorState() != MMTT_SHM_ERR_NONE)
		{
			myErrorState = MMTT_SHM_ERR_INFO_ALREADY_EXIST;
			return false;
		}
		mySharedMemInfo->lock();
		MMTT_SharedMemInfo *info = (MMTT_SharedMemInfo*)mySharedMemInfo->getMemory();
		if (!info)
		{
			myErrorState = MMTT_SHM_ERR_UNABLE_TO_MAP;
			mySharedMemInfo->unlock();
			return false;
		}
		info->magicNumber = MMTT_SHM_INFO_MAGIC_NUMBER;
		info->version = 2;
		info->supported = false;
		info->detach = false;
		memset(info->namePostFix, 0, MMTT_SHM_MAX_POST_FIX_SIZE);
		mySharedMemInfo->unlock();
	}
	else
	{
		if (mySharedMemInfo->getErrorState() != MMTT_SHM_ERR_NONE)
		{
			myErrorState = MMTT_SHM_ERR_INFO_DOESNT_EXIST;
			return false;
		}
		mySharedMemInfo->lock();
		MMTT_SharedMemInfo *info = (MMTT_SharedMemInfo*)mySharedMemInfo->getMemory();
		if (!info)
		{
			myErrorState = MMTT_SHM_ERR_UNABLE_TO_MAP;
			mySharedMemInfo->unlock();
			return false;
		}
		if (info->magicNumber != MMTT_SHM_INFO_MAGIC_NUMBER)
		{
			myErrorState = MMTT_SHM_ERR_INFO_DOESNT_EXIST;
			mySharedMemInfo->unlock();
			return false;
		}
		// Let the other process know that we support the info
		info->supported = true;
		mySharedMemInfo->unlock();
	}

	return true;
}

void *
MMTT_SharedMem::getMemory()
{
	if (!checkInfo())
	{
		return NULL;
	}

	if (myMemory == 0)
	{
		if ((myAmOwner && createSharedMem()) || (!myAmOwner && openSharedMem()))
		{
#ifdef WIN32 
			myMemory = MapViewOfFile(myMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
			assert(false);
			myMemory = NULL;
#endif 
			if (!myMemory)
				myErrorState = MMTT_SHM_ERR_UNABLE_TO_MAP;
		}
	}
	if (myMemory)
	{
		myErrorState = MMTT_SHM_ERR_NONE;
	}
	return myMemory;
}

bool
MMTT_SharedMem::lock()
{
	return myMutex->lock(5000);
}

bool
MMTT_SharedMem::unlock()
{
	return myMutex->unlock();
}

buff_index
Outlines_SharedMemHeader::grab_unused_buffer()
{
	buff_index found = BUFF_UNSET;
	for (int b = 0; b<NUM_BUFFS; b++) {
		if (!buff_inuse[b]) {
			found = b;
			break;
		}
	}
	if (found != BUFF_UNSET) {
		buff_inuse[found] = true;
	}
	return found;
}

void
Outlines_SharedMemHeader::xinit()
{
	// Magic number to make sure we are looking at the correct memory
	// must be set to MMTT_SHM_MAGIC_NUMBER
	magicNumber = MMTT_SHM_MAGIC_NUMBER;
	// header, must be set to MMTT_SHM_VERSION_NUMBER
	version = MMTT_SHM_VERSION_NUMBER;

	for (buff_index b = 0; b<NUM_BUFFS; b++) {
		outlinesOffset[b] = calcOutlineOffset(b);
		pointsOffset[b] = calcPointOffset(b);
		buff_inuse[b] = false;
		clear_lists(b);
	}

	noutlines_max = MMTT_OUTLINES_MAX;
	npoints_max = MMTT_POINTS_MAX;

	// These values need to be locked, when looked at or changed
	buff_being_constructed = BUFF_UNSET;
	buff_displayed_last_time = BUFF_UNSET;
	buff_to_display_next = BUFF_UNSET;

	lastUpdateTime = timeGetTime();
	active = 0;

	seqnum = -1;

}

int
Outlines_SharedMemHeader::addOutline(buff_index b,
int rid, int sid, float x, float y, float z, float area, int npoints) {

	NosuchAssert(b != BUFF_UNSET);

	int cnum = numoutlines[b];
	OutlineMem* om = outline(b, cnum);

	// NOTE: region id's in the shared memory are shifted down by 1
	om->region = rid - 1;
	om->sid = sid;
	om->x = x;
	om->y = y;
	om->z = z;
	om->area = area;
	om->npoints = npoints;
	om->index_of_firstpoint = numpoints[b];

	numoutlines[b]++;

	return cnum;
}

int
Outlines_SharedMemHeader::addPoint(buff_index b, float x, float y, float z) {
	NosuchAssert(b != BUFF_UNSET);
	int pnum = numpoints[b];
	PointMem* p = point(b, pnum);
	p->x = x;
	p->y = y;
	p->z = z;
	numpoints[b]++;
	return pnum;
}

void
Outlines_SharedMemHeader::clear_lists(buff_index buffnum) {
	NosuchAssert(buffnum != BUFF_UNSET);
	numoutlines[buffnum] = 0;
	numpoints[buffnum] = 0;
}

void
Outlines_SharedMemHeader::check_sanity() {
	int nused = 0;
	if (buff_inuse[0])
		nused++;
	if (buff_inuse[1])
		nused++;
	if (buff_inuse[2])
		nused++;

	bool ptr_used[3];
	ptr_used[0] = false;
	ptr_used[1] = false;
	ptr_used[2] = false;
	if (buff_being_constructed != BUFF_UNSET) {
		ptr_used[buff_being_constructed] = true;
	}
	if (buff_displayed_last_time != BUFF_UNSET) {
		ptr_used[buff_displayed_last_time] = true;
	}
	if (buff_to_display_next != BUFF_UNSET) {
		ptr_used[buff_to_display_next] = true;
	}

	int nptrused = 0;
	if (ptr_used[0])
		nptrused++;
	if (ptr_used[1])
		nptrused++;
	if (ptr_used[2])
		nptrused++;

	NosuchAssert(nused != nptrused);
}
MMTT_Mutex::MMTT_Mutex(const char *name)
{
#ifdef WIN32
	myMutex = CreateMutex(NULL, FALSE, s2ws(name).c_str());
#else
	assert(false);
#endif

}


MMTT_Mutex::~MMTT_Mutex()
{
#ifdef WIN32
	CloseHandle(myMutex);
#else
#endif 
}

bool
MMTT_Mutex::lock(int timeout)
{
#ifdef WIN32
	DWORD result = WaitForSingleObject(myMutex, timeout);
	if (result != WAIT_OBJECT_0)
		return false;
	else
		return true;
#else
	return false;
#endif 
}

bool
MMTT_Mutex::unlock()
{
#ifdef WIN32
	// Disable some warning about BOOL versus bool
#pragma warning(disable:4800)
	return (bool)ReleaseMutex(myMutex);
#else
	return false;
#endif 
}
