#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "heic.h"
#include "jpeg.h"

int printHelp()
{
    puts("img2datetime v1.0.1");
    puts("Rename images using their creation time.");
    puts("https://github.com/liu-congcong/img2datetime");
    puts("\nUsage:");
    puts("  img2datetime [options]");
    puts("\nOptions:");
    puts("  -i    Input directory containing the images.");
    puts("  -o    Output directory. Default: input directory.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char *input = NULL;
    char *output = NULL;
    for (int i = 1; i < argc - 1; i++)
    {
        if (!strncmp("-i", argv[i], 2)) input = argv[i + 1];
        else if (!strncmp("-o", argv[i], 2)) output = argv[i + 1];
    }
    if (!input)
    {
        printHelp();
        exit(EXIT_FAILURE);
    }
    if (!output) output = input;

    struct dirent *entry = NULL;
    struct stat statBuffer;
    char *ext = NULL;

    char *buffer1 = malloc(10240 * sizeof(char)); /* path to input img */
    char *buffer2 = malloc(10240 * sizeof(char)); /* path to output img */
    char buffer3[16]; /* datetime */
    struct timespec ts;
    long long nanoseconds;

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
                    strcpy(buffer3, "00000000-000000");
                    ext = strrchr(entry->d_name, '.');
                    if (!strncasecmp(ext, ".jpg", 4) || !strncasecmp(ext, ".jpeg", 5)) readJPEG(buffer1, buffer3);
                    else if (!strncasecmp(ext, ".heic", 5)) readHEIC(buffer1, buffer3);
                    else continue;
                    timespec_get(&ts, TIME_UTC);
                    nanoseconds = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
                    snprintf(buffer2, 10240, "%s/%s %lld%s", output, buffer3, nanoseconds, ext);
                    assert(access(buffer2, F_OK));
                    printf("%s -> %s\n", buffer1, buffer2);
                    assert(!rename(buffer1, buffer2));
                }
            }
        }
    }
    free(buffer1);
    free(buffer2);
    closedir(dir);
    return 0;
}
