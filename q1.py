# TODO: Use pydantic / attrs / dataclass / namedtuple for a more generic parser
#       (parsing columns by their names altough this is not a csv file)
#       address           perms offset  dev   inode   pathname
import argparse
import pathlib

WIDTH = 20
HEADERS = "address           perms offset  dev   inode   pathname".split()
NUM_OF_COLS = 7
parser = argparse.ArgumentParser()
parser.description = 'Print virtual memory address space for a given process'
parser.add_argument(
    'pid', help='process id of the process you wish to examine')
args = parser.parse_args()

# TODO: I dont like this method , cant even use variables/macros
format_row = "{:<20}" * NUM_OF_COLS  # or use ljust/rjust ?

# path is linux specific (a known special folder)
process_vmmap_path = pathlib.Path(f"/proc/{args.pid}/maps")
print(format_row.format("", *HEADERS))

# TODO: does this way uses a generator?
# read file content
with open(process_vmmap_path) as f:
    for line in f:
        line = line.split()
        try:
            addr_range, permissions, offset, device, inode_num, file_path = line
            if file_path.startswith("/"):
                print(format_row.format("", *line).strip())
        except ValueError:
            pass  # not an interesting row


# parser.add_argument('start', help='Starting virtual address')
# parser.add_argument('size', help='Size of the address space, in bytes')
# parser.add_argument('flags', nargs='?', type=str, default='', help='Flags set by the ELF file, see PF_X, PF_R, PF_W')
# parser.add_argument('offset', nargs='?', default=0, help='Offset into the original ELF file that the data is loaded from')
