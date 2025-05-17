#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exif.h"
#include "heic.h"
#include "utilities.h"

#ifdef _WIN32
#define fseek64 _fseeki64
#else
#define fseek64 fseeko
#endif

typedef struct
{
    /* size (4B), type (4B), largesize (0B, 8B) */
    uint64_t size;
    unsigned char type[4];
    uint64_t offset;
} BasicBoxHeader;

typedef struct
{
    /* size (4B), type (4B), largesize (0B, 8B), version (1B), flags (3B) */
    uint64_t size;
    unsigned char type[4];
    unsigned char version;
    uint64_t offset;
} FullBoxHeader;

typedef struct
{
    /* size (4B), type (4B), largesize (0B, 8B), version (1B), flags (3B), item_count (2B, 4B) */
    uint64_t size;
    unsigned char version;
    uint32_t itemCount;
    uint64_t offset;
} IINFBoxHeader;

typedef struct
{
    /*
    size (4B), type (4B), largesize (0B, 8B), version (1B), flags (3B),
    item_id (v0-v2:2B, >=v3:4B), item_protection_index (2B), item_type (>=v2:4B)
    */
    uint64_t size;
    uint32_t itemID;
    unsigned char itemType[4];
} INFEBox;

static int readBasicBoxHeader(unsigned char *buffer, BasicBoxHeader *header)
{
    int offset = 8;
    header->size = bytes2integer(buffer, 4, 1);
    memcpy(header->type, buffer + 4, 4);
    if (header->size == 1)
    {
        header->size = bytes2integer(buffer + 8, 8, 1);
        offset = 16;
    }
    header->offset = offset;
    return 0;
}

static int readFullBoxHeader(unsigned char *buffer, FullBoxHeader *header)
{
    int offset = 8;
    header->size = bytes2integer(buffer, 4, 1);
    memcpy(header->type, buffer + 4, 4);
    if (header->size == 1)
    {
        header->size = bytes2integer(buffer + 8, 8, 1);
        offset = 16;
    }
    header->version = buffer[offset];
    header->offset = offset + 4;
    return 0;
}

static int readIINFBoxHeader(unsigned char *buffer, IINFBoxHeader *header)
{
    int offset = 8;
    header->size = bytes2integer(buffer, 4, 1);
    if (header->size == 1)
    {
        header->size = bytes2integer(buffer + 8, 8, 1);
        offset = 16;
    }
    header->version = buffer[offset];
    int itemCountSize = 2;
    if (header->version) itemCountSize = 4;
    header->itemCount = bytes2integer(buffer + offset + 4, itemCountSize, 1);
    header->offset = offset + 4 + itemCountSize;
    return 0;
}

static int readINFEBox(unsigned char *buffer, INFEBox *infe)
{
    /*
    size (4B), type (4B), largesize (0B, 8B), version (1B), flags (3B),
    item_id (v0-v2:2B, >=v3:4B), item_protection_index (2B), item_type (>=v2:4B)
    */
    int offset = 8;
    infe->size = bytes2integer(buffer, 4, 1);
    if (infe->size == 1)
    {
        infe->size = bytes2integer(buffer + 8, 8, 1);
        offset = 16;
    }
    unsigned char version = buffer[offset];
    if (version >= 2)
    {
        int itemIDSize = (version == 2 ? 2 : 4);
        infe->itemID = bytes2integer(buffer + offset + 4, itemIDSize, 1);
        memcpy(infe->itemType, buffer + offset + itemIDSize + 6, 4);
    }
    else memset(infe->itemType, 0, 4);
    return 0;
}

static int readIINFBox(unsigned char *buffer)
{
    IINFBoxHeader iinfBoxHeader;
    readIINFBoxHeader(buffer, &iinfBoxHeader);
    uint64_t offset = iinfBoxHeader.offset;
    for (uint32_t i = 0; i < iinfBoxHeader.itemCount; i++)
    {
        INFEBox infeBox;
        readINFEBox(buffer + offset, &infeBox);
        if (!memcmp(infeBox.itemType, "Exif", 4)) return infeBox.itemID;
        offset += infeBox.size;
    }
    return 0;
}

static unsigned char *readILOCBox(unsigned char *buffer, uint32_t exifID, uint64_t *exifOffset, uint64_t *exifLength)
{
    /*
    size (4B), type (4B), largesize (0B, 8B),
    version (1B), flags (3B),
    offset_size (4b), length_size (4b), base_offset_size(4b), index_size(>=v1:4b, 4b), item_count (2B) ???
        item_ID (2B)
        construction_method (>=v1:2B)
        data_reference_index (2B)
        base_offset (base_offset_size B)
        extent_count (2B)
            extent_index (>=v1 & construction_method == 1: index_size B)
            extent_offset (offset_size B)
            extent_length (length_size B)
    */
    FullBoxHeader fullBoxHeader;
    readFullBoxHeader(buffer, &fullBoxHeader);
    uint64_t offset = fullBoxHeader.offset;
    unsigned char version = fullBoxHeader.version;
    unsigned char offsetSize = buffer[offset] >> 4;
    unsigned char lengthSize = buffer[offset] & 0x0F;
    offset++;
    unsigned char baseOffsetSize = buffer[offset] >> 4;
    unsigned char indexSize = version ? buffer[offset] & 0x0F : 0;
    offset++;
    uint16_t itemCount = bytes2integer(buffer + offset, 2, 1);
    offset += 2;

    for (uint16_t i = 0; i < itemCount; i++)
    {
        uint64_t extentOffset = 0;
        uint64_t extentLength = 0;
        uint16_t constructionMethod = 0;
        uint32_t itemID = bytes2integer(buffer + offset, 2, 1);
        offset += 2; /* item_ID (2B) */
        if (version)
        {
            constructionMethod = bytes2integer(buffer + offset, 2, 1);
            offset += 2; /* construction_method (2B) */
        }
        offset += 2; /* data_reference_index (2B) */
        uint64_t baseOffset = bytes2integer(buffer + offset, baseOffsetSize, 1);
        offset += baseOffsetSize; /* base_offset */
        uint16_t extentCount = bytes2integer(buffer + offset, 2, 1);
        offset += 2; /* extent_count */
        for (uint16_t j = 0; j < extentCount; j++)
        {
            if (constructionMethod == 1) offset += indexSize; /* extent_index */
            extentOffset = bytes2integer(buffer + offset, offsetSize, 1);
            offset += offsetSize;
            extentLength = bytes2integer(buffer + offset, lengthSize, 1);
            offset += lengthSize;
        }
        if (itemID == exifID && !constructionMethod)
        {
            *exifOffset = baseOffset + extentOffset;
            *exifLength = extentLength;
            break;
        }
    }
    return 0;
}

static int metaBox(unsigned char *buffer, BasicBoxHeader *basicBoxHeader, uint64_t *exifOffset, uint64_t *exifLength)
{
    uint64_t offset = basicBoxHeader->offset + 4; /* size (4B), type (4B), largesize (0B, 8B), version (1B), flags (3B) */
    uint32_t exifID = 0;
    char *exif = NULL;
    FullBoxHeader fullBoxHeader;
    while (offset < basicBoxHeader->size)
    {
        readFullBoxHeader(buffer + offset, &fullBoxHeader);
        /* printf("type: %s, size: %llu\n", fullBoxHeader.type, fullBoxHeader.size); */
        if (!memcmp(fullBoxHeader.type, "iinf", 4)) exifID = readIINFBox(buffer + offset);
        else if (!memcmp(fullBoxHeader.type, "iloc", 4))
        {
            readILOCBox(buffer + offset, exifID, exifOffset, exifLength);
            break;
        }
        offset += fullBoxHeader.size;
    }
    return 0;
}

int readHEIC(char *file, char *datetime)
{
    unsigned char buffer16[16];
    BasicBoxHeader basicBoxHeader;
    uint64_t exifOffset = 0;
    uint64_t exifLength = 0;
    FILE *fp = fopen(file, "rb");
    assert(fp);
    while (fread(buffer16, 16, 1, fp))
    {
        readBasicBoxHeader(buffer16, &basicBoxHeader);
        if (basicBoxHeader.size == 0) break;
        fseek(fp, -16, SEEK_CUR);
        if (!memcmp(basicBoxHeader.type, "meta", 4))
        {
            unsigned char *buffer = malloc(basicBoxHeader.size * sizeof(unsigned char));
            fread(buffer, basicBoxHeader.size, 1, fp);
            metaBox(buffer, &basicBoxHeader, &exifOffset, &exifLength);
            free(buffer);
            break;
        }
        fseek(fp, basicBoxHeader.size, SEEK_CUR);
    }
    if (exifOffset && exifLength)
    {
        fseek64(fp, exifOffset, SEEK_SET);
        char *buffer = malloc(exifLength * sizeof(char));
        fread(buffer, exifLength, 1, fp);
        decodeEXIF(buffer + 4, datetime);
        free(buffer);
    }
    fclose(fp);
    return 0;
}