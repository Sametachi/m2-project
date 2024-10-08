#include <StdAfx.hpp>
#ifndef LZ4HC_HEAPMODE
#  define LZ4HC_HEAPMODE 1
#endif



#define LZ4_HC_STATIC_LINKING_ONLY
#include <Modules/lz4hc.h>



#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
#if defined (__clang__)
#  pragma clang diagnostic ignored "-Wunused-function"
#endif


typedef enum { noDictCtx, usingDictCtxHc } dictCtx_directive;


#define LZ4_COMMONDEFS_ONLY
#ifndef LZ4_SRC_INCLUDED
#include "lz4.cpp"
#endif

#define OPTIMAL_ML (int)((ML_MASK-1)+MINMATCH)
#define LZ4_OPT_NUM   (1<<12)



#define MIN(a,b)   ( (a) < (b) ? (a) : (b) )
#define MAX(a,b)   ( (a) > (b) ? (a) : (b) )
#define HASH_FUNCTION(i)         (((i) * 2654435761U) >> ((MINMATCH*8)-LZ4HC_HASH_LOG))
#define DELTANEXTMAXD(p)         chainTable[(p) & LZ4HC_MAXD_MASK]    
#define DELTANEXTU16(table, pos) table[(U16)(pos)]   

#define UPDATABLE(ip, op, anchor) &ip, &op, &anchor

static U32 LZ4HC_hashPtr(const void* ptr) { return HASH_FUNCTION(LZ4_read32(ptr)); }



static void LZ4HC_clearTables (LZ4HC_CCtx_internal* hc4)
{
    MEM_INIT((void*)hc4->hashTable, 0, sizeof(hc4->hashTable));
    MEM_INIT(hc4->chainTable, 0xFF, sizeof(hc4->chainTable));
}

static void LZ4HC_init_internal (LZ4HC_CCtx_internal* hc4, const BYTE* start)
{
    uptrval startingOffset = (uptrval)(hc4->end - hc4->base);
    if (startingOffset > 1 GB) {
        LZ4HC_clearTables(hc4);
        startingOffset = 0;
    }
    startingOffset += 64 KB;
    hc4->nextToUpdate = (U32) startingOffset;
    hc4->base = start - startingOffset;
    hc4->end = start;
    hc4->dictBase = start - startingOffset;
    hc4->dictLimit = (U32) startingOffset;
    hc4->lowLimit = (U32) startingOffset;
}



LZ4_FORCE_INLINE void LZ4HC_Insert (LZ4HC_CCtx_internal* hc4, const BYTE* ip)
{
    U16* const chainTable = hc4->chainTable;
    U32* const hashTable  = hc4->hashTable;
    const BYTE* const base = hc4->base;
    U32 const target = (U32)(ip - base);
    U32 idx = hc4->nextToUpdate;

    while (idx < target) {
        U32 const h = LZ4HC_hashPtr(base+idx);
        size_t delta = idx - hashTable[h];
        if (delta>LZ4_DISTANCE_MAX) delta = LZ4_DISTANCE_MAX;
        DELTANEXTU16(chainTable, idx) = (U16)delta;
        hashTable[h] = idx;
        idx++;
    }

    hc4->nextToUpdate = target;
}


LZ4_FORCE_INLINE
int LZ4HC_countBack(const BYTE* const ip, const BYTE* const match,
                    const BYTE* const iMin, const BYTE* const mMin)
{
    int back = 0;
    int const min = (int)MAX(iMin - ip, mMin - match);
    assert(min <= 0);
    assert(ip >= iMin); assert((size_t)(ip-iMin) < (1U<<31));
    assert(match >= mMin); assert((size_t)(match - mMin) < (1U<<31));
    while ( (back > min)
         && (ip[back-1] == match[back-1]) )
            back--;
    return back;
}


static unsigned
LZ4HC_countPattern(const BYTE* ip, const BYTE* const iEnd, U32 const pattern32)
{
    const BYTE* const iStart = ip;
    reg_t const pattern = (sizeof(pattern)==8) ? (reg_t)pattern32 + (((reg_t)pattern32) << 32) : pattern32;

    while (likely(ip < iEnd-(sizeof(pattern)-1))) {
        reg_t const diff = LZ4_read_ARCH(ip) ^ pattern;
        if (!diff) { ip+=sizeof(pattern); continue; }
        ip += LZ4_NbCommonBytes(diff);
        return (unsigned)(ip - iStart);
    }

    if (LZ4_isLittleEndian()) {
        reg_t patternByte = pattern;
        while ((ip<iEnd) && (*ip == (BYTE)patternByte)) {
            ip++; patternByte >>= 8;
        }
    } else {  
        U32 bitOffset = (sizeof(pattern)*8) - 8;
        while (ip < iEnd) {
            BYTE const byte = (BYTE)(pattern >> bitOffset);
            if (*ip != byte) break;
            ip ++; bitOffset -= 8;
        }
    }

    return (unsigned)(ip - iStart);
}


static unsigned
LZ4HC_reverseCountPattern(const BYTE* ip, const BYTE* const iLow, U32 pattern)
{
    const BYTE* const iStart = ip;

    while (likely(ip >= iLow+4)) {
        if (LZ4_read32(ip-4) != pattern) break;
        ip -= 4;
    }
    {   const BYTE* bytePtr = (const BYTE*)(&pattern) + 3; 
        while (likely(ip>iLow)) {
            if (ip[-1] != *bytePtr) break;
            ip--; bytePtr--;
    }   }
    return (unsigned)(iStart - ip);
}

typedef enum { rep_untested, rep_not, rep_confirmed } repeat_state_e;
typedef enum { favorCompressionRatio=0, favorDecompressionSpeed } HCfavor_e;

LZ4_FORCE_INLINE int
LZ4HC_InsertAndGetWiderMatch (
    LZ4HC_CCtx_internal* hc4,
    const BYTE* const ip,
    const BYTE* const iLowLimit,
    const BYTE* const iHighLimit,
    int longest,
    const BYTE** matchpos,
    const BYTE** startpos,
    const int maxNbAttempts,
    const int patternAnalysis,
    const int chainSwap,
    const dictCtx_directive dict,
    const HCfavor_e favorDecSpeed)
{
    U16* const chainTable = hc4->chainTable;
    U32* const HashTable = hc4->hashTable;
    const LZ4HC_CCtx_internal * const dictCtx = hc4->dictCtx;
    const BYTE* const base = hc4->base;
    const U32 dictLimit = hc4->dictLimit;
    const BYTE* const lowPrefixPtr = base + dictLimit;
    const U32 ipIndex = (U32)(ip - base);
    const U32 lowestMatchIndex = (hc4->lowLimit + 64 KB > ipIndex) ? hc4->lowLimit : ipIndex - LZ4_DISTANCE_MAX;
    const BYTE* const dictBase = hc4->dictBase;
    int const lookBackLength = (int)(ip-iLowLimit);
    int nbAttempts = maxNbAttempts;
    U32 matchChainPos = 0;
    U32 const pattern = LZ4_read32(ip);
    U32 matchIndex;
    repeat_state_e repeat = rep_untested;
    size_t srcPatternLength = 0;

    DEBUGLOG(7, "LZ4HC_InsertAndGetWiderMatch");
    
    LZ4HC_Insert(hc4, ip);
    matchIndex = HashTable[LZ4HC_hashPtr(ip)];
    DEBUGLOG(7, "First match at index %u / %u (lowestMatchIndex)",
                matchIndex, lowestMatchIndex);

    while ((matchIndex>=lowestMatchIndex) && (nbAttempts)) {
        int matchLength=0;
        nbAttempts--;
        assert(matchIndex < ipIndex);
        if (favorDecSpeed && (ipIndex - matchIndex < 8)) {
            
        } else if (matchIndex >= dictLimit) {   
            const BYTE* const matchPtr = base + matchIndex;
            assert(matchPtr >= lowPrefixPtr);
            assert(matchPtr < ip);
            assert(longest >= 1);
            if (LZ4_read16(iLowLimit + longest - 1) == LZ4_read16(matchPtr - lookBackLength + longest - 1)) {
                if (LZ4_read32(matchPtr) == pattern) {
                    int const back = lookBackLength ? LZ4HC_countBack(ip, matchPtr, iLowLimit, lowPrefixPtr) : 0;
                    matchLength = MINMATCH + (int)LZ4_count(ip+MINMATCH, matchPtr+MINMATCH, iHighLimit);
                    matchLength -= back;
                    if (matchLength > longest) {
                        longest = matchLength;
                        *matchpos = matchPtr + back;
                        *startpos = ip + back;
            }   }   }
        } else {   
            const BYTE* const matchPtr = dictBase + matchIndex;
            if (LZ4_read32(matchPtr) == pattern) {
                const BYTE* const dictStart = dictBase + hc4->lowLimit;
                int back = 0;
                const BYTE* vLimit = ip + (dictLimit - matchIndex);
                if (vLimit > iHighLimit) vLimit = iHighLimit;
                matchLength = (int)LZ4_count(ip+MINMATCH, matchPtr+MINMATCH, vLimit) + MINMATCH;
                if ((ip+matchLength == vLimit) && (vLimit < iHighLimit))
                    matchLength += LZ4_count(ip+matchLength, lowPrefixPtr, iHighLimit);
                back = lookBackLength ? LZ4HC_countBack(ip, matchPtr, iLowLimit, dictStart) : 0;
                matchLength -= back;
                if (matchLength > longest) {
                    longest = matchLength;
                    *matchpos = base + matchIndex + back;   
                    *startpos = ip + back;
        }   }   }

        if (chainSwap && matchLength==longest) {    
            assert(lookBackLength==0);   
            if (matchIndex + (U32)longest <= ipIndex) {
                U32 distanceToNextMatch = 1;
                int pos;
                for (pos = 0; pos <= longest - MINMATCH; pos++) {
                    U32 const candidateDist = DELTANEXTU16(chainTable, matchIndex + (U32)pos);
                    if (candidateDist > distanceToNextMatch) {
                        distanceToNextMatch = candidateDist;
                        matchChainPos = (U32)pos;
                }   }
                if (distanceToNextMatch > 1) {
                    if (distanceToNextMatch > matchIndex) break;   
                    matchIndex -= distanceToNextMatch;
                    continue;
        }   }   }

        {   U32 const distNextMatch = DELTANEXTU16(chainTable, matchIndex);
            if (patternAnalysis && distNextMatch==1 && matchChainPos==0) {
                U32 const matchCandidateIdx = matchIndex-1;
                
                if (repeat == rep_untested) {
                    if ( ((pattern & 0xFFFF) == (pattern >> 16))
                      &  ((pattern & 0xFF)   == (pattern >> 24)) ) {
                        repeat = rep_confirmed;
                        srcPatternLength = LZ4HC_countPattern(ip+sizeof(pattern), iHighLimit, pattern) + sizeof(pattern);
                    } else {
                        repeat = rep_not;
                }   }
                if ( (repeat == rep_confirmed)
                  && (matchCandidateIdx >= dictLimit) ) {   
                    const BYTE* const matchPtr = base + matchCandidateIdx;
                    if (LZ4_read32(matchPtr) == pattern) {  
                        size_t const forwardPatternLength = LZ4HC_countPattern(matchPtr+sizeof(pattern), iHighLimit, pattern) + sizeof(pattern);
                        const BYTE* const lowestMatchPtr = (lowPrefixPtr + LZ4_DISTANCE_MAX >= ip) ? lowPrefixPtr : ip - LZ4_DISTANCE_MAX;
                        size_t const backLength = LZ4HC_reverseCountPattern(matchPtr, lowestMatchPtr, pattern);
                        size_t const currentSegmentLength = backLength + forwardPatternLength;

                        if ( (currentSegmentLength >= srcPatternLength)   
                          && (forwardPatternLength <= srcPatternLength) ) { 
                            matchIndex = matchCandidateIdx + (U32)forwardPatternLength - (U32)srcPatternLength;  
                        } else {
                            matchIndex = matchCandidateIdx - (U32)backLength;   
                            if (lookBackLength==0) {  
                                size_t const maxML = MIN(currentSegmentLength, srcPatternLength);
                                if ((size_t)longest < maxML) {
                                    assert(base + matchIndex < ip);
                                    if (ip - (base+matchIndex) > LZ4_DISTANCE_MAX) break;
                                    assert(maxML < 2 GB);
                                    longest = (int)maxML;
                                    *matchpos = base + matchIndex;   
                                    *startpos = ip;
                                }
                                {   U32 const distToNextPattern = DELTANEXTU16(chainTable, matchIndex);
                                    if (distToNextPattern > matchIndex) break;  
                                    matchIndex -= distToNextPattern;
                        }   }   }
                        continue;
                }   }
        }   }   

        
        matchIndex -= DELTANEXTU16(chainTable, matchIndex + matchChainPos);

    }  

    if ( dict == usingDictCtxHc
      && nbAttempts
      && ipIndex - lowestMatchIndex < LZ4_DISTANCE_MAX) {
        size_t const dictEndOffset = (size_t)(dictCtx->end - dictCtx->base);
        U32 dictMatchIndex = dictCtx->hashTable[LZ4HC_hashPtr(ip)];
        assert(dictEndOffset <= 1 GB);
        matchIndex = dictMatchIndex + lowestMatchIndex - (U32)dictEndOffset;
        while (ipIndex - matchIndex <= LZ4_DISTANCE_MAX && nbAttempts--) {
            const BYTE* const matchPtr = dictCtx->base + dictMatchIndex;

            if (LZ4_read32(matchPtr) == pattern) {
                int mlt;
                int back = 0;
                const BYTE* vLimit = ip + (dictEndOffset - dictMatchIndex);
                if (vLimit > iHighLimit) vLimit = iHighLimit;
                mlt = (int)LZ4_count(ip+MINMATCH, matchPtr+MINMATCH, vLimit) + MINMATCH;
                back = lookBackLength ? LZ4HC_countBack(ip, matchPtr, iLowLimit, dictCtx->base + dictCtx->dictLimit) : 0;
                mlt -= back;
                if (mlt > longest) {
                    longest = mlt;
                    *matchpos = base + matchIndex + back;
                    *startpos = ip + back;
            }   }

            {   U32 const nextOffset = DELTANEXTU16(dictCtx->chainTable, dictMatchIndex);
                dictMatchIndex -= nextOffset;
                matchIndex -= nextOffset;
    }   }   }

    return longest;
}

LZ4_FORCE_INLINE
int LZ4HC_InsertAndFindBestMatch(LZ4HC_CCtx_internal* const hc4,   
                                 const BYTE* const ip, const BYTE* const iLimit,
                                 const BYTE** matchpos,
                                 const int maxNbAttempts,
                                 const int patternAnalysis,
                                 const dictCtx_directive dict)
{
    const BYTE* uselessPtr = ip;
    
    return LZ4HC_InsertAndGetWiderMatch(hc4, ip, ip, iLimit, MINMATCH-1, matchpos, &uselessPtr, maxNbAttempts, patternAnalysis, 0 , dict, favorCompressionRatio);
}


LZ4_FORCE_INLINE int LZ4HC_encodeSequence (
    const BYTE** ip,
    BYTE** op,
    const BYTE** anchor,
    int matchLength,
    const BYTE* const match,
    limitedOutput_directive limit,
    BYTE* oend)
{
    size_t length;
    BYTE* const token = (*op)++;

#if defined(LZ4_DEBUG) && (LZ4_DEBUG >= 6)
    static const BYTE* start = NULL;
    static U32 totalCost = 0;
    U32 const pos = (start==NULL) ? 0 : (U32)(*anchor - start);
    U32 const ll = (U32)(*ip - *anchor);
    U32 const llAdd = (ll>=15) ? ((ll-15) / 255) + 1 : 0;
    U32 const mlAdd = (matchLength>=19) ? ((matchLength-19) / 255) + 1 : 0;
    U32 const cost = 1 + llAdd + ll + 2 + mlAdd;
    if (start==NULL) start = *anchor;  
    
    DEBUGLOG(6, "pos:%7u -- literals:%3u, match:%4i, offset:%5u, cost:%3u + %u",
                pos,
                (U32)(*ip - *anchor), matchLength, (U32)(*ip-match),
                cost, totalCost);
    totalCost += cost;
#endif

    
    length = (size_t)(*ip - *anchor);
    if ((limit) && ((*op + (length / 255) + length + (2 + 1 + LASTLITERALS)) > oend)) return 1;   
    if (length >= RUN_MASK) {
        size_t len = length - RUN_MASK;
        *token = (RUN_MASK << ML_BITS);
        for(; len >= 255 ; len -= 255) *(*op)++ = 255;
        *(*op)++ = (BYTE)len;
    } else {
        *token = (BYTE)(length << ML_BITS);
    }

    
    LZ4_wildCopy8(*op, *anchor, (*op) + length);
    *op += length;

    
    assert( (*ip - match) <= LZ4_DISTANCE_MAX );   
    LZ4_writeLE16(*op, (U16)(*ip-match)); *op += 2;

    
    assert(matchLength >= MINMATCH);
    length = (size_t)matchLength - MINMATCH;
    if ((limit) && (*op + (length / 255) + (1 + LASTLITERALS) > oend)) return 1;   
    if (length >= ML_MASK) {
        *token += ML_MASK;
        length -= ML_MASK;
        for(; length >= 510 ; length -= 510) { *(*op)++ = 255; *(*op)++ = 255; }
        if (length >= 255) { length -= 255; *(*op)++ = 255; }
        *(*op)++ = (BYTE)length;
    } else {
        *token += (BYTE)(length);
    }

    
    *ip += matchLength;
    *anchor = *ip;

    return 0;
}

LZ4_FORCE_INLINE int LZ4HC_compress_hashChain (
    LZ4HC_CCtx_internal* const ctx,
    const char* const source,
    char* const dest,
    int* srcSizePtr,
    int const maxOutputSize,
    unsigned maxNbAttempts,
    const limitedOutput_directive limit,
    const dictCtx_directive dict
    )
{
    const int inputSize = *srcSizePtr;
    const int patternAnalysis = (maxNbAttempts > 128);   

    const BYTE* ip = (const BYTE*) source;
    const BYTE* anchor = ip;
    const BYTE* const iend = ip + inputSize;
    const BYTE* const mflimit = iend - MFLIMIT;
    const BYTE* const matchlimit = (iend - LASTLITERALS);

    BYTE* optr = (BYTE*) dest;
    BYTE* op = (BYTE*) dest;
    BYTE* oend = op + maxOutputSize;

    int   ml0, ml, ml2, ml3;
    const BYTE* start0;
    const BYTE* ref0;
    const BYTE* ref = NULL;
    const BYTE* start2 = NULL;
    const BYTE* ref2 = NULL;
    const BYTE* start3 = NULL;
    const BYTE* ref3 = NULL;

    
    *srcSizePtr = 0;
    if (limit == fillOutput) oend -= LASTLITERALS;                  
    if (inputSize < LZ4_minLength) goto _last_literals;                  

    
    while (ip <= mflimit) {
        ml = LZ4HC_InsertAndFindBestMatch(ctx, ip, matchlimit, &ref, maxNbAttempts, patternAnalysis, dict);
        if (ml<MINMATCH) { ip++; continue; }

        
        start0 = ip; ref0 = ref; ml0 = ml;

_Search2:
        if (ip+ml <= mflimit) {
            ml2 = LZ4HC_InsertAndGetWiderMatch(ctx,
                            ip + ml - 2, ip + 0, matchlimit, ml, &ref2, &start2,
                            maxNbAttempts, patternAnalysis, 0, dict, favorCompressionRatio);
        } else {
            ml2 = ml;
        }

        if (ml2 == ml) { 
            optr = op;
            if (LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), ml, ref, limit, oend)) goto _dest_overflow;
            continue;
        }

        if (start0 < ip) {   
            if (start2 < ip + ml0) {  
                ip = start0; ref = ref0; ml = ml0;  
        }   }

        
        if ((start2 - ip) < 3) {  
            ml = ml2;
            ip = start2;
            ref =ref2;
            goto _Search2;
        }

_Search3:
        
        if ((start2 - ip) < OPTIMAL_ML) {
            int correction;
            int new_ml = ml;
            if (new_ml > OPTIMAL_ML) new_ml = OPTIMAL_ML;
            if (ip+new_ml > start2 + ml2 - MINMATCH) new_ml = (int)(start2 - ip) + ml2 - MINMATCH;
            correction = new_ml - (int)(start2 - ip);
            if (correction > 0) {
                start2 += correction;
                ref2 += correction;
                ml2 -= correction;
            }
        }
        

        if (start2 + ml2 <= mflimit) {
            ml3 = LZ4HC_InsertAndGetWiderMatch(ctx,
                            start2 + ml2 - 3, start2, matchlimit, ml2, &ref3, &start3,
                            maxNbAttempts, patternAnalysis, 0, dict, favorCompressionRatio);
        } else {
            ml3 = ml2;
        }

        if (ml3 == ml2) {  
            
            if (start2 < ip+ml)  ml = (int)(start2 - ip);
            
            optr = op;
            if (LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), ml, ref, limit, oend)) goto _dest_overflow;
            ip = start2;
            optr = op;
            if (LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), ml2, ref2, limit, oend)) goto _dest_overflow;
            continue;
        }

        if (start3 < ip+ml+3) {  
            if (start3 >= (ip+ml)) {  
                if (start2 < ip+ml) {
                    int correction = (int)(ip+ml - start2);
                    start2 += correction;
                    ref2 += correction;
                    ml2 -= correction;
                    if (ml2 < MINMATCH) {
                        start2 = start3;
                        ref2 = ref3;
                        ml2 = ml3;
                    }
                }

                optr = op;
                if (LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), ml, ref, limit, oend)) goto _dest_overflow;
                ip  = start3;
                ref = ref3;
                ml  = ml3;

                start0 = start2;
                ref0 = ref2;
                ml0 = ml2;
                goto _Search2;
            }

            start2 = start3;
            ref2 = ref3;
            ml2 = ml3;
            goto _Search3;
        }

        
        if (start2 < ip+ml) {
            if ((start2 - ip) < OPTIMAL_ML) {
                int correction;
                if (ml > OPTIMAL_ML) ml = OPTIMAL_ML;
                if (ip + ml > start2 + ml2 - MINMATCH) ml = (int)(start2 - ip) + ml2 - MINMATCH;
                correction = ml - (int)(start2 - ip);
                if (correction > 0) {
                    start2 += correction;
                    ref2 += correction;
                    ml2 -= correction;
                }
            } else {
                ml = (int)(start2 - ip);
            }
        }
        optr = op;
        if (LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), ml, ref, limit, oend)) goto _dest_overflow;

        
        ip = start2; ref = ref2; ml = ml2;

        
        start2 = start3; ref2 = ref3; ml2 = ml3;

        
        goto _Search3;
    }

_last_literals:
    
    {   size_t lastRunSize = (size_t)(iend - anchor);  
        size_t litLength = (lastRunSize + 255 - RUN_MASK) / 255;
        size_t const totalSize = 1 + litLength + lastRunSize;
        if (limit == fillOutput) oend += LASTLITERALS;  
        if (limit && (op + totalSize > oend)) {
            if (limit == limitedOutput) return 0;  
            
            lastRunSize  = (size_t)(oend - op) - 1;
            litLength = (lastRunSize + 255 - RUN_MASK) / 255;
            lastRunSize -= litLength;
        }
        ip = anchor + lastRunSize;

        if (lastRunSize >= RUN_MASK) {
            size_t accumulator = lastRunSize - RUN_MASK;
            *op++ = (RUN_MASK << ML_BITS);
            for(; accumulator >= 255 ; accumulator -= 255) *op++ = 255;
            *op++ = (BYTE) accumulator;
        } else {
            *op++ = (BYTE)(lastRunSize << ML_BITS);
        }
        memcpy(op, anchor, lastRunSize);
        op += lastRunSize;
    }

    
    *srcSizePtr = (int) (((const char*)ip) - source);
    return (int) (((char*)op)-dest);

_dest_overflow:
    if (limit == fillOutput) {
        op = optr;  
        goto _last_literals;
    }
    return 0;
}


static int LZ4HC_compress_optimal( LZ4HC_CCtx_internal* ctx,
    const char* const source, char* dst,
    int* srcSizePtr, int dstCapacity,
    int const nbSearches, size_t sufficient_len,
    const limitedOutput_directive limit, int const fullUpdate,
    const dictCtx_directive dict,
    HCfavor_e favorDecSpeed);


LZ4_FORCE_INLINE int LZ4HC_compress_generic_internal (
    LZ4HC_CCtx_internal* const ctx,
    const char* const src,
    char* const dst,
    int* const srcSizePtr,
    int const dstCapacity,
    int cLevel,
    const limitedOutput_directive limit,
    const dictCtx_directive dict
    )
{
    typedef enum { lz4hc, lz4opt } lz4hc_strat_e;
    typedef struct {
        lz4hc_strat_e strat;
        U32 nbSearches;
        U32 targetLength;
    } cParams_t;
    static const cParams_t clTable[LZ4HC_CLEVEL_MAX+1] = {
        { lz4hc,     2, 16 },  
        { lz4hc,     2, 16 },  
        { lz4hc,     2, 16 },  
        { lz4hc,     4, 16 },  
        { lz4hc,     8, 16 },  
        { lz4hc,    16, 16 },  
        { lz4hc,    32, 16 },  
        { lz4hc,    64, 16 },  
        { lz4hc,   128, 16 },  
        { lz4hc,   256, 16 },  
        { lz4opt,   96, 64 },  
        { lz4opt,  512,128 },  
        { lz4opt,16384,LZ4_OPT_NUM },  
    };

    DEBUGLOG(4, "LZ4HC_compress_generic(ctx=%p, src=%p, srcSize=%d)", ctx, src, *srcSizePtr);

    if (limit == fillOutput && dstCapacity < 1) return 0;   
    if ((U32)*srcSizePtr > (U32)LZ4_MAX_INPUT_SIZE) return 0;    

    ctx->end += *srcSizePtr;
    if (cLevel < 1) cLevel = LZ4HC_CLEVEL_DEFAULT;   
    cLevel = MIN(LZ4HC_CLEVEL_MAX, cLevel);
    {   cParams_t const cParam = clTable[cLevel];
        HCfavor_e const favor = ctx->favorDecSpeed ? favorDecompressionSpeed : favorCompressionRatio;
        int result;

        if (cParam.strat == lz4hc) {
            result = LZ4HC_compress_hashChain(ctx,
                                src, dst, srcSizePtr, dstCapacity,
                                cParam.nbSearches, limit, dict);
        } else {
            assert(cParam.strat == lz4opt);
            result = LZ4HC_compress_optimal(ctx,
                                src, dst, srcSizePtr, dstCapacity,
                                (int)cParam.nbSearches, cParam.targetLength, limit,
                                cLevel == LZ4HC_CLEVEL_MAX,   
                                dict, favor);
        }
        if (result <= 0) ctx->dirty = 1;
        return result;
    }
}

static void LZ4HC_setExternalDict(LZ4HC_CCtx_internal* ctxPtr, const BYTE* newBlock);

static int
LZ4HC_compress_generic_noDictCtx (
        LZ4HC_CCtx_internal* const ctx,
        const char* const src,
        char* const dst,
        int* const srcSizePtr,
        int const dstCapacity,
        int cLevel,
        limitedOutput_directive limit
        )
{
    assert(ctx->dictCtx == NULL);
    return LZ4HC_compress_generic_internal(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit, noDictCtx);
}

static int
LZ4HC_compress_generic_dictCtx (
        LZ4HC_CCtx_internal* const ctx,
        const char* const src,
        char* const dst,
        int* const srcSizePtr,
        int const dstCapacity,
        int cLevel,
        limitedOutput_directive limit
        )
{
    const size_t position = (size_t)(ctx->end - ctx->base) - ctx->lowLimit;
    assert(ctx->dictCtx != NULL);
    if (position >= 64 KB) {
        ctx->dictCtx = NULL;
        return LZ4HC_compress_generic_noDictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
    } else if (position == 0 && *srcSizePtr > 4 KB) {
        memcpy(ctx, ctx->dictCtx, sizeof(LZ4HC_CCtx_internal));
        LZ4HC_setExternalDict(ctx, (const BYTE *)src);
        ctx->compressionLevel = (short)cLevel;
        return LZ4HC_compress_generic_noDictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
    } else {
        return LZ4HC_compress_generic_internal(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit, usingDictCtxHc);
    }
}

static int
LZ4HC_compress_generic (
        LZ4HC_CCtx_internal* const ctx,
        const char* const src,
        char* const dst,
        int* const srcSizePtr,
        int const dstCapacity,
        int cLevel,
        limitedOutput_directive limit
        )
{
    if (ctx->dictCtx == NULL) {
        return LZ4HC_compress_generic_noDictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
    } else {
        return LZ4HC_compress_generic_dictCtx(ctx, src, dst, srcSizePtr, dstCapacity, cLevel, limit);
    }
}


int LZ4_sizeofStateHC(void) { return (int)sizeof(LZ4_streamHC_t); }

#ifndef _MSC_VER  
static size_t LZ4_streamHC_t_alignment(void)
{
    struct { char c; LZ4_streamHC_t t; } t_a;
    return sizeof(t_a) - sizeof(t_a.t);
}
#endif


int LZ4_compress_HC_extStateHC_fastReset (void* state, const char* src, char* dst, int srcSize, int dstCapacity, int compressionLevel)
{
    LZ4HC_CCtx_internal* const ctx = &((LZ4_streamHC_t*)state)->internal_donotuse;
#ifndef _MSC_VER  
    assert(((size_t)state & (LZ4_streamHC_t_alignment() - 1)) == 0);  
#endif
    if (((size_t)(state)&(sizeof(void*)-1)) != 0) return 0;   
    LZ4_resetStreamHC_fast((LZ4_streamHC_t*)state, compressionLevel);
    LZ4HC_init_internal (ctx, (const BYTE*)src);
    if (dstCapacity < LZ4_compressBound(srcSize))
        return LZ4HC_compress_generic (ctx, src, dst, &srcSize, dstCapacity, compressionLevel, limitedOutput);
    else
        return LZ4HC_compress_generic (ctx, src, dst, &srcSize, dstCapacity, compressionLevel, notLimited);
}

int LZ4_compress_HC_extStateHC (void* state, const char* src, char* dst, int srcSize, int dstCapacity, int compressionLevel)
{
    LZ4_streamHC_t* const ctx = LZ4_initStreamHC(state, sizeof(*ctx));
    if (ctx==NULL) return 0;   
    return LZ4_compress_HC_extStateHC_fastReset(state, src, dst, srcSize, dstCapacity, compressionLevel);
}

int LZ4_compress_HC(const char* src, char* dst, int srcSize, int dstCapacity, int compressionLevel)
{
#if defined(LZ4HC_HEAPMODE) && LZ4HC_HEAPMODE==1
    LZ4_streamHC_t* const statePtr = (LZ4_streamHC_t*)ALLOC(sizeof(LZ4_streamHC_t));
#else
    LZ4_streamHC_t state;
    LZ4_streamHC_t* const statePtr = &state;
#endif
    int const cSize = LZ4_compress_HC_extStateHC(statePtr, src, dst, srcSize, dstCapacity, compressionLevel);
#if defined(LZ4HC_HEAPMODE) && LZ4HC_HEAPMODE==1
    FREEMEM(statePtr);
#endif
    return cSize;
}


int LZ4_compress_HC_destSize(void* state, const char* source, char* dest, int* sourceSizePtr, int targetDestSize, int cLevel)
{
    LZ4_streamHC_t* const ctx = LZ4_initStreamHC(state, sizeof(*ctx));
    if (ctx==NULL) return 0;   
    LZ4HC_init_internal(&ctx->internal_donotuse, (const BYTE*) source);
    LZ4_setCompressionLevel(ctx, cLevel);
    return LZ4HC_compress_generic(&ctx->internal_donotuse, source, dest, sourceSizePtr, targetDestSize, cLevel, fillOutput);
}





LZ4_streamHC_t* LZ4_createStreamHC(void)
{
    LZ4_streamHC_t* const LZ4_streamHCPtr = (LZ4_streamHC_t*)ALLOC(sizeof(LZ4_streamHC_t));
    if (LZ4_streamHCPtr==NULL) return NULL;
    LZ4_initStreamHC(LZ4_streamHCPtr, sizeof(*LZ4_streamHCPtr));  
    return LZ4_streamHCPtr;
}

int LZ4_freeStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr)
{
    DEBUGLOG(4, "LZ4_freeStreamHC(%p)", LZ4_streamHCPtr);
    if (!LZ4_streamHCPtr) return 0;  
    FREEMEM(LZ4_streamHCPtr);
    return 0;
}


LZ4_streamHC_t* LZ4_initStreamHC (void* buffer, size_t size)
{
    LZ4_streamHC_t* const LZ4_streamHCPtr = (LZ4_streamHC_t*)buffer;
    if (buffer == NULL) return NULL;
    if (size < sizeof(LZ4_streamHC_t)) return NULL;
#ifndef _MSC_VER  
    if (((size_t)buffer) & (LZ4_streamHC_t_alignment() - 1)) return NULL;  
#endif
    
    LZ4_STATIC_ASSERT(sizeof(LZ4HC_CCtx_internal) <= LZ4_STREAMHCSIZE);
    DEBUGLOG(4, "LZ4_initStreamHC(%p, %u)", LZ4_streamHCPtr, (unsigned)size);
    
    LZ4_streamHCPtr->internal_donotuse.end = (const BYTE *)(ptrdiff_t)-1;
    LZ4_streamHCPtr->internal_donotuse.base = NULL;
    LZ4_streamHCPtr->internal_donotuse.dictCtx = NULL;
    LZ4_streamHCPtr->internal_donotuse.favorDecSpeed = 0;
    LZ4_streamHCPtr->internal_donotuse.dirty = 0;
    LZ4_setCompressionLevel(LZ4_streamHCPtr, LZ4HC_CLEVEL_DEFAULT);
    return LZ4_streamHCPtr;
}


void LZ4_resetStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
    LZ4_initStreamHC(LZ4_streamHCPtr, sizeof(*LZ4_streamHCPtr));
    LZ4_setCompressionLevel(LZ4_streamHCPtr, compressionLevel);
}

void LZ4_resetStreamHC_fast (LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
    DEBUGLOG(4, "LZ4_resetStreamHC_fast(%p, %d)", LZ4_streamHCPtr, compressionLevel);
    if (LZ4_streamHCPtr->internal_donotuse.dirty) {
        LZ4_initStreamHC(LZ4_streamHCPtr, sizeof(*LZ4_streamHCPtr));
    } else {
        
        LZ4_streamHCPtr->internal_donotuse.end -= (uptrval)LZ4_streamHCPtr->internal_donotuse.base;
        LZ4_streamHCPtr->internal_donotuse.base = NULL;
        LZ4_streamHCPtr->internal_donotuse.dictCtx = NULL;
    }
    LZ4_setCompressionLevel(LZ4_streamHCPtr, compressionLevel);
}

void LZ4_setCompressionLevel(LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
    DEBUGLOG(5, "LZ4_setCompressionLevel(%p, %d)", LZ4_streamHCPtr, compressionLevel);
    if (compressionLevel < 1) compressionLevel = LZ4HC_CLEVEL_DEFAULT;
    if (compressionLevel > LZ4HC_CLEVEL_MAX) compressionLevel = LZ4HC_CLEVEL_MAX;
    LZ4_streamHCPtr->internal_donotuse.compressionLevel = (short)compressionLevel;
}

void LZ4_favorDecompressionSpeed(LZ4_streamHC_t* LZ4_streamHCPtr, int favor)
{
    LZ4_streamHCPtr->internal_donotuse.favorDecSpeed = (favor!=0);
}


int LZ4_loadDictHC (LZ4_streamHC_t* LZ4_streamHCPtr,
              const char* dictionary, int dictSize)
{
    LZ4HC_CCtx_internal* const ctxPtr = &LZ4_streamHCPtr->internal_donotuse;
    DEBUGLOG(4, "LZ4_loadDictHC(%p, %p, %d)", LZ4_streamHCPtr, dictionary, dictSize);
    assert(LZ4_streamHCPtr != NULL);
    if (dictSize > 64 KB) {
        dictionary += (size_t)dictSize - 64 KB;
        dictSize = 64 KB;
    }
    
    {   int const cLevel = ctxPtr->compressionLevel;
        LZ4_initStreamHC(LZ4_streamHCPtr, sizeof(*LZ4_streamHCPtr));
        LZ4_setCompressionLevel(LZ4_streamHCPtr, cLevel);
    }
    LZ4HC_init_internal (ctxPtr, (const BYTE*)dictionary);
    ctxPtr->end = (const BYTE*)dictionary + dictSize;
    if (dictSize >= 4) LZ4HC_Insert (ctxPtr, ctxPtr->end-3);
    return dictSize;
}

void LZ4_attach_HC_dictionary(LZ4_streamHC_t *working_stream, const LZ4_streamHC_t *dictionary_stream) {
    working_stream->internal_donotuse.dictCtx = dictionary_stream != NULL ? &(dictionary_stream->internal_donotuse) : NULL;
}



static void LZ4HC_setExternalDict(LZ4HC_CCtx_internal* ctxPtr, const BYTE* newBlock)
{
    DEBUGLOG(4, "LZ4HC_setExternalDict(%p, %p)", ctxPtr, newBlock);
    if (ctxPtr->end >= ctxPtr->base + ctxPtr->dictLimit + 4)
        LZ4HC_Insert (ctxPtr, ctxPtr->end-3);   

    
    ctxPtr->lowLimit  = ctxPtr->dictLimit;
    ctxPtr->dictLimit = (U32)(ctxPtr->end - ctxPtr->base);
    ctxPtr->dictBase  = ctxPtr->base;
    ctxPtr->base = newBlock - ctxPtr->dictLimit;
    ctxPtr->end  = newBlock;
    ctxPtr->nextToUpdate = ctxPtr->dictLimit;   
}

static int LZ4_compressHC_continue_generic (LZ4_streamHC_t* LZ4_streamHCPtr,
                                            const char* src, char* dst,
                                            int* srcSizePtr, int dstCapacity,
                                            limitedOutput_directive limit)
{
    LZ4HC_CCtx_internal* const ctxPtr = &LZ4_streamHCPtr->internal_donotuse;
    DEBUGLOG(4, "LZ4_compressHC_continue_generic(ctx=%p, src=%p, srcSize=%d)",
                LZ4_streamHCPtr, src, *srcSizePtr);
    assert(ctxPtr != NULL);
    
    if (ctxPtr->base == NULL) LZ4HC_init_internal (ctxPtr, (const BYTE*) src);

    
    if ((size_t)(ctxPtr->end - ctxPtr->base) > 2 GB) {
        size_t dictSize = (size_t)(ctxPtr->end - ctxPtr->base) - ctxPtr->dictLimit;
        if (dictSize > 64 KB) dictSize = 64 KB;
        LZ4_loadDictHC(LZ4_streamHCPtr, (const char*)(ctxPtr->end) - dictSize, (int)dictSize);
    }

    
    if ((const BYTE*)src != ctxPtr->end)
        LZ4HC_setExternalDict(ctxPtr, (const BYTE*)src);

    
    {   const BYTE* sourceEnd = (const BYTE*) src + *srcSizePtr;
        const BYTE* const dictBegin = ctxPtr->dictBase + ctxPtr->lowLimit;
        const BYTE* const dictEnd   = ctxPtr->dictBase + ctxPtr->dictLimit;
        if ((sourceEnd > dictBegin) && ((const BYTE*)src < dictEnd)) {
            if (sourceEnd > dictEnd) sourceEnd = dictEnd;
            ctxPtr->lowLimit = (U32)(sourceEnd - ctxPtr->dictBase);
            if (ctxPtr->dictLimit - ctxPtr->lowLimit < 4) ctxPtr->lowLimit = ctxPtr->dictLimit;
        }
    }

    return LZ4HC_compress_generic (ctxPtr, src, dst, srcSizePtr, dstCapacity, ctxPtr->compressionLevel, limit);
}

int LZ4_compress_HC_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* src, char* dst, int srcSize, int dstCapacity)
{
    if (dstCapacity < LZ4_compressBound(srcSize))
        return LZ4_compressHC_continue_generic (LZ4_streamHCPtr, src, dst, &srcSize, dstCapacity, limitedOutput);
    else
        return LZ4_compressHC_continue_generic (LZ4_streamHCPtr, src, dst, &srcSize, dstCapacity, notLimited);
}

int LZ4_compress_HC_continue_destSize (LZ4_streamHC_t* LZ4_streamHCPtr, const char* src, char* dst, int* srcSizePtr, int targetDestSize)
{
    return LZ4_compressHC_continue_generic(LZ4_streamHCPtr, src, dst, srcSizePtr, targetDestSize, fillOutput);
}





int LZ4_saveDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, char* safeBuffer, int dictSize)
{
    LZ4HC_CCtx_internal* const streamPtr = &LZ4_streamHCPtr->internal_donotuse;
    int const prefixSize = (int)(streamPtr->end - (streamPtr->base + streamPtr->dictLimit));
    DEBUGLOG(4, "LZ4_saveDictHC(%p, %p, %d)", LZ4_streamHCPtr, safeBuffer, dictSize);
    if (dictSize > 64 KB) dictSize = 64 KB;
    if (dictSize < 4) dictSize = 0;
    if (dictSize > prefixSize) dictSize = prefixSize;
    memmove(safeBuffer, streamPtr->end - dictSize, dictSize);
    {   U32 const endIndex = (U32)(streamPtr->end - streamPtr->base);
        streamPtr->end = (const BYTE*)safeBuffer + dictSize;
        streamPtr->base = streamPtr->end - endIndex;
        streamPtr->dictLimit = endIndex - (U32)dictSize;
        streamPtr->lowLimit = endIndex - (U32)dictSize;
        if (streamPtr->nextToUpdate < streamPtr->dictLimit) streamPtr->nextToUpdate = streamPtr->dictLimit;
    }
    return dictSize;
}







int LZ4_compressHC(const char* src, char* dst, int srcSize) { return LZ4_compress_HC (src, dst, srcSize, LZ4_compressBound(srcSize), 0); }
int LZ4_compressHC_limitedOutput(const char* src, char* dst, int srcSize, int maxDstSize) { return LZ4_compress_HC(src, dst, srcSize, maxDstSize, 0); }
int LZ4_compressHC2(const char* src, char* dst, int srcSize, int cLevel) { return LZ4_compress_HC (src, dst, srcSize, LZ4_compressBound(srcSize), cLevel); }
int LZ4_compressHC2_limitedOutput(const char* src, char* dst, int srcSize, int maxDstSize, int cLevel) { return LZ4_compress_HC(src, dst, srcSize, maxDstSize, cLevel); }
int LZ4_compressHC_withStateHC (void* state, const char* src, char* dst, int srcSize) { return LZ4_compress_HC_extStateHC (state, src, dst, srcSize, LZ4_compressBound(srcSize), 0); }
int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* src, char* dst, int srcSize, int maxDstSize) { return LZ4_compress_HC_extStateHC (state, src, dst, srcSize, maxDstSize, 0); }
int LZ4_compressHC2_withStateHC (void* state, const char* src, char* dst, int srcSize, int cLevel) { return LZ4_compress_HC_extStateHC(state, src, dst, srcSize, LZ4_compressBound(srcSize), cLevel); }
int LZ4_compressHC2_limitedOutput_withStateHC (void* state, const char* src, char* dst, int srcSize, int maxDstSize, int cLevel) { return LZ4_compress_HC_extStateHC(state, src, dst, srcSize, maxDstSize, cLevel); }
int LZ4_compressHC_continue (LZ4_streamHC_t* ctx, const char* src, char* dst, int srcSize) { return LZ4_compress_HC_continue (ctx, src, dst, srcSize, LZ4_compressBound(srcSize)); }
int LZ4_compressHC_limitedOutput_continue (LZ4_streamHC_t* ctx, const char* src, char* dst, int srcSize, int maxDstSize) { return LZ4_compress_HC_continue (ctx, src, dst, srcSize, maxDstSize); }



int LZ4_sizeofStreamStateHC(void) { return LZ4_STREAMHCSIZE; }


int LZ4_resetStreamStateHC(void* state, char* inputBuffer)
{
    LZ4_streamHC_t* const hc4 = LZ4_initStreamHC(state, sizeof(*hc4));
    if (hc4 == NULL) return 1;   
    LZ4HC_init_internal (&hc4->internal_donotuse, (const BYTE*)inputBuffer);
    return 0;
}

void* LZ4_createHC (const char* inputBuffer)
{
    LZ4_streamHC_t* const hc4 = LZ4_createStreamHC();
    if (hc4 == NULL) return NULL;   
    LZ4HC_init_internal (&hc4->internal_donotuse, (const BYTE*)inputBuffer);
    return hc4;
}

int LZ4_freeHC (void* LZ4HC_Data)
{
    if (!LZ4HC_Data) return 0;  
    FREEMEM(LZ4HC_Data);
    return 0;
}

int LZ4_compressHC2_continue (void* LZ4HC_Data, const char* src, char* dst, int srcSize, int cLevel)
{
    return LZ4HC_compress_generic (&((LZ4_streamHC_t*)LZ4HC_Data)->internal_donotuse, src, dst, &srcSize, 0, cLevel, notLimited);
}

int LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* src, char* dst, int srcSize, int dstCapacity, int cLevel)
{
    return LZ4HC_compress_generic (&((LZ4_streamHC_t*)LZ4HC_Data)->internal_donotuse, src, dst, &srcSize, dstCapacity, cLevel, limitedOutput);
}

char* LZ4_slideInputBufferHC(void* LZ4HC_Data)
{
    LZ4_streamHC_t *ctx = (LZ4_streamHC_t*)LZ4HC_Data;
    const BYTE *bufferStart = ctx->internal_donotuse.base + ctx->internal_donotuse.lowLimit;
    LZ4_resetStreamHC_fast(ctx, ctx->internal_donotuse.compressionLevel);
    
    return (char *)(uptrval)bufferStart;
}



typedef struct {
    int price;
    int off;
    int mlen;
    int litlen;
} LZ4HC_optimal_t;


LZ4_FORCE_INLINE int LZ4HC_literalsPrice(int const litlen)
{
    int price = litlen;
    assert(litlen >= 0);
    if (litlen >= (int)RUN_MASK)
        price += 1 + ((litlen-(int)RUN_MASK) / 255);
    return price;
}



LZ4_FORCE_INLINE int LZ4HC_sequencePrice(int litlen, int mlen)
{
    int price = 1 + 2 ; 
    assert(litlen >= 0);
    assert(mlen >= MINMATCH);

    price += LZ4HC_literalsPrice(litlen);

    if (mlen >= (int)(ML_MASK+MINMATCH))
        price += 1 + ((mlen-(int)(ML_MASK+MINMATCH)) / 255);

    return price;
}


typedef struct {
    int off;
    int len;
} LZ4HC_match_t;

LZ4_FORCE_INLINE LZ4HC_match_t
LZ4HC_FindLongerMatch(LZ4HC_CCtx_internal* const ctx,
                      const BYTE* ip, const BYTE* const iHighLimit,
                      int minLen, int nbSearches,
                      const dictCtx_directive dict,
                      const HCfavor_e favorDecSpeed)
{
    LZ4HC_match_t match = { 0 , 0 };
    const BYTE* matchPtr = NULL;
    
    int matchLength = LZ4HC_InsertAndGetWiderMatch(ctx, ip, ip, iHighLimit, minLen, &matchPtr, &ip, nbSearches, 1 , 1 , dict, favorDecSpeed);
    if (matchLength <= minLen) return match;
    if (favorDecSpeed) {
        if ((matchLength>18) & (matchLength<=36)) matchLength=18;   
    }
    match.len = matchLength;
    match.off = (int)(ip-matchPtr);
    return match;
}


static int LZ4HC_compress_optimal ( LZ4HC_CCtx_internal* ctx,
                                    const char* const source,
                                    char* dst,
                                    int* srcSizePtr,
                                    int dstCapacity,
                                    int const nbSearches,
                                    size_t sufficient_len,
                                    const limitedOutput_directive limit,
                                    int const fullUpdate,
                                    const dictCtx_directive dict,
                                    const HCfavor_e favorDecSpeed)
{
#define TRAILING_LITERALS 3
    LZ4HC_optimal_t opt[LZ4_OPT_NUM + TRAILING_LITERALS];   

    const BYTE* ip = (const BYTE*) source;
    const BYTE* anchor = ip;
    const BYTE* const iend = ip + *srcSizePtr;
    const BYTE* const mflimit = iend - MFLIMIT;
    const BYTE* const matchlimit = iend - LASTLITERALS;
    BYTE* op = (BYTE*) dst;
    BYTE* opSaved = (BYTE*) dst;
    BYTE* oend = op + dstCapacity;

    
    DEBUGLOG(5, "LZ4HC_compress_optimal(dst=%p, dstCapa=%u)", dst, (unsigned)dstCapacity);
    *srcSizePtr = 0;
    if (limit == fillOutput) oend -= LASTLITERALS;   
    if (sufficient_len >= LZ4_OPT_NUM) sufficient_len = LZ4_OPT_NUM-1;

    
    assert(ip - anchor < LZ4_MAX_INPUT_SIZE);
    while (ip <= mflimit) {
         int const llen = (int)(ip - anchor);
         int best_mlen, best_off;
         int cur, last_match_pos = 0;

         LZ4HC_match_t const firstMatch = LZ4HC_FindLongerMatch(ctx, ip, matchlimit, MINMATCH-1, nbSearches, dict, favorDecSpeed);
         if (firstMatch.len==0) { ip++; continue; }

         if ((size_t)firstMatch.len > sufficient_len) {
             
             int const firstML = firstMatch.len;
             const BYTE* const matchPos = ip - firstMatch.off;
             opSaved = op;
             if ( LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), firstML, matchPos, limit, oend) )   
                 goto _dest_overflow;
             continue;
         }

         
         {   int rPos;
             for (rPos = 0 ; rPos < MINMATCH ; rPos++) {
                 int const cost = LZ4HC_literalsPrice(llen + rPos);
                 opt[rPos].mlen = 1;
                 opt[rPos].off = 0;
                 opt[rPos].litlen = llen + rPos;
                 opt[rPos].price = cost;
                 DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i) -- initial setup",
                             rPos, cost, opt[rPos].litlen);
         }   }
         
         {   int mlen = MINMATCH;
             int const matchML = firstMatch.len;   
             int const offset = firstMatch.off;
             assert(matchML < LZ4_OPT_NUM);
             for ( ; mlen <= matchML ; mlen++) {
                 int const cost = LZ4HC_sequencePrice(llen, mlen);
                 opt[mlen].mlen = mlen;
                 opt[mlen].off = offset;
                 opt[mlen].litlen = llen;
                 opt[mlen].price = cost;
                 DEBUGLOG(7, "rPos:%3i => price:%3i (matchlen=%i) -- initial setup",
                             mlen, cost, mlen);
         }   }
         last_match_pos = firstMatch.len;
         {   int addLit;
             for (addLit = 1; addLit <= TRAILING_LITERALS; addLit ++) {
                 opt[last_match_pos+addLit].mlen = 1; 
                 opt[last_match_pos+addLit].off = 0;
                 opt[last_match_pos+addLit].litlen = addLit;
                 opt[last_match_pos+addLit].price = opt[last_match_pos].price + LZ4HC_literalsPrice(addLit);
                 DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i) -- initial setup",
                             last_match_pos+addLit, opt[last_match_pos+addLit].price, addLit);
         }   }

         
         for (cur = 1; cur < last_match_pos; cur++) {
             const BYTE* const curPtr = ip + cur;
             LZ4HC_match_t newMatch;

             if (curPtr > mflimit) break;
             DEBUGLOG(7, "rPos:%u[%u] vs [%u]%u",
                     cur, opt[cur].price, opt[cur+1].price, cur+1);
             if (fullUpdate) {
                 
                 if ( (opt[cur+1].price <= opt[cur].price)
                   
                   && (opt[cur+MINMATCH].price < opt[cur].price + 3) )
                     continue;
             } else {
                 
                 if (opt[cur+1].price <= opt[cur].price) continue;
             }

             DEBUGLOG(7, "search at rPos:%u", cur);
             if (fullUpdate)
                 newMatch = LZ4HC_FindLongerMatch(ctx, curPtr, matchlimit, MINMATCH-1, nbSearches, dict, favorDecSpeed);
             else
                 
                 newMatch = LZ4HC_FindLongerMatch(ctx, curPtr, matchlimit, last_match_pos - cur, nbSearches, dict, favorDecSpeed);
             if (!newMatch.len) continue;

             if ( ((size_t)newMatch.len > sufficient_len)
               || (newMatch.len + cur >= LZ4_OPT_NUM) ) {
                 
                 best_mlen = newMatch.len;
                 best_off = newMatch.off;
                 last_match_pos = cur + 1;
                 goto encode;
             }

             
             {   int const baseLitlen = opt[cur].litlen;
                 int litlen;
                 for (litlen = 1; litlen < MINMATCH; litlen++) {
                     int const price = opt[cur].price - LZ4HC_literalsPrice(baseLitlen) + LZ4HC_literalsPrice(baseLitlen+litlen);
                     int const pos = cur + litlen;
                     if (price < opt[pos].price) {
                         opt[pos].mlen = 1; 
                         opt[pos].off = 0;
                         opt[pos].litlen = baseLitlen+litlen;
                         opt[pos].price = price;
                         DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i)",
                                     pos, price, opt[pos].litlen);
             }   }   }

             
             {   int const matchML = newMatch.len;
                 int ml = MINMATCH;

                 assert(cur + newMatch.len < LZ4_OPT_NUM);
                 for ( ; ml <= matchML ; ml++) {
                     int const pos = cur + ml;
                     int const offset = newMatch.off;
                     int price;
                     int ll;
                     DEBUGLOG(7, "testing price rPos %i (last_match_pos=%i)",
                                 pos, last_match_pos);
                     if (opt[cur].mlen == 1) {
                         ll = opt[cur].litlen;
                         price = ((cur > ll) ? opt[cur - ll].price : 0)
                               + LZ4HC_sequencePrice(ll, ml);
                     } else {
                         ll = 0;
                         price = opt[cur].price + LZ4HC_sequencePrice(0, ml);
                     }

                    assert((U32)favorDecSpeed <= 1);
                     if (pos > last_match_pos+TRAILING_LITERALS
                      || price <= opt[pos].price - (int)favorDecSpeed) {
                         DEBUGLOG(7, "rPos:%3i => price:%3i (matchlen=%i)",
                                     pos, price, ml);
                         assert(pos < LZ4_OPT_NUM);
                         if ( (ml == matchML)  
                           && (last_match_pos < pos) )
                             last_match_pos = pos;
                         opt[pos].mlen = ml;
                         opt[pos].off = offset;
                         opt[pos].litlen = ll;
                         opt[pos].price = price;
             }   }   }
             
             {   int addLit;
                 for (addLit = 1; addLit <= TRAILING_LITERALS; addLit ++) {
                     opt[last_match_pos+addLit].mlen = 1; 
                     opt[last_match_pos+addLit].off = 0;
                     opt[last_match_pos+addLit].litlen = addLit;
                     opt[last_match_pos+addLit].price = opt[last_match_pos].price + LZ4HC_literalsPrice(addLit);
                     DEBUGLOG(7, "rPos:%3i => price:%3i (litlen=%i)", last_match_pos+addLit, opt[last_match_pos+addLit].price, addLit);
             }   }
         }  

         assert(last_match_pos < LZ4_OPT_NUM + TRAILING_LITERALS);
         best_mlen = opt[last_match_pos].mlen;
         best_off = opt[last_match_pos].off;
         cur = last_match_pos - best_mlen;

 encode: 
         assert(cur < LZ4_OPT_NUM);
         assert(last_match_pos >= 1);  
         DEBUGLOG(6, "reverse traversal, looking for shortest path (last_match_pos=%i)", last_match_pos);
         {   int candidate_pos = cur;
             int selected_matchLength = best_mlen;
             int selected_offset = best_off;
             while (1) {  
                 int const next_matchLength = opt[candidate_pos].mlen;  
                 int const next_offset = opt[candidate_pos].off;
                 DEBUGLOG(7, "pos %i: sequence length %i", candidate_pos, selected_matchLength);
                 opt[candidate_pos].mlen = selected_matchLength;
                 opt[candidate_pos].off = selected_offset;
                 selected_matchLength = next_matchLength;
                 selected_offset = next_offset;
                 if (next_matchLength > candidate_pos) break; 
                 assert(next_matchLength > 0);  
                 candidate_pos -= next_matchLength;
         }   }

         
         {   int rPos = 0;  
             while (rPos < last_match_pos) {
                 int const ml = opt[rPos].mlen;
                 int const offset = opt[rPos].off;
                 if (ml == 1) { ip++; rPos++; continue; }  
                 rPos += ml;
                 assert(ml >= MINMATCH);
                 assert((offset >= 1) && (offset <= LZ4_DISTANCE_MAX));
                 opSaved = op;
                 if ( LZ4HC_encodeSequence(UPDATABLE(ip, op, anchor), ml, ip - offset, limit, oend) )   
                     goto _dest_overflow;
         }   }
     }  

 _last_literals:
     
     {   size_t lastRunSize = (size_t)(iend - anchor);  
         size_t litLength = (lastRunSize + 255 - RUN_MASK) / 255;
         size_t const totalSize = 1 + litLength + lastRunSize;
         if (limit == fillOutput) oend += LASTLITERALS;  
         if (limit && (op + totalSize > oend)) {
             if (limit == limitedOutput) return 0;  
             
             lastRunSize  = (size_t)(oend - op) - 1;
             litLength = (lastRunSize + 255 - RUN_MASK) / 255;
             lastRunSize -= litLength;
         }
         ip = anchor + lastRunSize;

         if (lastRunSize >= RUN_MASK) {
             size_t accumulator = lastRunSize - RUN_MASK;
             *op++ = (RUN_MASK << ML_BITS);
             for(; accumulator >= 255 ; accumulator -= 255) *op++ = 255;
             *op++ = (BYTE) accumulator;
         } else {
             *op++ = (BYTE)(lastRunSize << ML_BITS);
         }
         memcpy(op, anchor, lastRunSize);
         op += lastRunSize;
     }

     
     *srcSizePtr = (int) (((const char*)ip) - source);
     return (int) ((char*)op-dst);

 _dest_overflow:
     if (limit == fillOutput) {
         op = opSaved;  
         goto _last_literals;
     }
     return 0;
 }
