#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stddef.h>
#include <lz4frame.h>
#include <sys/mman.h>

#include "tfstructs.h"


void printHex(unsigned char* fbuf, size_t start, size_t len) {
    char *s = malloc(3 * len + 1);
    for (size_t i = 0; i < len; i++) {
        unsigned char v = *(fbuf + start + i);
        unsigned char h = v >> 4;
        unsigned char l = v & 0xf;
        s[3*i] = (h < 10 ? '0' : 'a' - 10) + h;
        s[3*i+1] = (l < 10 ? '0' : 'a' - 10) + l;
        s[3*i+2] = ' ';
    }
    s[3*len] = 0;
    printf("0x%x | %s\n", start, s);
    free(s);
}



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



int main(int argc, char* argv[]) {

    if (argc < 2) {
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
    char *fbuf = buf1 + saveSize * 2;

    // DECOMPRESS 1
    size_t dec1Len = decode_lz4(mSaveGame, saveSize, buf1, saveSize * 2);
    if ( dec1Len < 0 ) {
        printf("first decode failed: %ld\n", dec1Len);
        return -1;
    }

    printf("Dec1 has %lu bytes (%f%% of src)\n", dec1Len, (float)dec1Len * 100.0 / (float) saveSize);
    
    // DECOMPRESS 2
    size_t dec2Len = decode_lz4(buf1, dec1Len, fbuf, saveSize * 2);
    if ( dec2Len < 0 ) {
        printf("second decode failed: %ld\n", dec2Len);
        return -1;
    }

    printf("Dec2 has %lu bytes (%f%% of src)\n", dec2Len, (float)dec2Len * 100.0 / (float) saveSize);

    // check header
    if (memcmp("tf**", fbuf, 4)) {
        char sig[5];
        memcpy(&sig, fbuf, 4);
        fbuf[4] = 0;
        printf("signature not present | \"%s\"\n", sig);
        return -1;
    }

    char saveFile = 0;
    for (size_t i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) saveFile = 1;
    }

    if (saveFile) {
        // save file
        size_t rawNameEnd = strlen(argv[1]);
        char *dsfName = malloc(rawNameEnd + 5);
        strcpy(dsfName, argv[1]);
        strcpy(dsfName + rawNameEnd, ".hex");
        printf("Saving decompressed file to %s\n", dsfName);
        FILE *decompressedSaveFile = fopen(dsfName, "w");

        size_t w = fwrite(fbuf, 1, dec2Len, decompressedSaveFile);
        if (w != dec2Len) {
            printf("Failed to write all bytes to disk :( | wrote %lu\n", w);
        }
        free(dsfName);
    }

    TFHeader header;
    size_t headerAddr = 0x4;
    memcpy(&header, fbuf + headerAddr, sizeof header);

    printf("version: %x\ndifficulty: %x\nstartYear: %d\ntiles: %d x %d\ndate: %u\nmoney: %ld\n", 
    header.saveVersion, header.difficulty, header.startYear, header.tilesX, header.tilesY, header.date, header.money);

    size_t modsListAddr = headerAddr + sizeof header;


    // === MOD NAME LIST ===
    
    TFList modNameList;

    modNameList.len = *(uint32_t*) (fbuf + modsListAddr);
    modNameList.data = malloc(modNameList.len * sizeof(char*));

    modsListAddr += 4;

    for(size_t modNr = 0; modNr < modNameList.len; modNr++) {
        // read string
        ((char**)modNameList.data)[modNr] = readTFString(fbuf, &modsListAddr);
        printf("Mod #%lu: \"%s\"\n", modNr, ((char**)modNameList.data)[modNr]);
    }

    size_t currAddr = modsListAddr;
    printf("next addr: 0x%lx\n", currAddr);

    uint8_t achievementsEarnable = *(fbuf + currAddr);

    currAddr += 9; // unknown fields

    uint32_t uL1Len = *(uint32_t*)(fbuf + currAddr);
    currAddr += 4;
    printf("list len: %u, next addr: 0x%lx\n", uL1Len, currAddr);
    currAddr += uL1Len;
    printf("next addr: 0x%lx\n", currAddr);

    
    // === MOD ID LIST ===

    TFList modIdList;
    modIdList.len = *(uint32_t*)(fbuf + currAddr);
    modIdList.data = malloc(sizeof(char*) * modIdList.len);

    printf("midIdLen: %lu\n", modIdList.len);

    currAddr += 4;
    for (size_t i = 0; i < modNameList.len; i++) {
        ((char**)modIdList.data)[i] = readTFString(fbuf, &currAddr);
        printf("Mod #%lu: \"%s\"\n", i, ((char**)modIdList.data)[i]);
    }


    // === GAME SETTINGS LIST ===

    TFList settingsList;
    settingsList.len = *(uint32_t*)(fbuf + currAddr);
    printf("settingsListLen : %lu\n", settingsList.len);
    settingsList.data = malloc(sizeof(TFKeyValue) * settingsList.len);
    currAddr += 4;

    for (int i = 0; i < settingsList.len; i++) {
        TFKeyValue settingsKV = readTFKeyValue(fbuf, &currAddr);
    }

    // === LIST ===

    TFList u1List;
    u1List.len = *(uint32_t*)(fbuf + currAddr);
    printf("u1ListLen : %lu\n", u1List.len);
    if (u1List.len > 0) {
        puts("ERROR: listU1List structure unknown. can't continue parsing");
        return;
    }
    currAddr += 4;

    // 1 byte + 4 byte

    currAddr += 1 + 4;

    size_t stringLen = *(uint32_t*)(fbuf + currAddr);
    //printf("len: %lu\n", stringLen);
    char* settingsEndStr = malloc(stringLen + 1);
    currAddr += 4;
    memcpy(settingsEndStr, fbuf + currAddr, stringLen);
    settingsEndStr[stringLen] = 0;

    currAddr += stringLen;

    printf("%s | next addr: 0x%lx\n", settingsEndStr, currAddr);


    // mdl asset list

    TFList mdlAssetList;
    mdlAssetList.len = *(uint32_t*)(fbuf + currAddr);
    currAddr += 4;
    mdlAssetList.data = malloc(sizeof(char *) * mdlAssetList.len);
    char** malData = (char**) mdlAssetList.data;
    for(size_t i = 0; i < mdlAssetList.len; i++) {
        stringLen = *(uint32_t*)(fbuf + currAddr);
        currAddr += 4;
        // [4 byte id][str + '\0']
        malData[i] = malloc(4 + stringLen + 1);
        memcpy(malData[i] + 4, fbuf + currAddr, stringLen);
        *(malData[i] + 4 + stringLen) = 0;
        currAddr += stringLen;
        // copy id
        *(uint32_t*)malData[i] = *(uint32_t*)(fbuf + currAddr);
        currAddr += 4;

        //printf("ID: %x | path: %s\n", (uint32_t) *malData[i], malData[i] + 4);
    }

    printf("assets: %lu\n", mdlAssetList.len);

    printf("next addr: 0x%lx\n", currAddr);

    for(int j = 0; j < 9; j++) {
        //TFList loadLuaPathsList;

        //loadLuaPathsList.len = *(uint32_t*)(fbuf + currAddr);
        uint32_t len = *(uint32_t*)(fbuf + currAddr);
        printf("script chunk %u: %u entries\n", j, len);
        currAddr += 4;
        //loadLuaPathsList.data = malloc(sizeof(char*) + loadLuaPathsList.len);

        for(size_t i = 0; i < len; i++) {
            stringLen = *(uint32_t*)(fbuf + currAddr);
            currAddr += 4;
            char *str = malloc(stringLen + 1);
            memcpy(str, fbuf + currAddr, stringLen);
            currAddr += stringLen + 4; // + unused id
            str[stringLen] = 0;
            //((char**)loadLuaPathsList.data)[i] = str;
            printf("Lua script: %s\n", str);
        }

    }

    printf("next addr: 0x%lx\n", currAddr);


    // 4.1
    currAddr += 4; // unknown
    uint32_t listLen = *(uint32_t*)(fbuf + currAddr);
    
    if (listLen != 0) {
        // I hope listLen is correct and there are 4 byte data, but not sure about that
        printf("Error on 0x%lx: expected 0 sized list, got %u elements. this list is not reverse engeneered :(\n", currAddr, listLen);
        return;
    }

    currAddr += 4;

    uint32_t ecsListLen = *(uint32_t*) (fbuf + currAddr);
    currAddr += 4;
    printf("ecs list len: %u\n", ecsListLen);
    printf("next addr: 0x%lx\n", currAddr);

    for(uint32_t ecsListIdx = 0; ecsListIdx < ecsListLen; ecsListIdx++) {
        uint32_t innerLen = *(uint32_t*)(fbuf + currAddr);
        currAddr+=4;
        printf("\ninner ecs list at 0x%x | size: %u\n", currAddr, innerLen);
        puts("first:");
        printHex(fbuf, currAddr, 0x21);
        currAddr += innerLen * 0x21;
        
        // 4 * 8byte trailing
        currAddr += 32;
    }


    printf("next addr: 0x%lx\n", currAddr);

    return 0;
}   