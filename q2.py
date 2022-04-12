#!/usr/bin/python3

# TODO: Do I really want to look for open files by their path?
#       How about looking by inode number? (I think it is a 1:1 relationship)
import subprocess
import argparse
import os
from pprint import pprint
# import psutil # implements process utilities without creating new process

# TODO: is this a good idea? (see my workaround next)
LSOF_FILE_PATH_INDEX = -1
LSOF_PID_INDEX = 1
LSOF_FD_INDEX = 3

parser = argparse.ArgumentParser()
parser.description = 'Given a file path, print info about all the processes that have this file opened'
parser.add_argument(
    'path', help='Path of file to be looked after in processes memory')
args = parser.parse_args()
path = args.path
print("Found the following processes that opened the file:", path)

with open(LOG_PATH, "w+") as outfile:
    subprocess.check_call(["lsof"],
                          shell=True,
                          stdout=outfile, stderr=subprocess.STDOUT)
    outfile.seek(0)
    headers = outfile.readline()
    for line in outfile.readlines():
        line = line.rstrip().split()
        try:
            process, pid, _, fd, *_, opened_file_path = line  # unpack list
            # opened_file_path = line[LSOF_FILE_PATH_INDEX]
            if opened_file_path == path:
                # fd = line[LSOF_FD_INDEX]
                if fd.endswith('u'):
                    fd = fd[:-1]  # u for unsinged number

                # pid = line[LSOF_PID_INDEX]
                with open(f"/proc/{pid}/fdinfo/{fd}") as fdinfo:
                    print(f"PID: {pid} Process Name : {process}\nattributes:")
                    for attr in fdinfo.readlines():
                        print(attr, end='')
                print('\n==============')
        except:
            pass  # might get here due to calling lsof
