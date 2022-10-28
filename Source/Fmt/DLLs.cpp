#include "C_Stream.h"
#include "ROMFST.h"
#include "C_DataPack.h"
#include "BinUtils.h"
#include "C_HashMap.h"
#include "C_Utils.h"
#include "CL_Log.h"
#include "C_Hash.h"
#include "C_MemBlock.h"

namespace FormatsInternal
{
    struct DLLNameMapping
    {
        int mId;
        C_String mName;
    };

    static const char* sBankNames[] =
    {
        "core",
        "modgfx",
        "UNUSED",
        "projgfx",
        "zobj"
    };

    static const DLLNameMapping sDefaultNames[] =
    {
        { 1, "cmdmenu" },
        { 2, "camcontrol" },
        { 3, "ANIM" },
        { 4, "Race" },
        { 5, "AMSEQ" },
        { 6, "AMSFX" },
        { 7, "newday" },
        { 8, "newfog" },
        { 9, "newclouds" },
        { 10, "newstars" },
        { 11, "newlfx" },
        { 12, "minic" },
        { 13, "expgfx" },
        { 14, "modgfx" },
        { 15, "projgfx" },
        { 17, "partfx" },
        { 18, "objfsa" },
        { 19, "startgame" },
        { 20, "SCREEN" },
        { 21, "text" },
        { 22, "subtitles" },
        { 24, "waterfx" },
        { 25, "paths" },
        { 26, "CURVES" },
        { 28, "clrscr" },
        { 29, "gplay" },
        { 30, "tasktext" },
        { 31, "EEPROM" },
        { 32, "modelfx" },
        { 33, "baddieControl" },
        { 34, "partfx1" },
        { 35, "partfx2" },
        { 36, "dim_partfx" },
        { 37, "partfx3" },
        { 38, "nwa_partfx" },
        { 39, "swc_partfx" },
        { 40, "shp_partfx" },
        { 41, "clf_partfx" },
        { 42, "bay_partfx" },
        { 43, "bad_partfx" },
        { 44, "ice_partx" },
        { 45, "rex_partfx1" },
        { 46, "df_partfx" },
        { 47, "rex_partfx2" },
        { 48, "swh_partfx" },
        { 49, "dak_partfx" },
        { 50, "wc_partfx1" },
        { 51, "mmp_partfx" },
        { 52, "wc_partfx2" },
        { 54, "pickup" },
        { 56, "putdown" },
        { 60, "n_POST" },
        { 61, "n_rareware" },
        { 62, "n_mainmenu" },
        { 63, "n_gameselect" },
        { 64, "n_nameentry" },
        { 65, "n_options" },
        { 66, "n_pausmenu" },
        { 67, "n_gameover" },
        { 68, "old_titlescreen" },
        { 69, "old_menusys" },
        { 70, "old_levelselect" },
        { 71, "old_options" },
    };
    
    struct DLLNames
    {
        DLLNames()
        {
            for (int i = 0; i < WAR_ARRAY_SIZE(sDefaultNames); ++i)
                Set(sDefaultNames[i].mId, sDefaultNames[i].mName.c_str());
        }

        static DLLNames& GetInstance() { static DLLNames inst; return inst; }

        void Set(int aId, const char* aName)
        {
            DLLNameMapping& mapping = mNames.GetByHash(aId);
            mapping.mId = aId;
            mapping.mName = aName;
        }

        C_HashMap<int, DLLNameMapping> mNames;
    };

    bool ExportDLLs(FSTContext* aCtx)
    {
        C_Stream& handleTab = aCtx->GetFileStream(ROMFST::DLLS_TAB);
        C_Stream& handleBin = aCtx->GetFileStream(ROMFST::DLLS_BIN);

        const int numBanks = 4;
        int banks[numBanks];
        handleTab.ReadArray(banks, 4);

        C_Vector<int32> dllOffsets;
        C_Vector<int32> bssSizes;

        while (handleTab.IsEOF() == false)
        {
            int32 offs, bssSize;
            handleTab >> offs >> bssSize;

            if (offs == -1)
                break;

            dllOffsets.Add(offs);
            bssSizes.Add(bssSize);
        }

        C_FilePath outDir = aCtx->GetBaseDir();
        outDir.Combine("DLLS");
        C_FileSystem::DirectoryCreate(outDir);

        BinUtils::SplitFile(handleBin, dllOffsets, [aCtx, numBanks, &banks](int fileId, C_FilePath& outPath)
            {
                int bankId = 0;
                int localId = fileId;

                for (; bankId < numBanks; ++bankId)
                {
                    if (banks[bankId] == 0)
                        continue;

                    if (fileId < banks[bankId])
                        break;

                    localId = fileId - banks[bankId];
                }

                const DLLNameMapping* dllNameMap = DLLNames::GetInstance().mNames.FindByHash(fileId + 1);
                C_Strfmt<256> dllName("%s-%03i-%s.dll", sBankNames[bankId], localId, dllNameMap ? dllNameMap->mName.c_str() : "unk");

                outPath = aCtx->GetBaseDir();
                outPath.Combine("DLLS");
                outPath.Combine(dllName);
            }, [&bssSizes](int fileId, C_Stream& strm)
            {
                strm << bssSizes[fileId];
            });

        aCtx->MarkFileHandled(ROMFST::DLLS_BIN);
        aCtx->MarkFileHandled(ROMFST::DLLS_TAB);

        return true;
    }

    struct DllWriteEntry
    {
        C_String mFileName;
        uint32 mBssSize = 0;
        uint32 mBank = 0;
        uint32 mLocalId = 0;
        uint32 mOffset = 0;
    };

    bool CompileDLLs(FSTContext* aCtx)
    {
        C_FilePath dirPath = aCtx->GetBaseDir();
        dirPath.Combine("DLLS");

        C_Vector<C_String> dllFiles;
        if (!C_FileSystem::GetFilesInDirectory(dirPath, dllFiles))
            return false;

        char bankName[256];
        WAR_ZeroMem(bankName);
        char dllName[256];
        WAR_ZeroMem(dllName);

        const int numBanks = WAR_ARRAY_SIZE(sBankNames);
        uint32 bankHashes[numBanks];

        for (int i = 0; i < numBanks; ++i)
            bankHashes[i] = C_Hash(sBankNames[i]);

        C_Stream& handleTab = aCtx->GetFileStream(ROMFST::DLLS_TAB);
        C_Stream& handleBin = aCtx->GetFileStream(ROMFST::DLLS_BIN);

        C_Vector<DllWriteEntry> dllEntries;

        for (int i = 0; i < dllFiles.Count(); ++i)
        {
            if (C_StringUtils::EndsWith(".dll", dllFiles[i].c_str()) == false)
                continue;

            int dllLocalId = 0;

            if (3 != sscanf(dllFiles[i].c_str(), "%[^-]-%d-%[^.].dll", bankName, &dllLocalId, dllName))
            {
                WAR_LOG_ERROR(CAT_GENERAL, "Failed to parse DLL name: %s", dllFiles[i].c_str());
                return false;
            }

            uint32 bankIdHash = C_Hash((const char*)bankName);
            int bankId = -1;
            for (int j = 0; j < numBanks; ++j)
            {
                if (bankIdHash == bankHashes[j])
                {
                    bankId = j;
                    break;
                }
            }

            if (bankId == -1)
            {
                WAR_LOG_ERROR(CAT_GENERAL, "Failed to parse DLL name: %s, invalid bank name %s", dllFiles[i].c_str(), bankName);
                return false;
            }

            DllWriteEntry& entry = dllEntries.Add();
            entry.mFileName = dllFiles[i];
            entry.mBank = bankId;
            entry.mLocalId = dllLocalId;
        }

        dllEntries.Sort([](const DllWriteEntry& a, const DllWriteEntry& b)
            {
                uint32 keyA = (a.mBank << 16) | b.mLocalId;
                uint32 keyB = (b.mBank << 16) | b.mLocalId;
                return keyA < keyB;
            });

        uint32 dllId = 0;
        C_Vector<uint32> bankCounts;
        bankCounts.Resize(5, 0);

        for (int i = 0; i < dllEntries.Count(); ++i)
        {
            const DllWriteEntry& e = dllEntries[i];
            bankCounts[e.mBank] = ++dllId;
        }

        for (DllWriteEntry& e : dllEntries)
        {
            C_FilePath dllFilePath(dirPath);
            dllFilePath.Combine(e.mFileName.c_str());
            C_Ptr<C_MemBlock> dllFile = C_FileSystem::ReadFile(dllFilePath);
            if (!dllFile)
                return false;
            e.mOffset = handleBin.GetPosition();

            handleBin.WriteBytes(dllFile->mBlock, dllFile->mSize - 4);
            C_MemoryStream dllStrm(dllFile);
            dllStrm.SetEndianSwap(true);
            dllStrm.Seek(C_FileSystem::SeekEnd, 4);
            dllStrm >> e.mBssSize;
        }

        for (int i = 0; i < 4; ++i)
            handleTab << bankCounts[i];

        for (DllWriteEntry& e : dllEntries)
        {
            handleTab << e.mOffset << e.mBssSize;
        }

        handleTab << uint32(handleBin.GetPosition());
        handleTab << uint32(0);
        handleTab << int32(-1);
        handleTab << int32(-1);

        aCtx->MarkFileHandled(ROMFST::DLLS_BIN);
        aCtx->MarkFileHandled(ROMFST::DLLS_TAB);

        return true;
    }
}