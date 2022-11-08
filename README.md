# Description

This repo contains utilities to transfer SMBIOS tables from Host to BMC over IPMI BLOB protocol.

Two implementations are available:
- [transfer SMBIOS tables from the UEFI firmware (`DXE_DRIVER`)](UEFI)
- [transfer SMBIOS tables from the OS](OS)

# BMC requirements

BMC firmware should be based on the [openbmc](https://github.com/openbmc/openbmc) distribution.

You need to add [smbios-mdr](https://github.com/openbmc/smbios-mdr) and [phosphor-ipmi-blobs](https://github.com/openbmc/phosphor-ipmi-blobs) packages to the image.

Besides that you'll need to append the `smbios-mdr` recipe (`recipes-phosphor/smbios/smbios-mdr_git.bbappend`) with the following content:
```
PACKAGECONFIG:append = " smbios-ipmi-blob"

PACKAGECONFIG:remove = " cpuinfo"
```

# Results

After the successful transfer of SMBIOS tables you can check out the objects created by the `xyz.openbmc_project.Smbios.MDR_V2` service:
```
root@daytonax:~# busctl tree xyz.openbmc_project.Smbios.MDR_V2
`-/xyz
  `-/xyz/openbmc_project
    |-/xyz/openbmc_project/Smbios
    | `-/xyz/openbmc_project/Smbios/MDR_V2
    `-/xyz/openbmc_project/inventory
      `-/xyz/openbmc_project/inventory/system
        `-/xyz/openbmc_project/inventory/system/chassis
          `-/xyz/openbmc_project/inventory/system/chassis/motherboard
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/bios
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm0
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm1
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm10
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm11
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm12
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm13
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm14
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm15
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm2
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm3
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm4
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm5
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm6
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm7
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm8
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm9
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot0
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot1
            |-/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot2
            `-/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot3
```

Currently `smbios-mdr` parses only these types of SMBIOS tables ([specification](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.4.0.pdf)):
- BIOS Information (Type 0)
- System Information (Type 1)
- Processor Information (Type 4)
- System Slots (Type 9)
- Memory Device (Type 17)

Here is an example how [webui-vue](https://github.com/openbmc/webui-vue) can display the CPU information:

![CPU.png](CPU.png?raw=true "CPU info in webui")

And here is an example of DIMM information display:

![DIMM.png](DIMM.png?raw=true "DIMM info in webui")

If you want to display PCI devices as well, you should probably check the IBM fork of [webui-vue](https://github.com/ibm-openbmc/webui-vue).

