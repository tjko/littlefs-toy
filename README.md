# littlefs-toy
A free tool for working with [LittleFS](https://github.com/littlefs-project/littlefs) images.

**lfs** is a command-line utility that makes is easy to create and modify existing LittleFS images

  - Create and modify LittleFS images with similar command interface as _tar_.
  - Allows adding, updating, and deleting files and directories on a LittleFS filesystem.
  - Automatically detects filesystem blocksize and size when working on existing filesystem images.
  - Can edit LittleFS filesystem that is at arbitrary offset inside another file/image (makes it easy to work on LittleFS images emmbedded inside a firmware image, etc.)

# Usage

**lfs** works in similar fashion as **tar** command on Linux/Unix systems.

## Command Summary
```
Usage: lfs {command} [options] [(file) | (pattern) ...]

 Commands:
  -c, --create               Create (format) LFS image and add files
  -r, --append               Append (add) files to existing LFS image
  -d, --delete               Remove files from existing LFS image
  -t, --list                 List contents of existing LFS image
  -x, --extract              Extract files from existing LFS image

 Options:
 -f <imagefile>, --file=<imagefile>
                             Specify LFS image file location
 -b <blocksize>, --block-size=<blocksize>
                             LFS filesystem blocksize (default: 4096)
 -s <imagesize>, --size=<imagesize>
                             LFS filesystem size (required with -c)
 -o <imageoffset>, --offset=<imageoffset>
                             LFS filesystem start offset (default: 0)
 -h, --help                  Display usage information and exit
 -v, --verbose               Enable verbose mode
 -V, --version               Display program version
 -O, --overwrite             Overwrite image file (if it exists already)
 --direct                    Write to image file directly (use less memory)
 --shrink                    Truncate image file at the end of LFS image
```

## Usage Examples

### Create new LittleFS Image

Create a new 2Mb LittleFS filesystem image and add files to it (uses default 4Kb block size): 
```
$ lfs -c -v -f lfs.img -s 2M config.txt fw.bin
config.txt
fw.bin
```

### View contents of a exisint LittleFS Image

View contents of existing LittleFS image:
```
$ lfs -t -f lfs.img
./config.txt
./fw.bin
```

To also view file sizes (-v) option can be used:
```
$ lfs -t -v -f lfs.img
-rw-rw-rw- root/root      1876 0000-00-00 00:00 ./config.txt
-rw-rw-rw- root/root    262144 0000-00-00 00:00 ./fw.bin
```
(LittleFS does not store file permissions, ownership or timestamps, so these are "blank" in the output to keep format similar to _tar_ output)

### Extract files from an existing LittleFS Image

To extract individual files we can list their names:
```
$ lfs -x -v -f lfs.img config.txt
./config.txt
```

To extract all files into /tmp directory:
```
$ lfs -x -v -f lfs.img -C /tmp .
./config.txt
./fw.bin
```

### Working on LittleFS imaged embedded inside another file or image

It is possible to use (-o) option to specify offset from beginning of the
image file where the LittleFS image starts. Offset can be specified either in decimal
hexadecimal format.

Extract files from a 256K LittleFS at the end of 2Mb firmware dump into /tmp directory:
```
$ lfs -x -v -f flash.dump -o 0x1c0000 -C /tmp
./cert.pem
./fanpico.cfg
./key.pem
./ssh-ecdsa.der
./ssh-ed25519.der
```

Add/replace file in the LittleFS embbedded in the flash dump:
```
$ lfs -r -v -f flash.dump -o 0x1c0000  fanpico.cfg
./fanpico.cfg
```

# Compiling

Currently **lfs** is being developed for Linux and MacOS, but it shouldn't be too hard to port for other operating systems.

Basic development tools including CMake and C compiler is required.

## Get Sources

First fetch sources and required libraries:
```
$ git clone https://github.com/tjko/littlefs-toy.git
$ cd littlefs-toy
$ git submodule update --init --recurisve
```

## Compile Binaries

Next compile using _cmake_:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Installation

After _lfs_ has been successfully compiled, it can be installed:
```
$ sudo make install
```


 


