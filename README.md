# DinoFST

Command line utility for working with Dinosaur Planet assets.
This tool provides the following options:

- **dump_bin**: extract raw fst.bin from ROM
- **dump_files**: extract raw files from ROM to given directory
- **extract_files**: extract files from ROM into intermediate formats to given directory
- **compile_rom**: compile new FST and build a new ROM
- **elf2dll**: converts a MIPS .ELF into a DP compatible .DLL. Needs to adhere to the standard as seen in DinoSDK.

## Important notice

DinoFST is written using WARLOCK Engine, which is the framework I use for my projects. At the moment, this framework is not open source, hence this repository's current use is as reference only.

Therefore **there currently is no license for this code**, which means there is no use right whatsoever. This is reference code only for the time being.

## Credits
@nuggslet: additional research to the DLL system used in DP

## [ELFIO](https://github.com/serge1/ELFIO)
Used for parsing .ELF files

## [snescrc](http://n64dev.org/n64crc.html)
Used to calculate a new CRC for the ROM

## [DPWiki](http://dinosaurpla.net/)

## [DP FindersClub Discord](https://discord.gg/NesmVt6J4T)

