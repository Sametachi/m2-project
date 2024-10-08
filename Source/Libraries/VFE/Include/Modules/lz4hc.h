
#ifndef LZ4_HC_H_19834876238432
#define LZ4_HC_H_19834876238432

#if defined (__cplusplus)
extern "C" {
#endif



#include "lz4.h"   



#define LZ4HC_CLEVEL_MIN         3
#define LZ4HC_CLEVEL_DEFAULT     9
#define LZ4HC_CLEVEL_OPT_MIN    10
#define LZ4HC_CLEVEL_MAX        12




LZ4LIB_API int LZ4_compress_HC (const char* src, char* dst, int srcSize, int dstCapacity, int compressionLevel);






LZ4LIB_API int LZ4_sizeofStateHC(void);
LZ4LIB_API int LZ4_compress_HC_extStateHC(void* stateHC, const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel);



LZ4LIB_API int LZ4_compress_HC_destSize(void* stateHC,
                                  const char* src, char* dst,
                                        int* srcSizePtr, int targetDstSize,
                                        int compressionLevel);



 typedef union LZ4_streamHC_u LZ4_streamHC_t;   


LZ4LIB_API LZ4_streamHC_t* LZ4_createStreamHC(void);
LZ4LIB_API int             LZ4_freeStreamHC (LZ4_streamHC_t* streamHCPtr);



LZ4LIB_API void LZ4_resetStreamHC_fast(LZ4_streamHC_t* streamHCPtr, int compressionLevel);   
LZ4LIB_API int  LZ4_loadDictHC (LZ4_streamHC_t* streamHCPtr, const char* dictionary, int dictSize);

LZ4LIB_API int LZ4_compress_HC_continue (LZ4_streamHC_t* streamHCPtr,
                                   const char* src, char* dst,
                                         int srcSize, int maxDstSize);


LZ4LIB_API int LZ4_compress_HC_continue_destSize(LZ4_streamHC_t* LZ4_streamHCPtr,
                                           const char* src, char* dst,
                                                 int* srcSizePtr, int targetDstSize);

LZ4LIB_API int LZ4_saveDictHC (LZ4_streamHC_t* streamHCPtr, char* safeBuffer, int maxDictSize);







#define LZ4HC_DICTIONARY_LOGSIZE 16
#define LZ4HC_MAXD (1<<LZ4HC_DICTIONARY_LOGSIZE)
#define LZ4HC_MAXD_MASK (LZ4HC_MAXD - 1)

#define LZ4HC_HASH_LOG 15
#define LZ4HC_HASHTABLESIZE (1 << LZ4HC_HASH_LOG)
#define LZ4HC_HASH_MASK (LZ4HC_HASHTABLESIZE - 1)


#if defined(__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) )
#include <stdint.h>

typedef struct LZ4HC_CCtx_internal LZ4HC_CCtx_internal;
struct LZ4HC_CCtx_internal
{
    uint32_t   hashTable[LZ4HC_HASHTABLESIZE];
    uint16_t   chainTable[LZ4HC_MAXD];
    const uint8_t* end;         
    const uint8_t* base;        
    const uint8_t* dictBase;    
    uint32_t   dictLimit;       
    uint32_t   lowLimit;        
    uint32_t   nextToUpdate;    
    short      compressionLevel;
    int8_t     favorDecSpeed;   
    int8_t     dirty;           
    const LZ4HC_CCtx_internal* dictCtx;
};

#else

typedef struct LZ4HC_CCtx_internal LZ4HC_CCtx_internal;
struct LZ4HC_CCtx_internal
{
    unsigned int   hashTable[LZ4HC_HASHTABLESIZE];
    unsigned short chainTable[LZ4HC_MAXD];
    const unsigned char* end;        
    const unsigned char* base;       
    const unsigned char* dictBase;   
    unsigned int   dictLimit;        
    unsigned int   lowLimit;         
    unsigned int   nextToUpdate;     
    short          compressionLevel;
    char           favorDecSpeed;    
    char           dirty;            
    const LZ4HC_CCtx_internal* dictCtx;
};

#endif



#define LZ4_STREAMHCSIZE       (4*LZ4HC_HASHTABLESIZE + 2*LZ4HC_MAXD + 56 + ((sizeof(void*)==16) ? 56 : 0)  ) 
#define LZ4_STREAMHCSIZE_SIZET (LZ4_STREAMHCSIZE / sizeof(size_t))
union LZ4_streamHC_u {
    size_t table[LZ4_STREAMHCSIZE_SIZET];
    LZ4HC_CCtx_internal internal_donotuse;
}; 




LZ4LIB_API LZ4_streamHC_t* LZ4_initStreamHC (void* buffer, size_t size);






LZ4_DEPRECATED("use LZ4_compress_HC() instead") LZ4LIB_API int LZ4_compressHC               (const char* source, char* dest, int inputSize);
LZ4_DEPRECATED("use LZ4_compress_HC() instead") LZ4LIB_API int LZ4_compressHC_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);
LZ4_DEPRECATED("use LZ4_compress_HC() instead") LZ4LIB_API int LZ4_compressHC2              (const char* source, char* dest, int inputSize, int compressionLevel);
LZ4_DEPRECATED("use LZ4_compress_HC() instead") LZ4LIB_API int LZ4_compressHC2_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);
LZ4_DEPRECATED("use LZ4_compress_HC_extStateHC() instead") LZ4LIB_API int LZ4_compressHC_withStateHC               (void* state, const char* source, char* dest, int inputSize);
LZ4_DEPRECATED("use LZ4_compress_HC_extStateHC() instead") LZ4LIB_API int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);
LZ4_DEPRECATED("use LZ4_compress_HC_extStateHC() instead") LZ4LIB_API int LZ4_compressHC2_withStateHC              (void* state, const char* source, char* dest, int inputSize, int compressionLevel);
LZ4_DEPRECATED("use LZ4_compress_HC_extStateHC() instead") LZ4LIB_API int LZ4_compressHC2_limitedOutput_withStateHC(void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);
LZ4_DEPRECATED("use LZ4_compress_HC_continue() instead") LZ4LIB_API int LZ4_compressHC_continue               (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize);
LZ4_DEPRECATED("use LZ4_compress_HC_continue() instead") LZ4LIB_API int LZ4_compressHC_limitedOutput_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize, int maxOutputSize);


LZ4_DEPRECATED("use LZ4_createStreamHC() instead") LZ4LIB_API void* LZ4_createHC (const char* inputBuffer);
LZ4_DEPRECATED("use LZ4_saveDictHC() instead") LZ4LIB_API     char* LZ4_slideInputBufferHC (void* LZ4HC_Data);
LZ4_DEPRECATED("use LZ4_freeStreamHC() instead") LZ4LIB_API   int   LZ4_freeHC (void* LZ4HC_Data);
LZ4_DEPRECATED("use LZ4_compress_HC_continue() instead") LZ4LIB_API int LZ4_compressHC2_continue               (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel);
LZ4_DEPRECATED("use LZ4_compress_HC_continue() instead") LZ4LIB_API int LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);
LZ4_DEPRECATED("use LZ4_createStreamHC() instead") LZ4LIB_API int   LZ4_sizeofStreamStateHC(void);
LZ4_DEPRECATED("use LZ4_initStreamHC() instead") LZ4LIB_API  int   LZ4_resetStreamStateHC(void* state, char* inputBuffer);



LZ4LIB_API void LZ4_resetStreamHC (LZ4_streamHC_t* streamHCPtr, int compressionLevel);


#if defined (__cplusplus)
}
#endif

#endif 



#ifdef LZ4_HC_STATIC_LINKING_ONLY   
#ifndef LZ4_HC_SLO_098092834
#define LZ4_HC_SLO_098092834

#define LZ4_STATIC_LINKING_ONLY   
#include "lz4.h"

#if defined (__cplusplus)
extern "C" {
#endif


LZ4LIB_STATIC_API void LZ4_setCompressionLevel(
    LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel);


LZ4LIB_STATIC_API void LZ4_favorDecompressionSpeed(
    LZ4_streamHC_t* LZ4_streamHCPtr, int favor);


LZ4LIB_STATIC_API void LZ4_resetStreamHC_fast(
    LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel);


LZ4LIB_STATIC_API int LZ4_compress_HC_extStateHC_fastReset (
    void* state,
    const char* src, char* dst,
    int srcSize, int dstCapacity,
    int compressionLevel);


LZ4LIB_STATIC_API void LZ4_attach_HC_dictionary(
          LZ4_streamHC_t *working_stream,
    const LZ4_streamHC_t *dictionary_stream);

#if defined (__cplusplus)
}
#endif

#endif   
#endif   
