#include "BinUtils.h"
#include "C_Stream.h"
#include "C_MemBlock.h"

namespace BinUtils_private
{
    template<typename T>
    void ReadOffsets(C_Stream& aHandle, C_Vector<T>& aOut, int aStride)
    {
        while (aHandle.IsEOF() == false)
        {
            T val = aHandle.Get<T>(0);

            if (val == -1)
                break;

            aHandle += aStride;

            aOut.Add(val);
        }
    }
}

void BinUtils::ReadOffsets32(C_Stream& aHandle, C_Vector<int32>& aOut, int aStride /*= 4*/)
{
    BinUtils_private::ReadOffsets<int32>(aHandle, aOut, aStride);
}

void BinUtils::SplitFile(C_Stream& aHandle, C_Vector<int32>& aOffsetsAndEndSize, const std::function<void(int, C_FilePath&)>& aFmtFilenameFunc, const std::function<void(int, C_Stream&)>& aEndWriteFunc)
{
    C_FilePath pathBuff;

    C_Ptr<C_MemBlock> buffer = WAR_MemBlockAlloc(aHandle.GetLength());

    for (int i = 0; i < aOffsetsAndEndSize.Count() - 1; ++i)
    {
        const int32 offset = aOffsetsAndEndSize[i];
        const int32 size = aOffsetsAndEndSize[i + 1] - offset;

        aFmtFilenameFunc(i, pathBuff);

        aHandle.Seek(C_FileSystem::SeekSet, offset);
        aHandle.ReadBytes(buffer->mBlock, size);

        C_FileHandle ohandle;
        if (!C_FileSystem::Open(ohandle, pathBuff, C_FileSystem::FileWriteDiscard))
        {
            WAR_CHECK(false);
            return;
        }

        C_Stream ostrm(ohandle, false);
        ostrm.SetEndianSwap(true);
        ostrm.WriteBytes(buffer->mBlock, size);

        if (aEndWriteFunc)
            aEndWriteFunc(i, ostrm);

        C_FileSystem::Close(ohandle);
    }
}

