# Description

This folder contains `DXE_DRIVER` to transfer SMBIOS tables from Host to BMC over IPMI BLOB protocol.

# Porting

This driver was developed for the Insyde H2O framework. Due to legal reasons I've changed some of the identificators to custom names.

The code relies on the function `ExecuteIpmiCmd` from the custom IPMI protocol. If your want to port the `DXE_DRIVER` to your firmware, you need to replace them with your UEFI firmware IPMI protocol and similar execute function.

The `ExecuteIpmiCmd` is basically `WriteIpmiCmd-ReadIpmiCmd` in one function. Here is its prototype:
```cpp
/**
 Send request, include Network Function, LUN, and command of IPMI, with/without
 additional data to BMC.

 @param[in]        This                 A pointer to MY_IPMI_INTERFACE_PROTOCOL structure.
 @param[in]        Request              MY_IPMI_CMD_HEADER structure, storing Network Function, LUN
                                        and various IPMI command, to send to BMC.
 @param[in]        SendData             Optional arguments, if an IPMI command is required to
                                        send with data, this argument is a pointer to the data buffer.
                                        If no data is required, set this argument as NULL.
 @param[in]        SendLength           When sending command with data, this argument is the length of the data,
                                        otherwise set this argument as 0.
 @param[out]       RecvData             Data buffer to put the data read from BMC.
 @param[in, out]   RecvLength           in : Length of RecvData.
                                        out: Length of Data readed from BMC.
 @param[out]       StatusCodes          The bit 15 of StatusCodes means this argument is valid or not:
                                        1. If bit 15 set to 1, this is a valid Status Code,
                                        and the Status Code is in low byte.
                                        2. If bit 15 set to 0, there is no Status Code
                                        StatusCodes is valid when return value is EFI_ABORTED.
                                        If the return value is EFI_DEVICE_ERROR, it does not
                                        guarantee StatusCodes is valid, the caller must check bit 15.

 @retval EFI_SUCCESS                    Execute command successfully.
 @retval EFI_ABORTED                    1. When writing to BMC, BMC cannot enter "Write State",
                                        the error processing make BMC to enter "Idle State" successfully.
                                        2. When finishing reading from BMC, BMC cannot enter "Idle State",
                                        the error processing make BMC to enter "Idle State" successfully.
 @retval EFI_TIMEOUT                    Output buffer is not full or iutput buffer is not empty
                                        in a given time.
 @retval EFI_DEVICE_ERROR               1. When writing to BMC, BMC cannot enter "Write State",
                                        the error processing cannot make BMC to enter "Idle State".
                                        2. When finishing reading from BMC, BMC cannot enter "Idle State",
                                        the error processing cannot make BMC to enter "Idle State".
 @retval EFI_NO_MAPPING                 The request Network Function and the response Network Function
                                        does not match.
 @retval EFI_LOAD_ERROR                 Execute command successfully, but the completion code return
                                        from BMC is not 00h.
 @retval EFI_INVALID_PARAMETER          This or RecvData or RecvLength is NULL.
 @retval EFI_BUFFER_TOO_SMALL           1. IPMI Buffer can't store all data from BMC.
                                        2. RecvData Buffer is too small to save data.
*/
EFI_STATUS
EFIAPI
ExecuteIpmiCmd (
  IN     MY_IPMI_INTERFACE_PROTOCOL       *This,
  IN     MY_IPMI_CMD_HEADER               Request,
  IN     VOID                              *SendData OPTIONAL,
  IN     UINT8                             SendLength,
  OUT    VOID                              *RecvData,
  IN OUT UINT8                             *RecvLength,
  OUT    UINT16                            *StatusCodes OPTIONAL
  )
```

## `*.inf` file modifications

Here are the important parts of the INF file that you need to port to your platform:
```
[Packages]
  ...
  MyIpmiPkg/MyIpmiPkg.dec              # DEC file of the package with your IPMI protocol

...

[Protocols]
  gMyIpmiInterfaceProtocolGuid         # GUID identifier of your IPMI protocol

...

[Depex]
  gMyIpmiInterfaceProtocolGuid AND     # GUID identifier of your IPMI protocol
  ...
```

## `*.c` file modifications

Here are the important parts of the C file that you need to port to your platform:
```cpp
#include <Protocol/MyIpmiInterfaceProtocol.h>             // header for you IPMI protocol

...

MY_IPMI_INTERFACE_PROTOCOL* mIpmi = NULL;                 // structure for you IPMI protocol

...

EFI_STATUS sendBlobCommand(UINT8 BlobCmd, UINT8* Request, UINT8 RequestLength, UINT8* Response, UINT8* ResponseLength)
{
  ...

  MY_IPMI_CMD_HEADER IpmiCmdHeader;                       // MY_IPMI_CMD_HEADER is specific to the ExecuteIpmiCmd, check your protocol if it needs similar input
  IpmiCmdHeader.Lun = 0;
  IpmiCmdHeader.NetFn = IPMI_GOOGLE_NETFN;
  IpmiCmdHeader.Cmd = IPMI_GOOGLE_BLOB_CMD;

  ...

  EFI_STATUS Status = mIpmi->ExecuteIpmiCmd(                    // the main IPMI function that your UEFI firmware needs to implement
                                           mIpmi,
                                           IpmiCmdHeader,
                                           (VOID*)SendBuf,
                                           SendLength,
                                           (VOID*)ReceiveBuf,
                                           &ReceiveLength,
                                           &StatusCodes
                                           );
  ...
}

...

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ...
  Status = gBS->LocateProtocol (&gMyIpmiInterfaceProtocolGuid, NULL, (VOID **)&mIpmi);        // IPMI protocol GUID
  ...
}
```
