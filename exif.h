#ifndef __EXIF_H__
#define __EXIF_H__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint32_t bytes2integer(char *buffer, int n, int byteOrder)
{
    uint32_t x = 0;
    if (byteOrder) /* MM */
    {
        for (int i = 0; i < n; i++)
        {
            x |= (unsigned char)buffer[i] << (8 * (n - i - 1));
        }
    }
    else /* II */
    {
        for (int i = 0; i < n; i++)
        {
            x |= (unsigned char)buffer[i] << (8 * i);
        }
    }
    return x;
}

char *decodeTagEntries(char *tiff, int byteOrder, uint32_t offset)
{
    /*
    Number of Tags (2)
        Tag ID (2)
        Data Type (2)
        Number of Values (4)
        Value or Offset(4)
    */
    char *datatime = NULL;
    uint32_t n = bytes2integer(tiff + offset, 2, byteOrder);
    char *entry = tiff + offset + 2;
    for (uint32_t i = 0; i < n; i++)
    {
        uint32_t tag = bytes2integer(entry, 2, byteOrder);
        if (tag == 0x9003) /* DateTimeOriginal */
        {
            datatime = tiff + bytes2integer(entry + 8, 4, byteOrder);
            break;
        }
        else if (tag == 0x8769) /* Exif IFD Pointer */
        {
            offset = bytes2integer(entry + 8, 4, byteOrder);
            datatime = decodeTagEntries(tiff, byteOrder, offset);
            break;
        }
        entry += 12;
    }
    return datatime;
}

char *decodeEXIF(char *exif)
{
    char *datetime = NULL;
    if (!memcmp(exif, "Exif", 4))
    {
        char *tiff = exif + 6;
        /* TIFF header: MM|II + 002A + 0th IFD offset (uint32) */
        int byteOrder = 0;
        if (!memcmp(tiff, "MM", 2))
        {
            byteOrder = 1;
        }
        uint32_t ifd0Offset = bytes2integer(tiff + 4, 4, byteOrder);
        datetime = decodeTagEntries(tiff, byteOrder, ifd0Offset);
    }
    return datetime;
}

#endif
