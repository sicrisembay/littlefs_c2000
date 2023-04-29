# LittleFS C2000
This is an example LittleFS project.  TMS320F28335 is used as a target microcontroller.
This microcontroller has a byte definition of 16 bits.

```
sizeof(byte)  == 1
sizeof(short) == 1
sizeof(long)  == 2
```

## Hardware Test Setup
The setup is composed of C2000 DIMM100 Experimenter's kit with TMDSCNCD28335 control card and CY15FRAMKIT.


<p align="center">
  <img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/TMDSDOCK28335.png">
</p>

CY15FRAMKIT has a Cypress FM25W256 FRAM chip.  This chip has 32kB capacity and communicates to C2000 via SPI bus.


<p align="center">
  <img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/CY15FRAMKIT-001.jpg">
</p>

Connection between C2000 Experimeter kit and CY15FRAMKIT is shown below.


| Function    | C2000 Experimenter Kit | CY15FRAMKIT |
|-------------|------------------------|-------------|
| Chip Select | GPIO15                 | CS          |
| Clock       | GPIO14                 | SCK         |
| MOSI        | GPIO12                 | SI          |
| MISO        | GPIO13                 | SO          |
| Power       | 3V3                    | 3V3         |
| Power       | GND                    | GND         |


## LittleFS Configuration for FRAM
For this test, LittleFS configuration for this specific external FRAM storage is shown below.

| LFS Configuration | Value |
|-------------------|-------|
| read_size         | 1     |
| prog_size         | 1     |
| block_size        | 128   |
| block_count       | 256   |
| block_cycle       | 500   |
| cache_size        | 64    |
| lookahead_size    | 32    |



## Test
In this example, SysBIOS version 6.83 is used.  Most of the test is done using a command line interface.  For this, a port of
FreeRTOS-Plus-CLI to SysBIOS is used.


Some very useful CLI commands are:


| Command                  | Description                                                                                                                                    |
|--------------------------|------------------------------------------------------------------------------------------------------------------------------------------------|
| lfs_dump [block_id]      | This dumps the contents of a specified block.  This is very useful when debugging the FRAM contents.  Checkout the information in [SPEC.md](https://github.com/littlefs-project/littlefs/blob/master/SPEC.md). |
| lfs_erase [block_id]     | This erases (simply set to 0xFF) a specified block.  -1 block_id erases the entire FRAM.                                                       |
| lfs_format               | This formats the FRAM.                                                                                                                         |
| lfs_mount                | This mounts the LittleFS filesystem.                                                                                                           |
| lfs_umount               | This unmounts the LittleFS filesystem.                                                                                                         |
| lfs_ls path              | This lists the contents of directory specified by [path]                                                                                       |
| lfs_mkdir [fullPathName] | This creates a directory specified by [fullPathName]                                                                                           |
| lfs_fopen [fullPathName] | This opens or creates a file specified by [fullPathName].  Note: Default is LFS_O_CREAT \| LFS_O_RDWR                                          |
| lfs_fwrite [asciiString] | This writes an ASCII string to an opened file using lfs_fopen.                                                                                 |
| lfs_fread                | This reads the contents of an opened file and prints it in the console.                                                                        |
| lfs_fclose               | This closes an opened file.                                                                                                                    |
| lfs_mv [src] [dst]       | This moves/renames a file or directory from [src] to [dst]                                                                                     |
| lfs_rm [src]             | This removes a file or an empty directory                                                                                                      |
| ymodem_receive           | This receives a file from a YMODEM host and store it in the LittleFS filesystem                                                                |
| ymodem_send [filename]   | This reads [filename] from LittleFS filesystem and send it to a YMODEM host                                                                    |

### Test01: Format
First test is to format the FRAM using "lfs_format".  The superblock metadata pair (block0 and block1) is dump using lfs_dump.  Using the information from [SPEC.md](https://github.com/littlefs-project/littlefs/blob/master/SPEC.md), 
the superblocks are then inspected.  The figure below shows that superblocks are OK.


<img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/superblockAfterFormat.png">


### Test02: File Test
First part of the test, it creates a file, "fileTest01.txt", in the root directory and writes the string "Hello World!" to the said file.  The file is closed and the filesystem unmounted.  
Second part of the test, it opens the existing file, "fileTest01.txt", and reads the contents of the said file and prints the contents into the console.

<img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/fileTest.png">


### Test03: Directory Test
In this test, three (3) directories ("color", "movie", "potato") are created in the root directory.  Under "color" directory, three files are created.  These files are "green.txt", "blue.txt", and "red.txt".


<img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/directoryTest.png">


## Test04: Nested Directory Test
In this test, three (3) directories ("frozen", "spiderman", "transformers") are created in the "movie" directory.  A file, "robot.txt", is then created under "/movie/transformers" directory.


<img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/nestedDirTest.png">


## Test05: Moving File and Directory Test
First part of the test, it renames the existing file "testFile01.txt" to "bakedPotato.txt".
Second part of the test, it moves the file "bakedPotato.txt" to "/potato/bakedPotato.txt".
Third part of the test, it creates a directory "/potatoRecipe" and move it to "/potato/potatoRecipe"


<img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/moveTest.png">


## Test06: Removing file and empty directory
First part of the test, it removes an empty directory "/potato/potatoRecipe"
Second part of the test, it removes the file "/potato/bakedPotato.txt"
Third part of the test, it removes the empty directory "/potato"


<img src="https://github.com/sicrisembay/littlefs_c2000/blob/main/doc/img/removeTest.png">


