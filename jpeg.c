#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exif.h"
#include "jpeg.h"
#include "utilities.h"

#define SOI "\xFF\xD8"
#define APP0 "\xFF\xE0"
#define APP1 "\xFF\xE1"

int readJPEG(char *file, char *datetime)
{
    unsigned char *buffer = malloc(0xFFFF * sizeof(unsigned char));
    uint64_t size;
    FILE *fp = fopen(file, "rb");
    assert(fp);
    fread(buffer, 2, 1, fp);
    assert(!memcmp(buffer, SOI, 2));
    fread(buffer, 4, 1, fp); /* app0-1 (2B), size (2B) */
    if (!memcmp(buffer, APP0, 2))
    {
        size = bytes2integer(buffer + 2, 2, 1);
        fseek(fp, size - 2, SEEK_CUR);
        fread(buffer, 4, 1, fp); /* app0-1 (2B), size (2B) */
    }
    if (!memcmp(buffer, APP1, 2))
    {
        size = bytes2integer(buffer + 2, 2, 1);
        fread(buffer, size - 2, 1, fp);
        decodeEXIF(buffer, datetime);
    }
    fclose(fp);
    free(buffer);
    return 0;
}
