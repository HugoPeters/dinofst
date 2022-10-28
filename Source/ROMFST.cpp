#include "ROMFST.h"
#include "C_FileSystem.h"
#include "C_MemBlock.h"
#include "C_Stream.h"
#include "CL_Log.h"
#include "C_FilePath.h"
#include "C_DataPack.h"
#include "Formats.h"
#include "C_OS.h"
#include "n64crc.h"
#include "C_Math.h"

#define ROM_FST_OFFSET 0xA4970

namespace ROMFST_private
{
    struct FileInfo
    {
        const char* mName;
    };

    static const FileInfo sFileInfo[ROMFST::NUM_FILES] =
    {
        { "AUDIO.tab" },
        { "AUDIO.bin" },
        { "SFX.tab" },
        { "SFX.bin" },
        { "AMBIENT.tab" },
        { "AMBIENT.bin" },
        { "MUSIC.tab" },
        { "MUSIC.bin" },
        { "MPEG.tab" },
        { "MPEG.bin" },
        { "MUSICACTIONS.bin" },
        { "CAMACTIONS.bin" },
        { "LACTIONS.bin" },
        { "ANIMCURVES.bin" },
        { "ANIMCURVES.tab" },
        { "OBJSEQ2CURVE.tab" },
        { "FONTS.bin" },
        { "CACHEFON.bin" },
        { "CACHEFON2.bin" },
        { "GAMETEXT.bin" },
        { "GAMETEXT.tab" },
        { "GLOBALMAP.bin" },
        { "TABLES.bin" },
        { "TABLES.tab" },
        { "SCREENS.bin" },
        { "SCREENS.tab" },
        { "VOXMAP.bin" },
        { "VOXMAP.tab" },
        { "TEXPRE.tab" },
        { "TEXPRE.bin" },
        { "WARPTAB.bin" },
        { "MAPS.bin" },
        { "MAPS.tab" },
        { "MAPINFO.bin" },
        { "MAPSETUP.ind" },
        { "MAPSETUP.tab" },
        { "TEX1.bin" },
        { "TEX1.tab" },
        { "TEXTABLE.bin" },
        { "TEX0.bin" },
        { "TEX0.tab" },
        { "BLOCKS.bin" },
        { "BLOCKS.tab" },
        { "TRKBLK.bin" },
        { "HITS.bin" },
        { "HITS.tab" },
        { "MODELS.tab" },
        { "MODELS.bin" },
        { "MODELIND.bin" },
        { "MODANIM.tab" },
        { "MODANIM.bin" },
        { "ANIM.tab" },
        { "ANIM.bin" },
        { "AMAP.tab" },
        { "AMAP.bin" },
        { "BITTABLE.bin" },
        { "WEAPONDATA.bin" },
        { "VOXOBJ.tab" },
        { "VOXOBJ.bin" },
        { "MODLINES.bin" },
        { "MODLINES.tab" },
        { "SAVEGAME.bin" },
        { "SAVEGAME.tab" },
        { "OBJSEQ.bin" },
        { "OBJSEQ.tab" },
        { "OBJECTS.tab" },
        { "OBJECTS.bin" },
        { "OBJINDEX.bin" },
        { "OBJEVENT.bin" },
        { "OBJHITS.bin" },
        { "DLLS.bin" },
        { "DLLS.tab" },
        { "DLLSIMPORTTAB.bin" },
        { "ENVFXACT.bin" }
    };

    struct FSTLegend
    {
        C_Vector<C_String> mFileNames;

        void FromFSTInfo(const ROMFST::FSTInfo& aInfo)
        {
            for (int i = 0; i < aInfo.mFileOffsets.Count() - 1; ++i)
            {
                C_Strfmt<64> fileName = aInfo.GetFileName(i);
                mFileNames.Add(fileName.GetBuffer());
            }
        }

        void ToJson(const char* aPath)
        {
            C_DataPack pack;

            for (int i = 0; i < mFileNames.Count(); ++i)
            {
                pack.Set(i, mFileNames[i]);
            }

            pack.ToFileJson(aPath);
        }
    };

    class ROMFSTExtractContext : public FSTContext
    {
    public:
        struct CachedFile
        {
            C_Ptr<C_MemBlock> mBlob;
            C_MemoryStream mStream;
        };

        C_Stream& GetFileStream(ROMFST::File aFile) override
        {
            if (!mStreams[aFile].mBlob)
            {
                CachedFile& file = mStreams[aFile];
                file.mBlob = WAR_MemBlockAlloc(mFstInfo->GetFileSize(aFile));
                mFileStream->Seek(C_FileSystem::SeekSet, mFstInfo->GetAbsoluteFileOffset(aFile));
                mFileStream->ReadBytes(file.mBlob->mBlock, file.mBlob->mSize);
                file.mStream = C_MemoryStream(file.mBlob);
                file.mStream.SetEndianSwap(true);
            }

            return mStreams[aFile].mStream;
        }

        CachedFile mStreams[ROMFST::NUM_FILES];
        C_Stream* mFileStream = NULL;
        const ROMFST::FSTInfo* mFstInfo = NULL;
    };

    class FSTWriteContext : public FSTContext
    {
    public:
        C_Stream& GetFileStream(ROMFST::File aFile) override
        {
            WAR_ASSERT(IsFileHandled(aFile) == false, "File has already been written!");

            C_FilePath fullPath(mOutputDir);
            fullPath.Combine(ROMFST_private::sFileInfo[aFile].mName);

            C_FileHandle handle;
            bool ok = C_FileSystem::Open(handle, fullPath, C_FileSystem::FileWriteDiscard);

            WAR_ASSERT(ok, "Failed to open file: %s", fullPath);

            mStreams[aFile] = new C_Stream(handle, true);
            mStreams[aFile]->SetEndianSwap(true);

            return *mStreams[aFile];
        }

        void MarkFileHandled(int aFileType) override
        {
            mStreams[aFileType] = NULL;
            FSTContext::MarkFileHandled(aFileType);
        }

        C_FilePath mOutputDir;
        C_Ptr<C_Stream> mStreams[ROMFST::NUM_FILES];
    };
}

bool ROMFST::DumpBin(const char* aInPath, const char* aBinPath)
{
    using namespace ROMFST_private;

    C_FileHandle handle;
    if (!C_FileSystem::Open(handle, aInPath, C_FileSystem::FileReadOnly))
        return false;

    C_Stream strm(handle);
    strm.SetEndianSwap(true);

    FSTInfo info;
    if (!info.ReadROM(strm))
        return false;

    uint32 fstSize = info.GetSizeFull();

    C_Ptr<C_MemBlock> fst = WAR_MemBlockAlloc(fstSize);
    
    strm.Seek(C_FileSystem::SeekSet, ROM_FST_OFFSET);
    strm.ReadBytes(fst->mBlock, int(fstSize));

    WAR_LOG_INFO(CAT_GENERAL, "Write %s...", aBinPath);
    C_FileSystem::WriteFile(aBinPath, fst->mBlock, fst->mSize);
    return true;
}

bool ROMFST::DumpFiles(const char* aInPath, const char* aOutDir)
{
    using namespace ROMFST_private;

    if (!C_FileSystem::DirectoryExists(aOutDir))
    {
        WAR_LOG_ERROR(CAT_GENERAL, "Output directory is invalid: %s", aOutDir);
        return false;
    }

    C_FileHandle handle;
    if (!C_FileSystem::Open(handle, aInPath, C_FileSystem::FileReadOnly))
        return false;

    C_Stream strm(handle);
    strm.SetEndianSwap(true);

    FSTInfo info;
    if (!info.ReadROM(strm))
        return false;

    FSTLegend legend;
    legend.FromFSTInfo(info);

    C_Ptr<C_MemBlock> buffer = WAR_MemBlockAlloc(info.GetSizeFull());

    for (int i = 0; i < info.mFileOffsets.Count() - 1; ++i)
    {
        const uint32 size = info.GetFileSize(i);

        strm.Seek(C_FileSystem::SeekSet, info.GetAbsoluteFileOffset(i));
        strm.ReadBytes(buffer->mBlock, size);

        C_FilePath outPath(aOutDir);
        outPath.Combine(legend.mFileNames[i].c_str());

        C_FileSystem::WriteFile(outPath, buffer->mBlock, size);
    }

    C_FilePath legendFile(aOutDir);
    legendFile.Combine("fst.json");
    legend.ToJson(legendFile);
}

bool ROMFST::ExtractFiles(const char* aInPath, const char* aOutDir, const char* aDefsPath)
{
    using namespace ROMFST_private;

    if (!C_FileSystem::DirectoryExists(aOutDir))
    {
        WAR_LOG_ERROR(CAT_GENERAL, "Output directory is invalid: %s", aOutDir);
        return false;
    }

    C_FileHandle handle;
    if (!C_FileSystem::Open(handle, aInPath, C_FileSystem::FileReadOnly))
        return false;

    C_Stream strm(handle);
    strm.SetEndianSwap(true);

    FSTInfo info;
    if (!info.ReadROM(strm))
        return false;

    ROMFSTExtractContext ctx;
    ctx.Init(C_FilePath(aOutDir));
    ctx.mFileStream = &strm;
    ctx.mFstInfo = &info;
    ctx.mDefsPath = aDefsPath;

    for (int i = 0; i < C_Min(info.NumFiles(), (int)Formats::NUM_FMTS); ++i)
    {
        const FormatInfo& fmtInfo = Formats::GetFormatInfo(i);

        printf("%s...\n", fmtInfo.mName.c_str());

        if (!fmtInfo.mExportFunc(&ctx))
            return false;
    }

    printf("Misc...\n");

    // export unhandled files as raw files
    C_Ptr<C_MemBlock> buffer = WAR_MemBlockAlloc(info.GetSizeFull());

    for (int i = 0; i < info.NumFiles(); ++i)
    {
        if (ctx.IsFileHandled(i) == false)
        {
            const uint32 size = info.GetFileSize(i);

            strm.Seek(C_FileSystem::SeekSet, info.GetAbsoluteFileOffset(i));
            strm.ReadBytes(buffer->mBlock, size);

            C_FilePath outPath(aOutDir);
            outPath.Combine(info.GetFileName(i));

            C_FileSystem::WriteFile(outPath, buffer->mBlock, size);
        }
    }
}

bool ROMFST::CompileFiles(const char* aInPath, const char* aOutDir)
{
    using namespace ROMFST_private;

    if (!C_FileSystem::DirectoryExists(aInPath))
    {
        WAR_LOG_ERROR(CAT_GENERAL, "Input directory is invalid: %s", aInPath);
        return false;
    }

    if (!C_FileSystem::DirectoryExists(aOutDir))
    {
        WAR_LOG_ERROR(CAT_GENERAL, "Output directory is invalid: %s", aOutDir);
        return false;
    }

    FSTWriteContext ctx;
    ctx.Init(C_FilePath(aInPath));
    ctx.mOutputDir = aOutDir;

    for (int i = 0; i < Formats::NUM_FMTS; ++i)
    {
        const FormatInfo& fmtInfo = Formats::GetFormatInfo(i);

        printf("%s...\n", fmtInfo.mName.c_str());

        if (!fmtInfo.mCompileFunc(&ctx))
            return false;
    }

    printf("Misc...\n");

    for (int i = 0; i < ROMFST::NUM_FILES; ++i)
    {
        if (ctx.IsFileHandled(i) == false)
        {
            C_FilePath dstPath(aOutDir);
            dstPath.Combine(ROMFST_private::sFileInfo[i].mName);

            C_FilePath srcPath(aInPath);
            srcPath.Combine(ROMFST_private::sFileInfo[i].mName);

            if (C_FileSystem::Exists(srcPath) == false)
            {
                WAR_LOG_ERROR(CAT_GENERAL, "File doesn't exist: %s", srcPath);
                return false;
            }

            C_FileSystem::Copy(srcPath, dstPath);
        }
    }

    return true;
}

bool ROMFST::CompileFST(const char* aInPath, const char* aOutPath)
{
    using namespace ROMFST_private;

    C_FileHandle oh;
    if (!C_FileSystem::Open(oh, aOutPath, C_FileSystem::FileWriteDiscard))
        return false;

    C_Stream handle(oh);
    handle.SetEndianSwap(true);

    FSTInfo fst;
    fst.InitDefault();

    // reserve toc
    fst.Write(handle);

    for (int i = 0; i < ROMFST::NUM_FILES; ++i)
    {
        C_FilePath fpath(aInPath);
        fpath.Combine(sFileInfo[i].mName);

        if (!C_FileSystem::Exists(fpath))
        {
            WAR_LOG_ERROR(CAT_GENERAL, "Required FST file not found: %s", fpath);
            return false;
        }

        C_Ptr<C_MemBlock> fileBuff = C_FileSystem::ReadFile(fpath);

        if (!fileBuff)
        {
            WAR_LOG_ERROR(CAT_GENERAL, "Required FST file failed to read: %s", fpath);
            return false;
        }

        fst.mFileOffsets[i] = handle.GetPosition();
        handle.WriteBytes(fileBuff->mBlock, fileBuff->mSize);
    }

    fst.mFileOffsets[fst.mFileOffsets.Count() - 1] = handle.GetPosition();

    // actual toc
    handle.Seek(C_FileSystem::SeekSet, 0);
    fst.FileOffsetsFinalize();
    fst.Write(handle);

    return true;
}

bool ROMFST::InjectFST(const char* aRomPath, const char* aInPath, const char* aOutPath)
{
    C_Ptr<C_MemBlock> baseRom = C_FileSystem::ReadFile(aRomPath);

    if (!baseRom)
        return false;

    if (baseRom->mSize < ROM_FST_OFFSET)
    {
        WAR_LOG_ERROR(CAT_GENERAL, "Invalid source rom");
        return false;
    }

    C_Ptr<C_MemBlock> fst = C_FileSystem::ReadFile(aInPath);

    if (!fst)
        return false;

    const uint32 padSize = 1024 * 1024 * 64;
    const uint32 unpaddedRomSize = ROM_FST_OFFSET + fst->mSize;

    uint32 paddedRomSize = unpaddedRomSize;

    if (unpaddedRomSize < padSize)
        paddedRomSize = padSize;

    C_Ptr<C_MemBlock> newRom = WAR_MemBlockAlloc(paddedRomSize);

    if (unpaddedRomSize < padSize)
        WAR_ZeroMem((uint8*)newRom->mBlock + unpaddedRomSize, padSize - unpaddedRomSize);

    memcpy(newRom->mBlock, baseRom->mBlock, ROM_FST_OFFSET);
    memcpy((uint8*)newRom->mBlock + ROM_FST_OFFSET, fst->mBlock, fst->mSize);

    N64CRC::UpdateCRC(newRom->mBlock, newRom->mSize);

    return C_FileSystem::WriteFile(aOutPath, newRom->mBlock, newRom->mSize);
}

bool ROMFST::CompileROM(const char* aRomPath, const char* aInPath, const char* aOutPath)
{
    C_FilePath tempDir(C_OS::GetInstance()->GetWorkingDirectory());
    tempDir.Combine("temp");
    C_FileSystem::DirectoryCreate(tempDir);

    C_FilePath tempFilesDir(tempDir);
    tempFilesDir.Combine("ofst");
    C_FileSystem::DirectoryCreate(tempFilesDir);

    WAR_LOG_INFO(CAT_GENERAL, "Compile files...");
    if (!CompileFiles(aInPath, tempFilesDir))
        return false;

    C_FilePath tempFstPath(tempDir);
    tempFstPath.Combine("fst.bin");

    WAR_LOG_INFO(CAT_GENERAL, "Compile fst.bin...");
    if (!CompileFST(tempFilesDir, tempFstPath))
        return false;

    WAR_LOG_INFO(CAT_GENERAL, "Compile rom...");
    return InjectFST(aRomPath, tempFstPath, aOutPath);
}

bool FSTContext::ReadJson(C_DataPack& aPack, const char* aRelFileName)
{
    C_FilePath fullPath(mBaseDir);
    fullPath.Combine(aRelFileName);

    if (!C_FileSystem::Exists(fullPath))
    {
        WAR_LOG_ERROR(CAT_GENERAL, "File doesn't exist: %s", fullPath.GetBuffer());
        return false;
    }

    return aPack.FromFileJson(fullPath);
}

bool FSTContext::WriteJson(const C_DataPack& aPack, const char* aRelFileName)
{
    C_FilePath path;
    FixFilePath(aRelFileName, path);
    return aPack.ToFileJson(path);
}

void FSTContext::FixFilePath(const char* aRelFileName, C_FilePath& aOut)
{
    C_FilePath fullPath(mBaseDir);
    fullPath.Combine(aRelFileName);

    C_FilePath dirPath;
    C_PathUtils::GetDirectoryPath(fullPath, dirPath);

    C_FileSystem::DirectoryCreate(dirPath);

    aOut = fullPath;
}

uint32 ROMFST::FSTInfo::GetFSTOffset()
{
	return ROM_FST_OFFSET;
}

void ROMFST::FSTInfo::InitDefault()
{
	mFileOffsets.Resize(ROMFST::NUM_FILES + 1);
}

void ROMFST::FSTInfo::FileOffsetsFinalize()
{
	uint32 hdrSize = 4 + (mFileOffsets.Count() * 4);

	for (int i = 0; i < mFileOffsets.Count(); ++i)
	{
		mFileOffsets[i] = mFileOffsets[i] - hdrSize;
	}
}

uint32 ROMFST::FSTInfo::GetSizeFull() const
{
	uint32 size = mFileOffsets.Count() * 4;
	size += mFileOffsets[mFileOffsets.Count() - 1];
	return size;
}

uint32 ROMFST::FSTInfo::GetAbsoluteFileOffset(int idx) const
{
	return mContentOffset + mFileOffsets[idx];
}

uint32 ROMFST::FSTInfo::GetFileSize(int idx) const
{
	return mFileOffsets[idx + 1] - mFileOffsets[idx];
}

bool ROMFST::FSTInfo::Read(C_Stream& handle)
{
	mFileOffsets.Clear();

	int numFiles;
	handle >> numFiles;

	if (numFiles <= 0 || numFiles > 0xFFF)
	{
		WAR_LOG_WARNING(CAT_GENERAL, "Insane number of files in FST: %i, stopping", numFiles);
		return false;
	}

	if (numFiles != ROMFST::NUM_FILES)
	{
		WAR_LOG_WARNING(CAT_GENERAL, "Unexpected number of files in FST: %i, expected %i", numFiles, ROMFST::NUM_FILES);
	}

	handle.ReadArray(mFileOffsets, numFiles + 1); // +1 for FST size

	mContentOffset = handle.GetPosition();
}

bool ROMFST::FSTInfo::ReadROM(C_Stream& handle)
{
	handle.Seek(C_FileSystem::SeekSet, ROM_FST_OFFSET);
	return Read(handle);
}

C_Strfmt<64> ROMFST::FSTInfo::GetFileName(int idx) const
{
	using namespace ROMFST_private;

	const FileInfo* finfo = (idx < ROMFST::NUM_FILES) ? &sFileInfo[idx] : NULL;

	C_Strfmt<64> fileName;

	if (finfo)
		fileName.Format("%s", finfo->mName);
	else
		fileName.Format("UNKFILE_%i.bin", idx);

	return fileName;
}

void ROMFST::FSTInfo::WriteJson(const char* aOutPath)
{
	C_DataPack pack;

	pack.ToFileJson(aOutPath);
}

void ROMFST::FSTInfo::Write(C_Stream& handle)
{
	using namespace ROMFST_private;

	handle << uint32(mFileOffsets.Count() - 1);
	handle.WriteArray(mFileOffsets.GetBuffer(), mFileOffsets.Count());
}
