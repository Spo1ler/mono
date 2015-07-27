#ifndef CORGC_UTILS_H_
#define CORGC_UTILS_H_

#ifndef _MSC_VER
#define CORGC_UTILS_IMPORT
#else
#define CORGC_UTILS_IMPORT __declspec(dllimport)
#endif /*_MSC_VER*/

#include <mono/io-layer/uglify.h>

#ifdef TARGET_X86
#define CORGC_UTILS_API __stdcall
#else
#define CORGC_UTILS_API
#endif /*TARGET_x86*/

#define FATAL_ASSERT(e, msg)                                            \
        do                                                              \
                {                                                       \
                        if (!(e))                                       \
                                {                                       \
                                        fprintf(stderr, "FATAL ERROR: " msg); \
                                        abort();                        \
                                }                                       \
                }                                                       \
        while(0)

#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

CORGC_UTILS_IMPORT VOID CORGC_UTILS_API FlushProcessWriteBuffers();

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*CORGC_UTILS_H_ */
