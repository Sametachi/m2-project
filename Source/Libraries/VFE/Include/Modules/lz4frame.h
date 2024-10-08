



#ifndef LZ4F_H_09782039843
#define LZ4F_H_09782039843

#if defined (__cplusplus)
extern "C" {
#endif


#include <stddef.h>   






#if defined(LZ4_DLL_EXPORT) && (LZ4_DLL_EXPORT==1)
#  define LZ4FLIB_API __declspec(dllexport)
#elif defined(LZ4_DLL_IMPORT) && (LZ4_DLL_IMPORT==1)
#  define LZ4FLIB_API __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#  define LZ4FLIB_API __attribute__ ((__visibility__ ("default")))
#else
#  define LZ4FLIB_API
#endif

#ifdef LZ4F_DISABLE_DEPRECATE_WARNINGS
#  define LZ4F_DEPRECATE(x) x
#else
#  if defined(_MSC_VER)
#    define LZ4F_DEPRECATE(x) x   
#  elif defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 6))
#    define LZ4F_DEPRECATE(x) x __attribute__((deprecated))
#  else
#    define LZ4F_DEPRECATE(x) x   
#  endif
#endif



typedef size_t LZ4F_errorCode_t;

LZ4FLIB_API unsigned    LZ4F_isError(LZ4F_errorCode_t code);   
LZ4FLIB_API const char* LZ4F_getErrorName(LZ4F_errorCode_t code);   




#ifdef LZ4F_ENABLE_OBSOLETE_ENUMS
#  define LZ4F_OBSOLETE_ENUM(x) , LZ4F_DEPRECATE(x) = LZ4F_##x
#else
#  define LZ4F_OBSOLETE_ENUM(x)
#endif


typedef enum {
    LZ4F_default=0,
    LZ4F_max64KB=4,
    LZ4F_max256KB=5,
    LZ4F_max1MB=6,
    LZ4F_max4MB=7
    LZ4F_OBSOLETE_ENUM(max64KB)
    LZ4F_OBSOLETE_ENUM(max256KB)
    LZ4F_OBSOLETE_ENUM(max1MB)
    LZ4F_OBSOLETE_ENUM(max4MB)
} LZ4F_blockSizeID_t;


typedef enum {
    LZ4F_blockLinked=0,
    LZ4F_blockIndependent
    LZ4F_OBSOLETE_ENUM(blockLinked)
    LZ4F_OBSOLETE_ENUM(blockIndependent)
} LZ4F_blockMode_t;

typedef enum {
    LZ4F_noContentChecksum=0,
    LZ4F_contentChecksumEnabled
    LZ4F_OBSOLETE_ENUM(noContentChecksum)
    LZ4F_OBSOLETE_ENUM(contentChecksumEnabled)
} LZ4F_contentChecksum_t;

typedef enum {
    LZ4F_noBlockChecksum=0,
    LZ4F_blockChecksumEnabled
} LZ4F_blockChecksum_t;

typedef enum {
    LZ4F_frame=0,
    LZ4F_skippableFrame
    LZ4F_OBSOLETE_ENUM(skippableFrame)
} LZ4F_frameType_t;

#ifdef LZ4F_ENABLE_OBSOLETE_ENUMS
typedef LZ4F_blockSizeID_t blockSizeID_t;
typedef LZ4F_blockMode_t blockMode_t;
typedef LZ4F_frameType_t frameType_t;
typedef LZ4F_contentChecksum_t contentChecksum_t;
#endif


typedef struct {
  LZ4F_blockSizeID_t     blockSizeID;         
  LZ4F_blockMode_t       blockMode;           
  LZ4F_contentChecksum_t contentChecksumFlag; 
  LZ4F_frameType_t       frameType;           
  unsigned long long     contentSize;         
  unsigned               dictID;              
  LZ4F_blockChecksum_t   blockChecksumFlag;   
} LZ4F_frameInfo_t;

#define LZ4F_INIT_FRAMEINFO   { LZ4F_default, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0ULL, 0U, LZ4F_noBlockChecksum }    


typedef struct {
  LZ4F_frameInfo_t frameInfo;
  int      compressionLevel;    
  unsigned autoFlush;           
  unsigned favorDecSpeed;         
  unsigned reserved[3];         
} LZ4F_preferences_t;

#define LZ4F_INIT_PREFERENCES   { LZ4F_INIT_FRAMEINFO, 0, 0u, 0u, { 0u, 0u, 0u } }    




LZ4FLIB_API int LZ4F_compressionLevel_max(void);   


LZ4FLIB_API size_t LZ4F_compressFrameBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr);


LZ4FLIB_API size_t LZ4F_compressFrame(void* dstBuffer, size_t dstCapacity,
                                const void* srcBuffer, size_t srcSize,
                                const LZ4F_preferences_t* preferencesPtr);



typedef struct LZ4F_cctx_s LZ4F_cctx;   
typedef LZ4F_cctx* LZ4F_compressionContext_t;   

typedef struct {
  unsigned stableSrc;    
  unsigned reserved[3];
} LZ4F_compressOptions_t;



#define LZ4F_VERSION 100    
LZ4FLIB_API unsigned LZ4F_getVersion(void);


LZ4FLIB_API LZ4F_errorCode_t LZ4F_createCompressionContext(LZ4F_cctx** cctxPtr, unsigned version);
LZ4FLIB_API LZ4F_errorCode_t LZ4F_freeCompressionContext(LZ4F_cctx* cctx);




#define LZ4F_HEADER_SIZE_MIN  7   
#define LZ4F_HEADER_SIZE_MAX 19


#define LZ4F_BLOCK_HEADER_SIZE 4


#define LZ4F_BLOCK_CHECKSUM_SIZE 4


#define LZ4F_CONTENT_CHECKSUM_SIZE 4


LZ4FLIB_API size_t LZ4F_compressBegin(LZ4F_cctx* cctx,
                                      void* dstBuffer, size_t dstCapacity,
                                      const LZ4F_preferences_t* prefsPtr);


LZ4FLIB_API size_t LZ4F_compressBound(size_t srcSize, const LZ4F_preferences_t* prefsPtr);


LZ4FLIB_API size_t LZ4F_compressUpdate(LZ4F_cctx* cctx,
                                       void* dstBuffer, size_t dstCapacity,
                                 const void* srcBuffer, size_t srcSize,
                                 const LZ4F_compressOptions_t* cOptPtr);


LZ4FLIB_API size_t LZ4F_flush(LZ4F_cctx* cctx,
                              void* dstBuffer, size_t dstCapacity,
                        const LZ4F_compressOptions_t* cOptPtr);


LZ4FLIB_API size_t LZ4F_compressEnd(LZ4F_cctx* cctx,
                                    void* dstBuffer, size_t dstCapacity,
                              const LZ4F_compressOptions_t* cOptPtr);



typedef struct LZ4F_dctx_s LZ4F_dctx;   
typedef LZ4F_dctx* LZ4F_decompressionContext_t;   

typedef struct {
  unsigned stableDst;    
  unsigned reserved[3];  
} LZ4F_decompressOptions_t;





LZ4FLIB_API LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_dctx** dctxPtr, unsigned version);
LZ4FLIB_API LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_dctx* dctx);




#define LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH 5


size_t LZ4F_headerSize(const void* src, size_t srcSize);


LZ4FLIB_API size_t LZ4F_getFrameInfo(LZ4F_dctx* dctx,
                                     LZ4F_frameInfo_t* frameInfoPtr,
                                     const void* srcBuffer, size_t* srcSizePtr);


LZ4FLIB_API size_t LZ4F_decompress(LZ4F_dctx* dctx,
                                   void* dstBuffer, size_t* dstSizePtr,
                                   const void* srcBuffer, size_t* srcSizePtr,
                                   const LZ4F_decompressOptions_t* dOptPtr);



LZ4FLIB_API void LZ4F_resetDecompressionContext(LZ4F_dctx* dctx);   



#if defined (__cplusplus)
}
#endif

#endif  

#if defined(LZ4F_STATIC_LINKING_ONLY) && !defined(LZ4F_H_STATIC_09782039843)
#define LZ4F_H_STATIC_09782039843

#if defined (__cplusplus)
extern "C" {
#endif


#ifdef LZ4F_PUBLISH_STATIC_FUNCTIONS
#define LZ4FLIB_STATIC_API LZ4FLIB_API
#else
#define LZ4FLIB_STATIC_API
#endif



#define LZ4F_LIST_ERRORS(ITEM) \
        ITEM(OK_NoError) \
        ITEM(ERROR_GENERIC) \
        ITEM(ERROR_maxBlockSize_invalid) \
        ITEM(ERROR_blockMode_invalid) \
        ITEM(ERROR_contentChecksumFlag_invalid) \
        ITEM(ERROR_compressionLevel_invalid) \
        ITEM(ERROR_headerVersion_wrong) \
        ITEM(ERROR_blockChecksum_invalid) \
        ITEM(ERROR_reservedFlag_set) \
        ITEM(ERROR_allocation_failed) \
        ITEM(ERROR_srcSize_tooLarge) \
        ITEM(ERROR_dstMaxSize_tooSmall) \
        ITEM(ERROR_frameHeader_incomplete) \
        ITEM(ERROR_frameType_unknown) \
        ITEM(ERROR_frameSize_wrong) \
        ITEM(ERROR_srcPtr_wrong) \
        ITEM(ERROR_decompressionFailed) \
        ITEM(ERROR_headerChecksum_invalid) \
        ITEM(ERROR_contentChecksum_invalid) \
        ITEM(ERROR_frameDecoding_alreadyStarted) \
        ITEM(ERROR_maxCode)

#define LZ4F_GENERATE_ENUM(ENUM) LZ4F_##ENUM,


typedef enum { LZ4F_LIST_ERRORS(LZ4F_GENERATE_ENUM)
              _LZ4F_dummy_error_enum_for_c89_never_used } LZ4F_errorCodes;

LZ4FLIB_STATIC_API LZ4F_errorCodes LZ4F_getErrorCode(size_t functionResult);

LZ4FLIB_STATIC_API size_t LZ4F_getBlockSize(unsigned);




typedef struct LZ4F_CDict_s LZ4F_CDict;


LZ4FLIB_STATIC_API LZ4F_CDict* LZ4F_createCDict(const void* dictBuffer, size_t dictSize);
LZ4FLIB_STATIC_API void        LZ4F_freeCDict(LZ4F_CDict* CDict);



LZ4FLIB_STATIC_API size_t LZ4F_compressFrame_usingCDict(
    LZ4F_cctx* cctx,
    void* dst, size_t dstCapacity,
    const void* src, size_t srcSize,
    const LZ4F_CDict* cdict,
    const LZ4F_preferences_t* preferencesPtr);



LZ4FLIB_STATIC_API size_t LZ4F_compressBegin_usingCDict(
    LZ4F_cctx* cctx,
    void* dstBuffer, size_t dstCapacity,
    const LZ4F_CDict* cdict,
    const LZ4F_preferences_t* prefsPtr);



LZ4FLIB_STATIC_API size_t LZ4F_decompress_usingDict(
    LZ4F_dctx* dctxPtr,
    void* dstBuffer, size_t* dstSizePtr,
    const void* srcBuffer, size_t* srcSizePtr,
    const void* dict, size_t dictSize,
    const LZ4F_decompressOptions_t* decompressOptionsPtr);

#if defined (__cplusplus)
}
#endif

#endif  
