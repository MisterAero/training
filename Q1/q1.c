//TODO: this code looks like a total disaster (Code duplication)
//      How can I make it better?
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// single arg which is pid
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    int pid = atoi(argv[1]); // convert string to int, unsafe! use strtol() instead

    // if not a valid pid, return
    if (pid <= 0)
    {
        printf("Invalid pid: %d\n", pid);
        return 1;
    }

    // open the matching /proc/pid/maps api
    char path[64];
    sprintf(path, "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f)
    {
        printf("Couldn't open %s\n", path);
        return 1;
    }

    //use fgets to read line by line and parse it
    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        // get the base address of the driver
        char *base_addr = strtok(line, "-");
        if (base_addr == NULL)
        {
            printf("Couldn't parse base address\n");
            continue;
        }

        // get the end address of the driver
        char *end_addr = strtok(NULL, " ");
        if (end_addr == NULL)
        {
            printf("Couldn't parse end address\n");
            continue;
        }

        // permissions
        char *perms = strtok(NULL, " ");
        if (perms == NULL)
        {
            printf("Couldn't parse permissions\n");
            continue;
        }

        // offset
        char *offset = strtok(NULL, " ");
        if (offset == NULL)
        {
            printf("Couldn't parse offset\n");
            continue;
        }

        // device
        char *device = strtok(NULL, " ");
        if (device == NULL)
        {
            printf("Couldn't parse device\n");
            continue;
        }

        // inode
        char *inode = strtok(NULL, " ");
        if (inode == NULL)
        {
            
            printf("Couldn't parse inode\n");
            continue;
        }

        // path
        // Note: using a delimiter of \n didn't work... WHY?
        char *path = strtok(NULL, " ");
        if (path == NULL)
        {
            printf("Couldn't parse path\n");
            continue;
        }

        // if path is not file, (does'nt start with / or .), skip
        if (path[0] != '/' && path[0] != '.')
        {
            continue;
        }

        //print all the info in one line
        printf("%s-%s %s %s %s %6s ---> %s", base_addr, end_addr, perms, offset, device, inode, path);
    }

    return 0;
}

