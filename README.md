# Process dumper

A proof of concept to dump the memory of a process directly to memory
instead of dumping to a file. This allows for custom operations such
as encrypting the process dump before writing to file or sending over
a network socket. Currently this program will invert the dump in memory
before writing the output to disk. Furthermore, in an attempt to avoid
crashing the targeted process when performing the dump, the process is first
cloned and then dumped.

## Usage
    .\pdump.exe -h
    Usage: C:\pdump.exe [options] <process name> <filepath>
      [options]
        -d, enable debug outprints

    e.g.,
    .\pdump.exe lsass.exe C:\windows\temp\output.log

## Compilation
The accompanying Makefile can be used to cross-compile this project from Linux
using MinGW. Note that the commands "sed" and "date" are required to calculate
the date to use for the built-in killswitch.


To compile the 64-bit binarie from linux:

    make build

