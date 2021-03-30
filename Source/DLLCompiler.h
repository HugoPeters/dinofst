#ifndef _DLLCompiler_h_
#define _DLLCompiler_h_

namespace DLLCompiler
{
    bool ConvertELFtoDLL(const char* aELFPath, const char* aDLLPath, const char* aDefsPath);
}

#endif // _DLLCompiler_h_
