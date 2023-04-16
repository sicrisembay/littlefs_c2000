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

TODO!