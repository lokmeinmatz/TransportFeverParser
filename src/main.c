#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stddef.h>
#include <lz4frame.h>
#include <sys/mman.h>

// packed strucht of tf / tf2 header 
// by https://www.transportfever.net/index.php?thread/4659-train-fever-savegame-format/
typedef struct __attribute__((__packed__)) {
    int32_t saveVersion;
    int32_t difficulty;
    int32_t startYear;
    int32_t tilesX;
    int32_t tilesY;
    uint32_t date;
    int64_t money;
} TFHeader;

typedef struct {
    int32_t len;
} TFStringHead;


// decodes data from origData to targetBuf.
// format is in lz4 frames
// returns decoded length or -1 if failed
size_t decode_lz4(const char* origData, size_t origLen, char* targetBuf, size_t targetLen) {
    LZ4F_dctx* dctx;

    if (*(int*)origData != 0x184D2204) {
        puts("file is not lz4 compressed, magic number not present :/");
        return -1;
    }

    
    if (LZ4F_isError(LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION))) {
        puts("Error 1");
        return -1;
    }

    puts("created decompressio context");

    LZ4F_decompressOptions_t options;

    memset(&options, 0, sizeof options);
    options.stableDst = 1;

    int ret = LZ4F_decompress(dctx, targetBuf, &targetLen, origData, &origLen, &options);

    if (LZ4F_isError(ret)) {
        puts("decompress failed");
    }

    //printf("used %lu bytes\n", origLen);

    LZ4F_freeDecompressionContext(dctx);
    return targetLen;
}

typedef struct {
    size_t len;
    void* data;
} TFList;


// allocates list
// after finsih addr points to next byte
char* readTFString(char* buf, size_t* addr) {
    uint32_t len = *(uint32_t*) (buf + *addr);
    char *res = malloc(len + 1);
    memcpy(res, buf + *addr + 4, len);
    res[len] = 0;
    *addr += 4 + len + 4;
    return res;
}


int main(int argc, char* argv[]) {

    if (argc != 2) {
        puts("specify path to save file");
        return -1;
    }
    
    // get original file size
    FILE* fSaveGame = fopen(argv[1], "r");
    //FILE* fSaveGame = fopen("/mnt/c/Program Files (x86)/Steam/userdata/256166337/1066780/local/save/desert1.sav", "r");
    fseek(fSaveGame, 0L, SEEK_END);
    size_t saveSize = ftell(fSaveGame);
    printf("Save size: %ld\n", saveSize);
    rewind(fSaveGame);
    int f = fileno(fSaveGame);
    char *mSaveGame = mmap(NULL, saveSize, PROT_READ, MAP_PRIVATE, f, 0);
    printf("%d %lx\n", f, (size_t) mSaveGame);
    if (!mSaveGame) {
        puts("Failed to open file");
        return -1;
    }

    
    char *buf1 = malloc(saveSize * 4);
    char *rBuf2 = buf1 + saveSize * 2;

    // DECOMPRESS 1
    size_t dec1Len = decode_lz4(mSaveGame, saveSize, buf1, saveSize * 2);
    if ( dec1Len < 0 ) {
        printf("first decode failed: %ld\n", dec1Len);
        return -1;
    }

    printf("Dec1 has %lu bytes (%f%% of src)\n", dec1Len, (float)dec1Len * 100.0 / (float) saveSize);
    
    // DECOMPRESS 2
    size_t dec2Len = decode_lz4(buf1, dec1Len, rBuf2, saveSize * 2);
    if ( dec2Len < 0 ) {
        printf("second decode failed: %ld\n", dec2Len);
        return -1;
    }

    printf("Dec2 has %lu bytes (%f%% of src)\n", dec2Len, (float)dec2Len * 100.0 / (float) saveSize);

    // check header
    if (memcmp("tf**", rBuf2, 4)) {
        char sig[5];
        memcpy(&sig, rBuf2, 4);
        rBuf2[4] = 0;
        printf("signature not present | \"%s\"\n", sig);
        return -1;
    }

    char saveFile = 0;
    for (size_t i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-s")) saveFile = 1;
    }

    if (saveFile) {
        // save file
        size_t rawNameEnd = strlen(argv[1]);
        char *dsfName = malloc(rawNameEnd + 5);
        strcpy(dsfName, argv[1]);
        strcpy(dsfName + rawNameEnd, ".hex");
        printf("Saving decompressed file to %s\n", dsfName);
        FILE *decompressedSaveFile = fopen(dsfName, "w");

        size_t w = fwrite(rBuf2, 1, dec2Len, decompressedSaveFile);
        if (w != dec2Len) {
            printf("Failed to write all bytes to disk :( | wrote %lu\n", w);
        }
        free(dsfName);
    }

    TFHeader header;
    size_t headerAddr = 0x4;
    memcpy(&header, rBuf2 + headerAddr, sizeof header);

    printf("version: %x\ndifficulty: %x\nstartYear: %d\ntiles: %d x %d\ndate: %u\nmoney: %ld\n", 
    header.saveVersion, header.difficulty, header.startYear, header.tilesX, header.tilesY, header.date, header.money);

    size_t modsListAddr = headerAddr + sizeof header;
    
    TFList modsList;

    modsList.len = *(uint32_t*) (rBuf2 + modsListAddr);
    modsList.data = malloc(modsList.len * sizeof(char*));

    modsListAddr += 4;

    for(size_t modNr = 0; modNr < modsList.len; modNr++) {
        // read string
        ((char**)modsList.data)[modNr] = readTFString(rBuf2, &modsListAddr);
        printf("Mod #%lu: \"%s\"\n", modNr, ((char**)modsList.data)[modNr]);
    }

    printf("next addr: 0x%lx\n", modsListAddr);

    return 0;
}   