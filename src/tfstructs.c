#include "tfstructs.h"

TFKeyValue readTFKeyValue(char* buf, size_t* addr) {
    uint32_t keyLen = *(uint32_t*) (buf + *addr);
    uint32_t valueLen = *(uint32_t*) (buf + *addr + keyLen + 4);
    TFKeyValue kv;
    kv.key = malloc(keyLen + valueLen + 2);
    kv.value = kv.key + keyLen + 1;
    printf("%lx [%lu] => %lx [%lu]\n", kv.key, keyLen, kv.value, valueLen);
    memcpy(kv.key, buf + *addr + 4, keyLen);
    *(kv.value - 1) = 0;

    memcpy(kv.value, buf + *addr + 4 + keyLen + 4, keyLen);
    kv.value[valueLen + 1] = 0;
    printf("%s = %s\n", kv.key, kv.value);

    *addr += keyLen + 4 + valueLen + 4;
    return kv;
}


// allocates list
// after finsih addr points to next byte
char* readTFString(char* buf, size_t* addr) {
    uint32_t len = *(uint32_t*) (buf + *addr);
    char *res = malloc(len + 1);
    memcpy(res, buf + *addr + 4, len);
    res[len] = 0;
    *addr += 4 + len + 4; // add severity
    return res;
}