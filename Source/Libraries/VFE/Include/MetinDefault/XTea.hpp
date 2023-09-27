#include <string>
#ifdef __cplusplus
extern "C" {
#endif

#define TEA_KEY_LENGTH 16
    int32_t tea_encrypt(uint32_t* dest, const uint32_t* src, const uint32_t* key, int32_t size);
    int32_t tea_decrypt(uint32_t* dest, const uint32_t* src, const uint32_t* key, int32_t size);

#ifdef __cplusplus
};
#endif
