#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/MyIpmiInterfaceProtocol.h>


#define IPMI_GOOGLE_NETFN              46
#define IPMI_GOOGLE_BLOB_CMD           128

#define BMC_BLOB_CMD_CODE_GET_COUNT    0
#define BMC_BLOB_CMD_CODE_ENUMERATE    1
#define BMC_BLOB_CMD_CODE_OPEN         2
#define BMC_BLOB_CMD_CODE_READ         3
#define BMC_BLOB_CMD_CODE_WRITE        4
#define BMC_BLOB_CMD_CODE_COMMIT       5
#define BMC_BLOB_CMD_CODE_CLOSE        6
#define BMC_BLOB_CMD_CODE_DELETE       7
#define BMC_BLOB_CMD_CODE_STAT         8
#define BMC_BLOB_CMD_CODE_SESSION_STAT 9

#define BLOB_STATE_FLAG_OPEN_READ      (1 << 0)
#define BLOB_STATE_FLAG_OPEN_WRITE     (1 << 1)
#define BLOB_STATE_FLAG_COMMITING      (1 << 2)
#define BLOB_STATE_FLAG_COMMITED       (1 << 3)
#define BLOB_STATE_FLAG_COMMIT_ERROR   (1 << 4)

#pragma pack (1)
typedef struct {
  UINT8 OEN[3];
  UINT8 BlobCmd;
} BLOB_REQUEST;

typedef struct {
  UINT8 OEN[3];
  UINT8 BlobCmd;
  UINT16 Crc;
  UINT8 Data[1];
} BLOB_REQUEST_WITH_DATA;

typedef struct {
  UINT8 Completion;
  UINT8 OEN[3];
} BLOB_RESPONSE;

typedef struct {
  UINT8 Completion;
  UINT8 OEN[3];
  UINT16 Crc;
  UINT8 Data[1];
} BLOB_RESPONSE_WITH_DATA;
#pragma pack ()

UINT8 OpenBMC_OEN[3] = {0xcf, 0xc2, 0x00};

MY_IPMI_INTERFACE_PROTOCOL* mIpmi = NULL;

VOID PrintBuffer(UINT8* Buffer, UINTN Size)
{
  UINTN i = 0;
  while (i < Size) {
    DEBUG((EFI_D_VERBOSE, "%02x ", Buffer[i]));
    i++;
    if (!(i%16)) {
      DEBUG((EFI_D_VERBOSE, " | "));
      for (UINTN j=16; j>0; j--)
        if ((Buffer[i-j] >= 0x20) && (Buffer[i-j] < 0x7E))
          DEBUG((EFI_D_VERBOSE, "%c", Buffer[i-j]));
        else
          DEBUG((EFI_D_VERBOSE, "."));
      DEBUG((EFI_D_VERBOSE, "\n"));
    }
  }

  if (i%16) {
    for (UINTN j=0; j<=15; j++) {
      if ((i+j)%16)
        DEBUG((EFI_D_VERBOSE, "   "));
      else
        break;
    }
    DEBUG((EFI_D_VERBOSE, " | "));

    for (UINTN j=(i%16); j>0; j--) {
      if ((Buffer[i-j] >= 0x20) && (Buffer[i-j] < 0x7E))
        DEBUG((EFI_D_VERBOSE, "%c", Buffer[i-j]));
      else
        DEBUG((EFI_D_VERBOSE, "."));
    }
    DEBUG((EFI_D_VERBOSE, "\n"));
  }
}

UINT16 generateCrc(UINT8* data, UINTN size)
{
  UINT16 kPoly = 0x1021;
  UINT16 kLeftBit = 0x8000;
  UINT16 crc = 0xFFFF;
  INTN kExtraRounds = 2;
  for (UINTN i = 0; i < size + kExtraRounds; ++i) {
    for (UINTN j = 0; j < 8; ++j) {
      BOOLEAN xor_flag = (crc & kLeftBit) ? 1 : 0;
      crc <<= 1;
      if (i < size && (data[i] & (1 << (7 - j))))
      {
          crc++;
      }
      if (xor_flag)
      {
          crc ^= kPoly;
      }
    }
  }
  return crc;
}

EFI_STATUS sendBlobCommand(UINT8 BlobCmd, UINT8* Request, UINT8 RequestLength, UINT8* Response, UINT8* ResponseLength)
{
  UINT8 SendLength;
  if (RequestLength) {
	SendLength = sizeof(BLOB_REQUEST_WITH_DATA) - 1 + RequestLength;
  } else {
	SendLength = sizeof(BLOB_REQUEST);
  }
  UINT8* SendBuf = (UINT8*)AllocateZeroPool(SendLength);
  ((BLOB_REQUEST*)SendBuf)->OEN[0] = OpenBMC_OEN[0];
  ((BLOB_REQUEST*)SendBuf)->OEN[1] = OpenBMC_OEN[1];
  ((BLOB_REQUEST*)SendBuf)->OEN[2] = OpenBMC_OEN[2];
  ((BLOB_REQUEST*)SendBuf)->BlobCmd = BlobCmd;
  if (RequestLength) {
    ((BLOB_REQUEST_WITH_DATA*)SendBuf)->Crc = generateCrc(Request, RequestLength);
	CopyMem(&(((BLOB_REQUEST_WITH_DATA*)SendBuf)->Data), Request, RequestLength);
  }

  DEBUG((EFI_D_VERBOSE, "SEND BUF:\n"));
  PrintBuffer(SendBuf, SendLength);

  MY_IPMI_CMD_HEADER IpmiCmdHeader;
  IpmiCmdHeader.Lun = 0;
  IpmiCmdHeader.NetFn = IPMI_GOOGLE_NETFN;
  IpmiCmdHeader.Cmd = IPMI_GOOGLE_BLOB_CMD;

  UINT16 StatusCodes;

  UINT8 ReceiveLength;
  if (*ResponseLength) {
	ReceiveLength = sizeof(BLOB_RESPONSE_WITH_DATA) - 1 + (*ResponseLength);
  } else {
	ReceiveLength = sizeof(BLOB_RESPONSE);
  }
  UINT8* ReceiveBuf = (UINT8*)AllocateZeroPool(ReceiveLength);

  EFI_STATUS Status = mIpmi->ExecuteIpmiCmd(
                                           mIpmi,
                                           IpmiCmdHeader,
                                           (VOID*)SendBuf,
                                           SendLength,
                                           (VOID*)ReceiveBuf,
                                           &ReceiveLength,
                                           &StatusCodes
                                           );
  FreePool(SendBuf);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Error! ExecuteIpmiCmd returned %r\n", Status));
    FreePool(ReceiveBuf);
    return Status;
  }
  DEBUG((EFI_D_VERBOSE, "RECEIVE BUF:\n"));
  PrintBuffer(ReceiveBuf, ReceiveLength);

  if (  (ReceiveLength < sizeof(BLOB_RESPONSE)) ||
       ((ReceiveLength > sizeof(BLOB_RESPONSE)) && (ReceiveLength < sizeof(BLOB_RESPONSE_WITH_DATA)))  ) {
	DEBUG((EFI_D_ERROR, "Error! Response error: Wrong response size %d\n", ReceiveLength));
	FreePool(ReceiveBuf);
	return EFI_DEVICE_ERROR;
  }
  if (((BLOB_RESPONSE*)ReceiveBuf)->Completion != 0x00) {
	DEBUG((EFI_D_ERROR, "Error! Response error: Completion byte error\n", ReceiveBuf[0]));
	FreePool(ReceiveBuf);
	return EFI_DEVICE_ERROR;
  }
  if ((((BLOB_RESPONSE*)ReceiveBuf)->OEN[0] != OpenBMC_OEN[0]) ||
      (((BLOB_RESPONSE*)ReceiveBuf)->OEN[1] != OpenBMC_OEN[1]) ||
	  (((BLOB_RESPONSE*)ReceiveBuf)->OEN[2] != OpenBMC_OEN[2])) {
	DEBUG((EFI_D_ERROR, "Error! Response error: OEN mismatch\n"));
	FreePool(ReceiveBuf);
    return EFI_DEVICE_ERROR;
  }
  if (ReceiveLength >= sizeof(BLOB_RESPONSE_WITH_DATA)) {
	*ResponseLength = ReceiveLength - (sizeof(BLOB_RESPONSE_WITH_DATA) - 1);
	UINT16 Crc = generateCrc(((BLOB_RESPONSE_WITH_DATA*)ReceiveBuf)->Data, *ResponseLength);
	if (Crc != ((BLOB_RESPONSE_WITH_DATA*)ReceiveBuf)->Crc) {
		DEBUG((EFI_D_ERROR, "Error! Response error: CRC mismatch\n"));
		FreePool(ReceiveBuf);
		return EFI_DEVICE_ERROR;
	}
	CopyMem(Response, &(((BLOB_RESPONSE_WITH_DATA*)ReceiveBuf)->Data), *ResponseLength);
  } else {
	*ResponseLength = 0;
  }
  FreePool(ReceiveBuf);

  DEBUG((EFI_D_VERBOSE, "Response:\n"));
  PrintBuffer(Response, *ResponseLength);
  return Status;
}

EFI_STATUS BlobGetCount(UINT32* BlobCount)
{
  UINT8 ResponseLength = sizeof(*BlobCount);
  EFI_STATUS Status = sendBlobCommand(BMC_BLOB_CMD_CODE_GET_COUNT,
                                      NULL,
                                      0,
                                      (UINT8*)BlobCount,
                                      &ResponseLength);
  if (ResponseLength != sizeof(*BlobCount))
    return EFI_DEVICE_ERROR;
  return Status;
}

EFI_STATUS BlobEnumerate(UINT32 BlobIndex, UINT8* BlobId)
{
  UINT8 ResponseLength = 127;
  EFI_STATUS Status = sendBlobCommand(BMC_BLOB_CMD_CODE_ENUMERATE,
                                     (UINT8*)&BlobIndex,
                                     (UINT8)sizeof(BlobIndex),
                                     BlobId,
                                     &ResponseLength);
  return Status;
}

EFI_STATUS BlobOpen(UINT16 flags, UINT8* BlobId, UINT16* Session)
{
  UINT8 ResponseLength = (UINT8)sizeof(*Session);
  UINT8 RequestLength = (UINT8)sizeof(flags) + (UINT8)AsciiStrLen(BlobId) + 1;
  UINT8* Request = (UINT8*)AllocateZeroPool(RequestLength);
  Request[0] = flags & 0xFF;
  Request[1] = (flags >> 8) & 0xFF;
  CopyMem(&Request[2], BlobId, AsciiStrLen(BlobId));
  EFI_STATUS Status = sendBlobCommand(BMC_BLOB_CMD_CODE_OPEN,
                                     Request,
                                     RequestLength,
                                     (UINT8*)Session,
                                     &ResponseLength);
  FreePool(Request);
  if (ResponseLength != ((UINT8)sizeof(*Session)))
    return EFI_DEVICE_ERROR;
  return Status;
}

EFI_STATUS BlobClose(UINT16 Session)
{
  UINT8 Response;
  UINT8 ResponseLength = 0;
  EFI_STATUS Status = sendBlobCommand(BMC_BLOB_CMD_CODE_CLOSE,
                                     (UINT8*)&Session,
                                     (UINT8)sizeof(Session),
                                     &Response,
                                     &ResponseLength);
  if (ResponseLength != 0)
    return EFI_DEVICE_ERROR;
  return Status;
}

EFI_STATUS BlobWrite(UINT16 Session, UINT32 Offset, UINT8* Data, UINT8 DataSize)
{
  UINT8 Response;
  UINT8 ResponseLength = 0;
  UINT8 RequestLength = (UINT8)sizeof(Session) + (UINT8)sizeof(Offset) + DataSize;
  UINT8* Request = (UINT8*)AllocateZeroPool(RequestLength);
  Request[0] = Session & 0xFF;
  Request[1] = (Session >> 8) & 0xFF;
  Request[2] = Offset & 0xFF;
  Request[3] = (Offset >> 8) & 0xFF;
  Request[4] = (Offset >> 16) & 0xFF;
  Request[5] = (Offset >> 24) & 0xFF;
  CopyMem(&Request[6], Data, DataSize);
  EFI_STATUS Status = sendBlobCommand(BMC_BLOB_CMD_CODE_WRITE,
                                     Request,
                                     RequestLength,
                                     &Response,
                                     &ResponseLength);
  FreePool(Request);
  if (ResponseLength != 0)
    return EFI_DEVICE_ERROR;
  return Status;
}

EFI_STATUS BlobLongWrite(UINT16 Session, UINT32 Offset, UINT8* Data, UINT32 DataSize)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 Count = 0;
  while (Count < DataSize) {
    UINT32 Size = ((DataSize - Count) > 128) ? 128 : (DataSize - Count);
    Status = BlobWrite(Session, Offset + Count, &Data[Count], (UINT8)Size);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobWrite failed: %r\n", Status));
      return Status;
    }
    Count += Size;
  }
  return Status;
}

EFI_STATUS BlobCommit(UINT16 Session)
{
  UINT8 Response;
  UINT8 ResponseLength = 0;
  UINT8 RequestLength = (UINT8)sizeof(Session) + 1;
  UINT8* Request = (UINT8*)AllocateZeroPool(RequestLength);
  Request[0] = Session & 0xFF;
  Request[1] = (Session >> 8) & 0xFF;
  Request[2] = 0;
  EFI_STATUS Status = sendBlobCommand(BMC_BLOB_CMD_CODE_COMMIT,
                                     Request,
                                     RequestLength,
                                     &Response,
                                     &ResponseLength);
  FreePool(Request);
  if (ResponseLength != 0)
    return EFI_DEVICE_ERROR;
  return Status;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT8* SmbiosTableEntryPoint;
  UINTN SmbiosTableEntryPointLength;
  UINT8* SmbiosTableAddress;
  UINTN SmbiosTableLength;

  EFI_STATUS Status;
  Status = EfiGetSystemConfigurationTable(&gEfiSmbios3TableGuid, (VOID**)&SmbiosTableEntryPoint);
  if (EFI_ERROR(Status)) {
	Status = EfiGetSystemConfigurationTable(&gEfiSmbiosTableGuid, (VOID**)&SmbiosTableEntryPoint);
	if (EFI_ERROR(Status)) {
	  DEBUG((EFI_D_ERROR, "Error! Can't locate SMBIOS table\n"));
      return EFI_NOT_FOUND;
	}
	SmbiosTableEntryPointLength = sizeof(SMBIOS_TABLE_ENTRY_POINT);
	SmbiosTableAddress = (UINT8*)(UINTN)(((SMBIOS_TABLE_ENTRY_POINT*)SmbiosTableEntryPoint)->TableAddress);
	SmbiosTableLength = ((SMBIOS_TABLE_ENTRY_POINT*)SmbiosTableEntryPoint)->TableLength;
  } else {
	SmbiosTableEntryPointLength = sizeof(SMBIOS_TABLE_3_0_ENTRY_POINT);
	SmbiosTableAddress = (UINT8*)(UINTN)(((SMBIOS_TABLE_3_0_ENTRY_POINT*)SmbiosTableEntryPoint)->TableAddress);
	SmbiosTableLength = ((SMBIOS_TABLE_3_0_ENTRY_POINT*)SmbiosTableEntryPoint)->TableMaximumSize;
  }

  DEBUG((EFI_D_VERBOSE, "Smbios table entry point\n"));
  PrintBuffer(SmbiosTableEntryPoint, SmbiosTableEntryPointLength);
  DEBUG((EFI_D_VERBOSE, "Smbios table\n"));
  PrintBuffer(SmbiosTableAddress, SmbiosTableLength);


  Status = gBS->LocateProtocol (&gMyIpmiInterfaceProtocolGuid, NULL, (VOID **)&mIpmi);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Error! Can't locate IPMI protocol\n"));
    return Status;
  }
  UINT32 BlobCount = 0;
  BlobGetCount(&BlobCount);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Error! BlobGetCount failed: %r\n", Status));
    return Status;
  }
  DEBUG((EFI_D_INFO, "BlobCount = %d\n", BlobCount));

  for (UINT32 id = 0; id < BlobCount; id++) {
    UINT8* BlobId = (UINT8*)AllocateZeroPool(255);
    Status = BlobEnumerate(id, BlobId);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobEnumerate failed: %r\n", Status));
      FreePool(BlobId);
      return Status;
    }
    DEBUG((EFI_D_INFO, "BlobEnumerate: id=%d: %a\n", id, BlobId));
    if (!AsciiStrCmp(BlobId, "\\smbios")) {
      FreePool(BlobId);
      continue;
    }
    DEBUG((EFI_D_INFO, "SMBIOS blob is found\n"));
    UINT16 Session;
    Status = BlobOpen(BLOB_STATE_FLAG_OPEN_WRITE, BlobId, &Session);
    FreePool(BlobId);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobOpen failed: %r\n", Status));
      return Status;
    }
    DEBUG((EFI_D_INFO, "Session = 0x%04x\n", Session));
    Status = BlobLongWrite(Session, 0, SmbiosTableAddress, (UINT32)SmbiosTableLength);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobLongWrite failed: %r\n", Status));
      BlobClose(Session);
      return Status;
    }
    Status = BlobLongWrite(Session, (UINT32)SmbiosTableLength, SmbiosTableEntryPoint, (UINT32)SmbiosTableEntryPointLength);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobLongWrite failed: %r\n", Status));
      BlobClose(Session);
      return Status;
    }
    Status = BlobCommit(Session);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobCommit failed: %r\n", Status));
      BlobClose(Session);
      return Status;
    }
    Status = BlobClose(Session);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "Error! BlobClose failed: %r\n", Status));
      return Status;
    }
	DEBUG((EFI_D_INFO, "SMBIOS tables were successfully transmitted to the BMC\n"));
  }

  return EFI_SUCCESS;
}
