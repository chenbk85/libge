// X86Info.cpp

#include <gepriv/X86Info.h>

#ifdef _MSC_VER
#include <intrin.h>
#include <Windows.h>
#endif

namespace X86Info
{
EXPORT bool x86info_hasCMPXCHG16B;
EXPORT bool x86info_hasLZCNT;
EXPORT bool x86info_hasMOVBE;
EXPORT bool x86info_hasPOPCNT;
EXPORT bool x86info_hasPCLMULQDQ;
EXPORT bool x86info_hasSSE;
EXPORT bool x86info_hasSSE2;
EXPORT bool x86info_hasSSE3;
EXPORT bool x86info_hasSupplementalSSE3;
EXPORT bool x86info_hasSSE4_1;
EXPORT bool x86info_hasSSE4_2;
EXPORT bool x86info_hasSSE4a;
EXPORT bool x86info_hasAVX;
EXPORT bool x86info_hasAES;


void initializeX86Info()
{
    int cpuInfo[4] = {-1};
    int nFeatureInfo = 0;

    unsigned int nIds, nExIds;

    ::__cpuid(cpuInfo, 0);
    nIds = cpuInfo[0];

    // Get the information associated with each valid Id
    ::__cpuid(cpuInfo, 1);

    // Interpret CPU feature information.
    x86info_hasSSE3 = (cpuInfo[2] & (1 << 0)) || false;
    x86info_hasPCLMULQDQ = (cpuInfo[2] & (1 << 1)) || false;
    x86info_hasSupplementalSSE3 = (cpuInfo[2] & (1 << 9)) || false;
    x86info_hasCMPXCHG16B = (cpuInfo[2] & (1 << 13)) || false;
    x86info_hasSSE4_1 = (cpuInfo[2] & (1 << 19)) || false;
    x86info_hasSSE4_2 = (cpuInfo[2] & (1 << 20)) || false;
    x86info_hasPOPCNT = (cpuInfo[2] & (1 << 23)) || false;
    x86info_hasAES = (cpuInfo[2] & (1 << 25)) || false;

    x86info_hasSSE = (cpuInfo[3] & (1 << 25)) || false;
    x86info_hasSSE2 = (cpuInfo[3] & (1 << 26)) || false;

    // AVX Requires Visual Studio 2010 SP1 or later
#if (_MSC_FULL_VER >= 160040219)
    // Checking for AVX requires 3 things:
    // 1) CPUID indicates the OS uses XSAVE and XRSTORE instructions (allowing
    //    saving YMM registers on context switch)
    // 2) CPUID indicates support for AVX
    // 3) XGETBV indicates the AVX registers will be saved/restored on
    //    context switch
    //
    // Note that XGETBV is only available on 686 or later CPUs, so the
    // instruction needs to be guarded.
    bool osUsesXSAVE_XRSTORE = cpuInfo[2] & (1 << 27) || false;
    bool hasAVX = cpuInfo[2] & (1 << 28) || false;

    if (osUsesXSAVE_XRSTORE && hasAVX)
    {
        // If this errors, you need VS2010 SP1 or later
        unsigned long long xcrFeatureMask = ::_xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        x86info_hasAVX = (xcrFeatureMask & 0x6) || false;
    }
    else
    {
        x86info_hasAVX = false;
    }
#endif

    nFeatureInfo = cpuInfo[3];

    // Running the CPUID instruction with 0x80000000 in EAX gets the highest
    // extended function supported.
    ::__cpuid(cpuInfo, 0x80000000);
    nExIds = cpuInfo[0];

    // Get the information associated with each extended ID.
    // TODO: Remove loop
    for (uint32 i = 0x80000000; i <= nExIds; i++)
    {
        ::__cpuid(cpuInfo, i);

        if  (i == 0x80000001)
        {
            x86info_hasLZCNT = (cpuInfo[2] & 0x20) || false;
            x86info_hasSSE4a = (cpuInfo[2] & 0x40) || false;
        }
    }
}

} // End namespace X86Info
