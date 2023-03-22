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
$ cd smbios_blob_transfer/OS
$ g++ smbios_blob_transfer.cpp -L~/ipmi-blob-tool/builddir/src/ -lipmiblob -I~/ipmi-blob-tool/src/ -o smbios_blob_transfer
$ sudo LD_LIBRARY_PATH=~/ipmi-blob-tool/builddir/src/ ./smbios_blob_transfer
```
After the program execution this file should be created on the BMC:
```
/var/lib/smbios/smbios2
```

If you don't want to pass all the parameters for dynamic library linking, you can install `ipmi-blob-tool` to your system.


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

