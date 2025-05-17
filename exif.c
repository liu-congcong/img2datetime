#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exif.h"
#include "utilities.h"

unsigned char *decodeTagEntries(unsigned char *tiff, int byteOrder, uint32_t offset)
{
    /*
    Number of Tags (2)
        Tag ID (2)
        Data Type (2)
        Number of Values (4)
        Value or Offset(4)
    */
    unsigned char *tag9003 = NULL; /* DateTimeOriginal */
    unsigned char *tag0132 = NULL; /* DateTime */
    uint16_t n = bytes2integer(tiff + offset, 2, byteOrder);
    unsigned char *entry = tiff + offset + 2;
    for (uint16_t i = 0; i < n; i++)
    {
        uint16_t tag = bytes2integer(entry, 2, byteOrder);
        if (tag == 0x9003)
        {
            tag9003 = tiff + bytes2integer(entry + 8, 4, byteOrder);
            break;
        }
        else if (tag == 0x0132) /* DateTime */
        {
            tag0132 = tiff + bytes2integer(entry + 8, 4, byteOrder);
        }
        else if (tag == 0x8769) /* Exif IFD Pointer */
        {
            offset = bytes2integer(entry + 8, 4, byteOrder);
            tag9003 = decodeTagEntries(tiff, byteOrder, offset);
            break;
        }
        entry += 12;
    }
    return tag9003 ? tag9003 : tag0132;
}

int decodeEXIF(unsigned char *exif, char *datetime)
{
    if (!memcmp(exif, "Exif", 4))
    {
        unsigned char *tiff = exif + 6;
        /* TIFF header: MM|II + 002A + 0th IFD offset (uint32) */
        int byteOrder = 0;
        if (!memcmp(tiff, "MM", 2)) byteOrder = 1;
        uint32_t ifd0Offset = bytes2integer(tiff + 4, 4, byteOrder);
        unsigned char *x = decodeTagEntries(tiff, byteOrder, ifd0Offset);
        if (x) sprintf(datetime, "%.4s%.2s%.2s-%.2s%.2s%.2s", x, x + 5, x + 8, x + 11, x + 14, x + 17);
    }
    return 0;
}
