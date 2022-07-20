This is a course project for CS 537 in University of Wisconsin, Madison. 

### Brief Introduction

An ext2 disk image is found which contain some important jpg files. Some of them have already been deleted (whose inodes are marked "unused"), but fortunately the entire disk is not damaged and the inode contents and file data are still intact.

I'm required to scan through the disk image to reconstruct all the jpg files inside it. This will be done by first reading the super-node, then locate the inode tables and scan through them to find those inodes who represent a jpg file (a jpg file çan be identified by looking at the file magic number stored in the first data block). Then locate the data blocks and read all the values out byte by byte, and finally write them to a pre-specified directory.

The detailed project requirements are specified in the file [Project_5_File_systems.pdf](https://github.com/Alexanderia-Mike/cs537-p5/blob/main/Project_5_File_systems.pdf).


### Usage

1. go to the directory `src/`

2. type `make` to compile

3. type `./runScan [DISK_IMAGE_FILE] [OUT_DIR]`

Then all the deleted and undeleted jpg images stored inside the file `[DISK_IMAGE_FILE]` will be put into the directory `[OUT_DIR]`.

### Limitations:

1. the size of the images should be no larger than 64.26 MB.
2. 
