#include "C_Stream.h"
#include "ROMFST.h"
#include "C_DataPack.h"
#include "BinUtils.h"
#include "C_Utils.h"
#include "CL_Log.h"
#include "C_FileSystem.h"
#include "C_MemBlock.h"

namespace FormatsInternal
{
    bool ExportMODELS(FSTContext* aCtx)
    {
        C_Stream& handleTab = aCtx->GetFileStream(ROMFST::MODELS_TAB);
        C_Stream& handleBin = aCtx->GetFileStream(ROMFST::MODELS_BIN);

        C_Vector<int32> offsets;

        while (handleTab.IsEOF() == false)
        {
            int32 offs;
            handleTab >> offs;
            offsets.Add(offs);
        }

        C_FilePath outDir = aCtx->GetBaseDir();
        outDir.Combine("MODELS");
        C_FileSystem::DirectoryCreate(outDir);

        for (int i = 0; i < offsets.Count(); ++i)
        {
            int32 offs = offsets[i];

            if (offs == -1)
                break;

            int32 sizeComp = offsets[i + 1] - offs;
        }

        BinUtils::SplitFile(handleBin, offsets, [aCtx](int fileId, C_FilePath& outPath)
            {
                C_Strfmt<256> name("%04i.bin", fileId);

                outPath = aCtx->GetBaseDir();
                outPath.Combine("MODELS");
                outPath.Combine(name);
            });

        aCtx->MarkFileHandled(ROMFST::MODELS_TAB);
        aCtx->MarkFileHandled(ROMFST::MODELS_BIN);

        return true;
    }

    bool CompileMODELS(FSTContext* aCtx)
    {
        C_FilePath dirPath = aCtx->GetBaseDir();
        dirPath.Combine("MODELS");

        C_Vector<C_String> files;
        if (!C_FileSystem::GetFilesInDirectory(dirPath, files))
            return false;

        C_Stream& handleTab = aCtx->GetFileStream(ROMFST::MODELS_TAB);
        C_Stream& handleBin = aCtx->GetFileStream(ROMFST::MODELS_BIN);

        struct Entry
        {
            C_String fname;
            uint32 offset;
            int id;
        };

        C_Vector<Entry> entries;

        for (int i = 0; i < files.Count(); ++i)
        {
            if (C_StringUtils::EndsWith(".mp3", files[i].c_str()) == false)
                continue;

            int mp3Id = 0;

            if (1 != sscanf(files[i].c_str(), "%d.mp3", &mp3Id))
            {
                WAR_LOG_ERROR(CAT_GENERAL, "Failed to parse MP# name: %s", files[i].c_str());
                return false;
            }

            Entry& entry = entries.Add();
            entry.id = mp3Id;
            entry.fname = files[i];
        }

        entries.Sort([](const Entry& a, const Entry& b)
            {
                return a.id < b.id;
            });

        for (Entry& e : entries)
        {
            C_FilePath filePath(dirPath);
            filePath.Combine(e.fname.c_str());
            C_Ptr<C_MemBlock> file = C_FileSystem::ReadFile(filePath);
            if (!file)
                return false;
            e.offset = handleBin.GetPosition();
            handleBin.WriteBytes(file->mBlock, file->mSize - 4);
        }

        for (Entry& e : entries)
        {
            handleTab << e.offset;
        }

        handleTab << uint32(handleBin.GetPosition());

        aCtx->MarkFileHandled(ROMFST::MODELS_TAB);
        aCtx->MarkFileHandled(ROMFST::MODELS_BIN);

        return true;
    }
}