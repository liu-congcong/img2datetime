#include <stdint.h>

uint64_t bytes2integer(unsigned char *buffer, int n, int byteOrder)
{
    uint64_t x = 0;
    if (byteOrder) /* MM */
    {
        for (int i = 0; i < n; i++) x |= buffer[i] << (8 * (n - i - 1));
    }
    else /* II */
    {
        for (int i = 0; i < n; i++) x |= buffer[i] << (8 * i);
    }
    return x;
}
