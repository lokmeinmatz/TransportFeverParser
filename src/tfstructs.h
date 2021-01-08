
#include <stdint.h>
#include <stddef.h>

/*

TF2 file sructure


char[4] 'tf**'
int32_t saveVersion;
int32_t difficulty;
int32_t startYear;
int32_t tilesX;
int32_t tilesY;
uint32_t date;
int64_t money;


=== Mod Names ===

uint32_t modListLen;
#modListeLen x {
    uint32_t modNameLen;
    char modName[modNameLen];
    uint32_t severity?;
}

uint8_t achievements?;
int32_t unknown;
int32_t unknown;


=== UnknownList ===

uint32_t unknownList1Len;
#unknownList1Len x {
    uint8_t ?;
}


=== Mod Id List ===

uint32_t modIdListLen;
#modIdListLen x {
    uint32_t modIdLen;
    char modId[modIdLen];
    uint32_t severity?;
}


=== Game Settings List ===

uint32_t settingsListLen;
#settingsListLen x {
    uint32_t keyLen;
    char key[keyLen];
    uint32_t valueLen;
    char value[valueLen];
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