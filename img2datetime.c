#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "exif.h"

#define SOI "\xff\xd8"
#define APP0 "\xff\xe0"
#define APP1 "\xff\xe1"

int readJPEG(char *file, char *datetime)
{
    char *buffer = malloc((0xffff + 1) * sizeof(char));
    int flag = 0;
    uint32_t size = 0;
    FILE *openFile = fopen(file, "rb");
    if (openFile)
    {
        fread(buffer, 2, 1, openFile);
        if (!memcmp(buffer, SOI, 2))
        {
            fread(buffer, 4, 1, openFile);
            if (!memcmp(buffer, APP0, 2))
            {
                size = bytes2integer(buffer + 2, 2, 1);
                fseek(openFile, size - 2, SEEK_CUR);
                fread(buffer, 4, 1, openFile);
            }
            if (!memcmp(buffer, APP1, 2))
            {
                size = bytes2integer(buffer + 2, 2, 1);
                fread(buffer, size - 2, 1, openFile);
                char *i = decodeEXIF(buffer);
                if (i)
                {
                    sprintf(datetime, "%.4s%.2s%.2s-%.2s%.2s%.2s", i, i + 5, i + 8, i + 11, i + 14, i + 17);
                    flag = 1;
                }
            }
            else
            {
                printf("No EEIF was found: \"%s\"\n", file);
            }
        }
    }
    else
    {
        printf("Open \"%s\" failed.\n", file);
    }
    fclose(openFile);
    free(buffer);
    return flag;
}


int readHEIC(char *file, char *datetime)
{
    char *buffer = malloc((0xffff + 1) * sizeof(char));
    int flag = 0;
    uint32_t size = 0;
    FILE *openFile = fopen(file, "rb");
    if (openFile)
    {
        uint32_t size = 0;
        while (fread(buffer, 8, 1, openFile))
        {
            size = bytes2integer(buffer, 4, 1);
            if (!memcmp(buffer + 4, "mdat", 4))
            {
                fread(buffer, 0xffff, 1, openFile);
                char *i = decodeEXIF(buffer + 12); // ???????????????
                if (i)
                {
                    sprintf(datetime, "%.4s%.2s%.2s-%.2s%.2s%.2s", i, i + 5, i + 8, i + 11, i + 14, i + 17);
                    flag = 2;
                }
                break;
            }
            fseek(openFile, size - 8, SEEK_CUR);
        }
    }
    else
    {
        printf("Open \"%s\" failed.\n", file);
    }
    fclose(openFile);
    free(buffer);
    
    flag = 0;

    return flag;
}

int printHelp(char *self)
{
    puts("\nRename photos using created time v0.1 (https://github.com/liu-congcong/img2datetime)");
    puts("Usage:");
    printf("    %s --input <str>.\n", self);
    puts("Options:");
    puts("    -i/--input: the directory containing the photos.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if ((argc != 3) || strncasecmp("-i", argv[1], 2))
    {
        printHelp(argv[0]);
        exit(0);
    }
    DIR *openDirectory = opendir(argv[2]);
    if (openDirectory)
    {
        struct dirent *entry = NULL;
        struct stat statBuffer;
        char *buffer1 = malloc(1024 * 1024 * sizeof(char));
        char *buffer2 = malloc(16 * sizeof(char));
        char *buffer3 = malloc(1024 * 1024 * sizeof(char));
        int flag = 0;
        while ((entry = readdir(openDirectory)) != NULL)
        {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                snprintf(buffer1, 1024 * 1024, "%s/%s", argv[2], entry->d_name);
                if (stat(buffer1, &statBuffer) != -1)
                {
                    if (!S_ISDIR(statBuffer.st_mode))
                    {
                        flag = readJPEG(buffer1, buffer2);
                        if (flag)
                        {
                            snprintf(buffer3, 1024 * 1024, "%s/%s.%s", argv[2], buffer2, flag == 1 ? "jpeg" : "heic");
                            if (strcasecmp(buffer1, buffer3))
                            {
                                if (access(buffer3, F_OK))
                                {
                                    printf("%s -> %s\n", buffer1, buffer3);
                                    rename(buffer1, buffer3);
                                }
                                else
                                {
                                    printf("%s |> %s\n", buffer1, buffer3);
                                }
                            }
                        }
                    }
                }
            }
        }
        free(buffer1);
        free(buffer2);
        free(buffer3);
    }
    else
    {
        printf("Open \"%s\" failed.\n", argv[2]);
    }
    closedir(openDirectory);
    return 0;
}
