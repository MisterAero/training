/*
 * Copyright (c) 2010, The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <unistd.h> //
#include <dirent.h> // DIR, struct dirent, opendir(), readdir(), closedir()
#include <errno.h>
#include <fcntl.h>  // O_RDONLY, O_NONBLOCK, open(), close(), fcntl(), F_GETFL, F_SETFL
#include <libgen.h> // basename(), dirname(), strdup(), strndup(), strlen()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // getpid(), getcwd(), readlink(), lstat(), stat() , unlink(),
#include <pwd.h>       // struct passwd, getpwuid(), getpwnam(), getpwent(), endpwent(), setpwent()
#include <sys/stat.h>  // struct stat, stat() , lstat(), S_ISDIR(), S_ISLNK(), S_ISREG(), S_ISFIFO(), S_ISSOCK()
#include <sys/types.h> // ssize_t, off_t, size_t, mode_t, uid_t, gid_t, pid_t
#include <limits.h>    // PATH_MAX
#include <stddef.h>    // size_t, NULL, offsetof(), memset(), memcpy(), memcmp()
// #include <ctype.h>

#define BUF_MAX 1024
#define CMD_DISPLAY_MAX (9 + 1)
#define USER_DISPLAY_MAX (10 + 1)

// info about a running process
struct pid_info_t
{
    pid_t pid;                     // process id
    char user[USER_DISPLAY_MAX];   // user name
    char cmdline[CMD_DISPLAY_MAX]; // command line
    char path[PATH_MAX];           // path
    ssize_t parent_length;         // length of parent path string ("/proc/1234/")
};

void print_header()
{
    printf("%-9s %5s %10s %4s %9s %18s %9s %10s %s\n",
           "COMMAND",
           "PID",
           "USER",
           "FD",
           "TYPE",
           "DEVICE",
           "SIZE/OFF",
           "NODE",
           "NAME");
}

void print_type(char *type, struct pid_info_t *info)
{
    static ssize_t link_dest_size;
    static char link_dest[PATH_MAX];
    strncat(info->path, type, sizeof(info->path));
    if ((link_dest_size = readlink(info->path, link_dest, sizeof(link_dest) - 1)) < 0)
    {
        if (errno == ENOENT)
            goto out;
        snprintf(link_dest, sizeof(link_dest), "%s (readlink: %s)", info->path, strerror(errno));
    }
    else
    {
        link_dest[link_dest_size] = '\0';
    }
    // Things that are just the root filesystem are uninteresting (we already know)
    if (!strcmp(link_dest, "/"))
        goto out;
    printf("%-9s %5d %10s %4s %9s %18s %9s %10s %s\n",
           info->cmdline, info->pid, info->user, type,
           "???", "???", "???", "???", link_dest);
out:
    info->path[info->parent_length] = '\0';
}

// Prints out all file that have been memory mapped
void print_maps(struct pid_info_t *info)
{
    FILE *maps;
    char buffer[PATH_MAX + 100];
    size_t offset;
    int major, minor;
    char device[10];
    long int inode;
    char file[PATH_MAX];
    strncat(info->path, "maps", sizeof(info->path));
    maps = fopen(info->path, "r");
    if (!maps)
        goto out;
    while (fscanf(maps, "%*x-%*x %*s %zx %5s %ld %s\n", &offset, device, &inode,
                  file) == 4)
    {
        // We don't care about non-file maps
        if (inode == 0 || !strcmp(device, "00:00"))
            continue;
        printf("%-9s %5d %10s %4s %9s %18s %9zd %10ld %s\n",
               info->cmdline, info->pid, info->user, "mem",
               "???", device, offset, inode, file);
    }
    fclose(maps);
out:
    info->path[info->parent_length] = '\0';
}

// Prints out all open file descriptors
void print_fds(struct pid_info_t *info)
{
    static char *fd_path = "fd/";
    strncat(info->path, fd_path, sizeof(info->path));
    int previous_length = info->parent_length;
    info->parent_length += strlen(fd_path);
    DIR *dir = opendir(info->path);
    if (dir == NULL)
    {
        char msg[BUF_MAX];
        snprintf(msg, sizeof(msg), "%s (opendir: %s)", info->path, strerror(errno));
        printf("%-9s %5d %10s %4s %9s %18s %9s %10s %s\n",
               info->cmdline, info->pid, info->user, "FDS",
               "", "", "", "", msg);
        goto out;
    }
    struct dirent *de;
    while ((de = readdir(dir)))
    {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        print_type(de->d_name, info);
    }
    closedir(dir);
out:
    info->parent_length = previous_length;
    info->path[info->parent_length] = '\0';
}

void lsof_dumpinfo(pid_t pid)
{
    int fd; 
    struct pid_info_t info;
    struct stat pidstat;
    struct passwd *pw;
    info.pid = pid;
    snprintf(info.path, sizeof(info.path), "/proc/%d/", pid);
    info.parent_length = strlen(info.path);
    
    // Get the UID by calling stat on the proc/pid directory.
    // (get the file attributes (status/stats) and pour it into the stat structure,
    // which is treated as a buffer
    if (!stat(info.path, &pidstat))
    {
        pw = getpwuid(pidstat.st_uid); // get user database entry
        if (pw)
        {
            // copy user name to info.user
            strlcpy(info.user, pw->pw_name, sizeof(info.user)); 
        }
        else
        {
            // copy uid of the file owner to info.user
            snprintf(info.user, USER_DISPLAY_MAX, "%d", (int)pidstat.st_uid); 
        }
    }
    else
    {
        strcpy(info.user, "???");
    }
    // Read the command line information; each argument is terminated with NULL.
    strncat(info.path, "cmdline", sizeof(info.path));
    fd = open(info.path, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Couldn't read %s\n", info.path);
        return;
    }
    char cmdline[PATH_MAX];
    int numRead = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    if (numRead < 0)
    {
        fprintf(stderr, "Error reading cmdline: %s: %s\n", info.path, strerror(errno));
        return;
    }
    cmdline[numRead] = '\0';
    // We only want the basename of the cmdline
    strlcpy(info.cmdline, basename(cmdline), sizeof(info.cmdline));
    // Read each of these symlinks
    print_type("cwd", &info);
    print_type("exe", &info);
    print_type("root", &info);
    print_fds(&info);
    print_maps(&info);
}

// int lsof_main(int argc, char *argv[])
int main(int argc, char *argv[])
{
    long int pid = 0;
    char *endptr; // Used to check if the pid is a number
    if (argc == 2)
    {
        pid = strtol(argv[1], &endptr, 10);
    }
    print_header();

    if (pid)
    {
        lsof_dumpinfo(pid);
    }
    else
    {
        // If no pid was specified, we'll just dump all the processes
        DIR *dir = opendir("/proc");
        if (dir == NULL)
        {
            fprintf(stderr, "Couldn't open /proc\n");
            return -1;
        }
        struct dirent *de; // Used to read the directory content (like an iterator)
        while ((de = readdir(dir)))
        {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue; // Skip . and ..
            // Only inspect directories that are PID numbers
            pid = strtol(de->d_name, &endptr, 10);
            if (*endptr != '\0')
                continue;       // Not a number, skip
            lsof_dumpinfo(pid); // dump info for current pid
        }
        closedir(dir); // cleanup (proc)
    }
    return 0;
}