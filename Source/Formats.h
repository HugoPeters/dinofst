#ifndef _Formats_h_
#define _Formats_h_

class FSTContext;

typedef bool(*FSTHandleFunc)(FSTContext* aCtx);

struct FormatInfo
{
    FormatInfo(FSTHandleFunc aExportFunc, FSTHandleFunc aCompileFunc)
        : mExportFunc(aExportFunc)
        , mCompileFunc(aCompileFunc)
    {}

    FSTHandleFunc mExportFunc;
    FSTHandleFunc mCompileFunc;
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

        NUM_FMTS
    };

    const FormatInfo& GetFormatInfo(int aType);
}


#endif // _Formats_h_
