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
    int32_t tiles;
    uint32_t date;
    int64_t money;
    int32_t __padding;
    int32_t numMods;
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

    printf("used %lu bytes\n", origLen);

    LZ4F_freeDecompressionContext(dctx);
    return targetLen;
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

    TFHeader header;
    memcpy(&header, rBuf2 + 4, sizeof header);

    printf("version: %x\ndifficulty: %x\nstartYear: %d\ntiles: %d\ndate: %u\nmoney: %ld\n#mods: %u\n", 
    header.saveVersion, header.difficulty, header.startYear, header.tiles, header.date, header.money, header.numMods);

    return 0;
}   