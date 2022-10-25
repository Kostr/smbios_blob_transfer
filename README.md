# Description

This program performs a transfer of SMBIOS tables from Host to BMC over IPMI BLOB protocol.

# Usage

Install the latest meson:
```
$ pip3 install --user meson
$ source ~/.profile
```
Build [ipmi-blob-tool](https://github.com/openbmc/ipmi-blob-tool) library:
```
$ cd ~
$ git clone https://github.com/openbmc/ipmi-blob-tool.git
$ cd ipmi-blob-tool
$ meson setup builddir
$ cd builddir
$ meson compile
```
Build and run `smbios_blob_transfer`:
```
$ cd ~
$ git clone https://github.com/Kostr/smbios_blob_transfer.git
$ cd smbios_blob_transfer
$ g++ smbios_blob_transfer.cpp -L~/ipmi-blob-tool/builddir/src/ -lipmiblob -I~/ipmi-blob-tool/src/ -o smbios_blob_transfer
$ sudo LD_LIBRARY_PATH=~/ipmi-blob-tool/builddir/src/ ./smbios_blob_transfer
```
After the program execution this file should be created on the BMC:
```
/var/lib/smbios/smbios2
```

If you don't want to pass all the parameters for dynamic library linking, you can install `ipmi-blob-tool` to your system.

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

# `smbios_transfer.go`

This program was written as an alternative to Go utility [smbios_transfer.go](https://github.com/u-root/u-root/blob/main/cmds/exp/smbios_transfer/smbios_transfer.go).

If you want to, you can use it instead.
```
$ git clone https://github.com/u-root/u-root.git
$ cd u-root/cmds/exp/smbios_transfer/
$ sudo go run smbios_transfer.go
```

If you don't have `go`, you'll need to install it. If you can't use package manager you can find all the downloads at the [https://go.dev/dl/](https://go.dev/dl/):
```
$ cd ~
$ wget https://go.dev/dl/go1.19.2.linux-amd64.tar.gz
$ sudo tar -xvf go1.19.2.linux-amd64.tar.gz
$ sudo mv go /usr/local
```

Add this to your `~/.profile`:
```
export GOPATH=$HOME/go
export PATH=$PATH:/usr/local/go/bin:$GOPATH/bin
```

Now check that `go` is working:
```
source ~/.profile
$ go version
go version go1.19.2 linux/amd64
```

Access to smbios table files is not available for ordinary user, so you'll need to execute go program from root:
```
$ sudo -s
```
Therefore you need to midify `~/.profile` for the root user as well:
```
export GOPATH=$HOME/go
export PATH=$PATH:/usr/local/go/bin:$GOPATH/bin
```
Check that `go` is working for root:
```
$ source ~/.profile
$ go version
go version go1.19.2 linux/amd64
```

Now run the `smbios_transfer.go` program:
```
$ cd ~/u-root/cmds/exp/smbios_transfer/
$ go run smbios_transfer.go
```
Alternatively you can build the executable and run it without `go`:
```
$ go build smbios_transfer.go
$ ./smbios_transfer
```

