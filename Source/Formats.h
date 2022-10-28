#ifndef _Formats_h_
#define _Formats_h_

#include "C_String.h"

class FSTContext;

typedef bool(*FSTHandleFunc)(FSTContext* aCtx);

struct FormatInfo
{
    FormatInfo(FSTHandleFunc aExportFunc, FSTHandleFunc aCompileFunc, const char* aName)
        : mExportFunc(aExportFunc)
        , mCompileFunc(aCompileFunc)
        , mName(aName)
    {}

    FSTHandleFunc mExportFunc;
    FSTHandleFunc mCompileFunc;
    C_String mName;
};

namespace Formats
{
    enum Type
    {
        //AMAP,
        //AMBIENT,
        //ANIM,
        //ANIMCURVES,
        //AUDIO,
        //BITTABLE,
        //BLOCKS,
        //CACHEFON,
        //CACHEFON2,
        //CAMACTIONS,
        DLLS,
        DLLSIMPORTTAB,
        //ENVFXACT,
        //FONTS,
        //GAMETEXT,
        GLOBALMAP,
        //HITS,
        //LACTIONS,
        MAPINFO,
        //MAPS,
        //MAPSETUP,
        //MODANIM,
        //MPEG,
        MODELS,
		MODELIND,

        NUM_FMTS
    };

    const FormatInfo& GetFormatInfo(int aType);
}


#endif // _Formats_h_
