#ifndef _ModInfo_h_
#define _ModInfo_h_

#include "C_String.h"
#include "C_GUID.h"

struct ModMetadata
{
	C_String mTitle;
	C_String mDescription;
	C_String mAuthor;
	C_GUID mUID;
	int mVersion = 0;
};

enum ModType
{
	// simple binary changes to game data without changing file sizes
	MODTYPE_SIMPLE,

	// advanced changes to files that require FST rebuild
	MODTYPE_FST
};

struct ModInfo
{
public:
	ModMetadata mMeta;
	ModType mType;
};


#endif // _ModInfo_h_
