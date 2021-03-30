#ifndef _BinUtils_h_
#define _BinUtils_h_

#include "C_Vector.h"
#include <functional>
#include "C_FilePath.h"

class C_Stream;

namespace BinUtils
{
    void ReadOffsets32(C_Stream& aHandle, C_Vector<int32>& aOut, int aStride = 4);

    void SplitFile(C_Stream& aHandle, C_Vector<int32>& aOffsetsAndEndSize, const function<void(int, C_FilePath&)>& aFmtFilenameFunc, const function<void(int, C_Stream&)>& aEndWriteFunc = {});
}


#endif // _BinUtils_h_
