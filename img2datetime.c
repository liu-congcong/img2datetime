#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "heic.h"
#include "jpeg.h"

int printHelp()
{
    puts("img2datetime v1.0.0");
    puts("Rename images using their creation time.");
    puts("https://github.com/liu-congcong/img2datetime");
    puts("\nUsage:");
    puts("  img2datetime [options]");
    puts("\nOptions:");
    puts("  -i    Input directory containing the images.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char *input = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (!strncmp("-i", argv[i], 2)) input = argv[i + 1];
    }
    if (!input)
    {
        printHelp();
        exit(EXIT_FAILURE);
    }
    struct dirent *entry = NULL;
    struct stat statBuffer;
    char *ext = NULL;

    char *buffer1 = malloc(10240 * sizeof(char)); /* path to input img */
    char *buffer2 = malloc(10240 * sizeof(char)); /* path to output img */
    char buffer3[16]; /* datetime */

    DIR *dir = opendir(input);
    assert(dir);
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
        {
            snprintf(buffer1, 10240, "%s/%s", input, entry->d_name);
            if (stat(buffer1, &statBuffer) != -1)
            {
                if (!S_ISDIR(statBuffer.st_mode))
                {
                    memset(buffer3, 0, 16);
                    ext = strrchr(entry->d_name, '.');
                    if (!strncasecmp(ext, ".jpg", 4) || !strncasecmp(ext, ".jpeg", 5)) readJPEG(buffer1, buffer3);
                    else if (!strncasecmp(ext, ".heic", 5)) readHEIC(buffer1, buffer3);
                    else continue;
                    if (strlen(buffer3) && strncmp(entry->d_name, buffer3, 15))
                    {
                        snprintf(buffer2, 10240, "%s/%s%s", input, buffer3, ext);
                        if (access(buffer2, F_OK))
                        {
                            printf("%s -> %s\n", buffer1, buffer2);
                            rename(buffer1, buffer2);
                        }
                    }
                }
            }
        }
    }
    free(buffer1);
    free(buffer2);
    closedir(dir);
    return 0;
}
