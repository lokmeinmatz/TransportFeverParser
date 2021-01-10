
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*

TF2 file sructure

>>> Main save game write function (std::basic_ostream<char,struct_std::char_traits<char>_>::write) and writes "tf**" <<<

char[4] 'tf**'
int32 saveVersion

>>> Function 1: general header <<<

int32 difficulty
int32 startYear
int32 tilesX
int32 tilesY
uint32 date
int64 money


=== Mod Names ===

uint32 modListLen
#modListeLen {
    uint32 modNameLen
    char modName[modNameLen]
    uint32 severity?
}

uint8 achievements?
int32 unknown
int32 unknown


=== UnknownList ===

uint32 unknownList1Len
#unknownList1Len {
    uint8 ?
}

>>> Function 2: modId and kv <<<

=== Mod Id List ===

uint32 modIdListLen
#modIdListLen {
    uint32 modIdLen
    char modId[modIdLen]
    uint32 severity?
}


=== Game Settings List ===

uint32 settingsListLen
#settingsListLen {
    uint32 keyLen
    char key[keyLen]
    uint32 valueLen
    char value[valueLen]
}

=== Unknown List , currently 0 ===

uint32 listLen


===

int8 unknown

int32 unknown

uint32 strLen

char unknownStr[strLen]

===
>>> function 3.1 <<<

uint32 assetPathListLen

#assetPathListLen {
    uint32 strLen
    char assetPath[strLen]
    uint32 assetId?
}

>>> function 3.2-3.10 (repeated 9 times call with diffrent args but allways strings) "writeLuaAssets()" <<<

uint32 listLen
#listLen {
    uint32 stringLen
    char str[stringLen]
    uint32 id
}

>>> function 4.1 <<<


! maybe {
    int32 ?
    uint32 len?
    #len { // (currently 0)
        int32 ?
    }

    uint32 len
    #len {
        uint32 innerLen
        #innerLen {
            ! struct {
                int32
                int32
                int32
                int64
                int64
                int8
                int8
                int8
                int8
                int8
            } (size 0x21)
        }

        int64
        int64
        int64
        int64
    }
}


*/

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



typedef struct {
    size_t len;
    void* data;
} TFList;

typedef struct {
    char* key;
    char* value;
} TFKeyValue;


TFKeyValue readTFKeyValue(char* buf, size_t* addr);
char* readTFString(char* buf, size_t* addr);