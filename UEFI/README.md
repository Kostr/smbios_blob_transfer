# Description

Currently there are two UEFI implementations:
- [custom](custom) - the version that you should use if you have your BIOS sources and can use IPMI protocols implemented by your BIOS vendor,
- [standalone](standalone) - fully standalone version that can be built with the modern EDK2 and integrated to your BIOS via [UEFITool](https://github.com/LongSoft/UEFITool) or executed as an UEFI application. This version uses IPMI KCS as a transfer protocol.
