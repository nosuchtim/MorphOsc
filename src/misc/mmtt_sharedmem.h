/*
* Some of this code is derived from sample code in Touch Designer, for
* sharing textures in shared memory.
*/

#pragma once

using namespace TUIO;

#ifdef WIN32
#include <windows.h>
typedef HANDLE shmId;
typedef HANDLE mutexId;
#else
typedef int shmId;
typedef int mutexId;
#endif

#ifndef DLLEXP
#define DLLEXP
#endif 

class DLLEXP MMTT_Mutex
{
public:
	MMTT_Mutex(const char *name);
	~MMTT_Mutex();

	bool       lock(int timeout);
	bool       unlock();

	// This class is distributed to the users, so make sure it doesn't
	// rely on any internal Touch classes

private:
	mutexId       myMutex;
};


#define MMTT_SHM_INFO_MAGIC_NUMBER 0x56ed34ba

#define MMTT_SHM_INFO_DECORATION "4jhd783h"

#define MMTT_SHM_MAX_POST_FIX_SIZE 32

typedef enum
{
	MMTT_SHM_ERR_NONE = 0,
	MMTT_SHM_ERR_ALREADY_EXIST,
	MMTT_SHM_ERR_DOESNT_EXIST,
	MMTT_SHM_ERR_INFO_ALREADY_EXIST,
	MMTT_SHM_ERR_INFO_DOESNT_EXIST,
	MMTT_SHM_ERR_UNABLE_TO_MAP,
} MMTT_SharedMemError;

// This is an internal class used by MMTT_SharedMem to handle
// resizing and closing of a memory segment
class MMTT_SharedMemInfo
{
public:
	int magicNumber;
	int version;
	bool supported;
	char namePostFix[MMTT_SHM_MAX_POST_FIX_SIZE];

	// version 2
	bool	detach;
};

class DLLEXP MMTT_SharedMem
{
public:

	// Use this if you are the SENDER
	// size is in bytes
	MMTT_SharedMem(const char *name, unsigned int size);

	// Use this one if tyou are the RECEIVER, you won't know the size of the
	// memory segment
	MMTT_SharedMem(const char *name);


	~MMTT_SharedMem();

	// Get the size of the shared memory, only the SENDER will know the size
	// the RECEIVER will always get a value of 0
	unsigned int getSize() const
	{
		return mySize;
	}
	void		resize(unsigned int size);

	bool		lock();
	bool		unlock();


	//
	// detach will disassociate the shared memory segment from this process.
	// It returns true if the associated memory mapping has been freed.
	// It returns false if the memory mapping is still around after detaching,
	// this usually means someone else still has the mapping open.
	//
	bool		detach();

	//
	//  getMemory will return a pointer to the shared memory segment.
	//
	void		*getMemory();

	char		*getName()
	{
		return myShortName;
	}

	MMTT_SharedMemError getErrorState()
	{
		return myErrorState;
	}


private:

	// Internal constructor, not for external use
	MMTT_SharedMem(const char *name, unsigned int size, bool supportInfo);

	bool		 open(const char* name, unsigned int size = 0, bool supportInfo = true);

	bool		 detachInternal();
	bool		 checkInfo();
	void		 randomizePostFix();
	void		 createName();

	bool		 createSharedMem();
	bool		 openSharedMem();

	bool		 createInfo();

	char		*myShortName;
	char		*myName;
	char		 myNamePostFix[MMTT_SHM_MAX_POST_FIX_SIZE];
	unsigned int mySize;
	void		*myMemory;

	shmId		 	myMapping;
	MMTT_Mutex		*myMutex;
	MMTT_SharedMem	*mySharedMemInfo;
	MMTT_SharedMemError	myErrorState;

	bool				myAmOwner;
	bool				mySupportInfo;

};

#define MMTT_SHM_MAGIC_NUMBER 	0xe95df674
// Version 2 got rid of cursors in favor of outlines
#define MMTT_SHM_VERSION_NUMBER 	3
#define MMTT_CURSORS_MAX 100
#define MMTT_OUTLINES_MAX 1000
#define MMTT_POINTS_MAX 100000

class CBlob;

// If you add new members to this after it's released, add them after dataOffset
class Image_SharedMemHeader {

public:
	/* Begin version 1 */
	// Magic number to make sure we are looking at the correct memory
	// must be set to MMTT_SHM_MAGIC_NUMBER
	int							magicNumber;

	// version number of this header, must be set to MMTT_SHM_VERSION_NUMBER
	int							version;

	// image width
	int							width;

	// image height
	int							height;

	// X aspect of the image
	float						aspectx;

	// Y aspect of the image
	float						aspecty;

	// Format of the image data in memory (RGB, RGBA, BGR, BGRA etc.)
	int							dataFormat;

	// The data type of the image data in memory (unsigned char, float)
	int							dataType;

	// The desired pixel format of the texture to be created in Touch (RGBA8, RGBA16, RGBA32 etc.)
	int							pixelFormat;

	// This offset (in bytes) is the diffrence between the start of this header,
	// and the start of the image data
	// The SENDER is required to set this. Unless you are doing something custom
	// you should set this to calcOffset();
	// If you are the RECEIVER, don't change this value.
	int							dataOffset;

	/* End version 1 */


	// Both the sender and the reciever can use this to get the pointer to the actual
	// image data (as long as dataOffset is set beforehand).
	void						*getImage()
	{
		char *c = (char*)this;
		c += dataOffset;
		return (void*)c;
	}


	int							calcDataOffset()
	{
		return sizeof(Image_SharedMemHeader);
	}

};

typedef struct PointMem {
	float x;
	float y;
	float z;
} PointMem;

typedef struct OutlineMem {
	int region;
	int sid;
	float x;
	float y;
	float z;
	float area;
	int npoints;
	int index_of_firstpoint;
} OutlineMem;

#define BUFF_UNSET (-1)
#define NUM_BUFFS 3
typedef int buff_index;

class Outlines_SharedMemHeader
{
public:
	// Magic number to make sure we are looking at the correct memory
	// must be set to MMTT_SHM_MAGIC_NUMBER
	int							magicNumber;

	// version number of this header, must be set to MMTT_SHM_VERSION_NUMBER
	int							version;

	int							noutlines_max;
	int							npoints_max;

	// These are the values that, whenever they are looked at or changed,
	// need to be locked. //////////////////////////////////////////////////
	buff_index		buff_being_constructed; //  -1, 0, 1, 2
	buff_index		buff_displayed_last_time; //  -1, 0, 1, 2
	buff_index		buff_to_display_next; //  -1, 0, 1, 2
	buff_index		buff_to_display;
	bool			buff_inuse[3];
	////////////////////////////////////////////////////////////////////////

	int							numoutlines[3];
	int							numpoints[3];

	// This offset (in bytes) is the distance from the start of the data.
	// WARNING: do not re-order these fields, the calc.* methods depend on it.
	int							outlinesOffset[3];
	int							pointsOffset[3];

	int	seqnum;
	long lastUpdateTime;  // directly from timeGetTime()
	int active;

	// END OF DATA, the rest is method declarations

	char *Data() {
		return (char*)this + sizeof(Outlines_SharedMemHeader);
	}

	OutlineMem* outline(int buffnum, int outlinenum) {
		int offset = calcOutlineOffset(buffnum, outlinenum);
		OutlineMem* om = (OutlineMem*)(Data() + offset);
		return om;
	}
	PointMem* point(int buffnum, int pointnum) {
		return (PointMem*)(Data() + calcPointOffset(buffnum, pointnum));
	}

	int	calcOutlineOffset(int buffnum, int outlinenum = 0) {
		int v1 = 0;
		int v2 = buffnum*noutlines_max*sizeof(OutlineMem);
		int v3 = outlinenum*sizeof(OutlineMem);
		return v1 + v2 + v3;
	}
	int	calcPointOffset(int buffnum, int pointnum = 0) {
		return calcOutlineOffset(0) + NUM_BUFFS*noutlines_max*sizeof(OutlineMem)
			+ buffnum*npoints_max*sizeof(PointMem)
			+ pointnum*sizeof(PointMem);
	}

	int addPoint(buff_index b, float x, float y, float z);
	int addOutline(buff_index b, int region, int sid, float x, float y, float z, float area, int npoints);

	void xinit();
	void clear_lists(buff_index b);
	void check_sanity();
	buff_index grab_unused_buffer();
};

void print_buff_info(char *prefix, Outlines_SharedMemHeader* h);