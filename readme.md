# ELFdwt
Open source implementation of the program `ELFDwT` that is included in the Keil uVision tool package. This tool calculates the checksum of the first 7 words of the vector table and sets it at address 0x0000001c of the elf file.

This tool was created with information from the document `LPC Boot ROM checksum` from the NXP community page ([this document](https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/lpc%40tkb/163/1/LPC%20Boot%20ROM%20checksum.pdf))

> :warning: **Currently only targets with little endian are supported**
