#include <unistd.h>
// #include <include/platform.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <sys/mman.h>
// #include <errno.h>
// #include <ctype.h>
// #include <stdarg.h>

//procps_status_t
// #include <proc/readproc.h>

// #define UNUSED_PARAM(p) (void)(p)
int lsof_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	procps_status_t *proc = NULL;

	while ((proc = procps_scan(proc, PSSCAN_PID|PSSCAN_EXE)) != NULL) {
		char name[sizeof("/proc/%u/fd/0123456789") + sizeof(int)*3];
		unsigned baseofs;
		DIR *d_fd;
		char *fdlink;
		struct dirent *entry;

		if (getpid() == proc->pid)
			continue;

		baseofs = sprintf(name, "/proc/%u/fd/", proc->pid);
		d_fd = opendir(name);
		if (d_fd) {
			while ((entry = readdir(d_fd)) != NULL) {
				/* Skip entries '.' and '..' (and any hidden file) */
				if (entry->d_name[0] == '.')
					continue;

				safe_strncpy(name + baseofs, entry->d_name, 10);
				if ((fdlink = xmalloc_readlink(name)) != NULL) {
					printf("%d\t%s\t%s\n", proc->pid, proc->exe, fdlink);
					free(fdlink);
				}
			}
			closedir(d_fd);
		}
	}

	return EXIT_SUCCESS;
}