#ifndef _ROMFST_h_
#define _ROMFST_h_

#include "C_FilePath.h"

class C_Stream;
class C_DataPack;

namespace ROMFST
{
    enum File
    {
        AUDIO_TAB,
        AUDIO_BIN,
        SFX_TAB,
        SFX_BIN,
        AMBIENT_TAB,
        AMBIENT_BIN,
        MUSIC_TAB,
        MUSIC_BIN,
        MPEG_TAB,
        MPEG_BIN,
        MUSICACTIONS_BIN,
        CAMACTIONS_BIN,
        LACTIONS_BIN,
        ANIMCURVES_BIN,
        ANIMCURVES_TAB,
        OBJSEQ2CURVE_TAB,
        FONTS_BIN,
        CACHEFON_BIN,
        CACHEFON2_BIN,
        GAMETEXT_BIN,
        GAMETEXT_TAB,
        GLOBALMAP_BIN,
        TABLES_BIN,
        TABLES_TAB,
        SCREENS_BIN,
        SCREENS_TAB,
        VOXMAP_BIN,
        VOXMAP_TAB,
        TEXPRE_TAB,
        TEXPRE_BIN,
        WARPTAB_BIN,
        MAPS_BIN,
        MAPS_TAB,
        MAPINFO_BIN,
        MAPSETUP_IND,
        MAPSETUP_TAB,
        TEX1_BIN,
        TEX1_TAB,
        TEXTABLE_BIN,
        TEX0_BIN,
        TEX0_TAB,
        BLOCKS_BIN,
        BLOCKS_TAB,
        TRKBLK_BIN,
        HITS_BIN,
        HITS_TAB,
        MODELS_TAB,
        MODELS_BIN,
        MODELIND_BIN,
        MODANIM_TAB,
        MODANIM_BIN,
        ANIM_TAB,
        ANIM_BIN,
        AMAP_TAB,
        AMAP_BIN,
        BITTABLE_BIN,
        WEAPONDATA_BIN,
        VOXOBJ_TAB,
        VOXOBJ_BIN,
        MODLINES_BIN,
        MODLINES_TAB,
        SAVEGAME_BIN,
        SAVEGAME_TAB,
        OBJSEQ_BIN,
        OBJSEQ_TAB,
        OBJECTS_TAB,
        OBJECTS_BIN,
        OBJINDEX_BIN,
        OBJEVENT_BIN,
        OBJHITS_BIN,
        DLLS_BIN,
        DLLS_TAB,
        DLLSIMPORTTAB_BIN,
        ENVFXACT_BIN,

        NUM_FILES
    };

    // extract fst.bin from rom
    bool DumpBin(const char* aInPath, const char* aBinPath);

    bool DumpFiles(const char* aInPath, const char* aOutDir);

    bool ExtractFiles(const char* aInPath, const char* aOutDir, const char* aDefsPath);

    bool CompileFiles(const char* aInPath, const char* aOutDir);

    bool CompileFST(const char* aInPath, const char* aOutPath);

    bool InjectFST(const char* aRomPath, const char* aInPath, const char* aOutPath);

    bool CompileROM(const char* aRomPath, const char* aInPath, const char* aOutPath);
};

class FSTContext
{
public:
    void Init(const C_FilePath& aBasePath)
    {
        mBaseDir = aBasePath;
    }

    bool ReadJson(C_DataPack& aPack, const char* aRelFileName);
    bool WriteJson(const C_DataPack& aPack, const char* aRelFileName);
    void FixFilePath(const char* aRelFileName, C_FilePath& aOut);
    bool IsFileHandled(int aFileType) const { return mHandledFlags[aFileType]; }
    const C_FilePath& GetBaseDir() const { return mBaseDir; }

    virtual void MarkFileHandled(int aFileType) { mHandledFlags[aFileType] = true; }
    virtual C_Stream& GetFileStream(ROMFST::File aFile) = 0;

    string mDefsPath;

protected:
    C_FilePath mBaseDir;
    bool mHandledFlags[ROMFST::NUM_FILES] = { false };
};

#endif // _ROMFST_h_
