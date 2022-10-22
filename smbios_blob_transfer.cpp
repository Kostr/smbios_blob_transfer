#include <fstream>
#include <iostream>
#include <unistd.h>
#include <ipmiblob/blob_handler.hpp>
#include <ipmiblob/blob_interface.hpp>
#include <ipmiblob/ipmi_handler.hpp>

inline std::vector<uint8_t> read_vector_from_disk(std::string file_path)
{
  std::ifstream instream(file_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
  return data;
}

std::uint32_t writeFileToBlob(ipmiblob::BlobHandler& blob, std::uint16_t session, std::uint32_t blob_offset, std::string filename)
{
  std::cout << "Send file " << filename << std::endl;
  std::vector<std::uint8_t> bytes;
  bytes = read_vector_from_disk(filename);
  std::uint32_t file_offset = 0;
  while (file_offset < bytes.size()) {
    std::uint32_t size = ((bytes.size() - file_offset) > 128) ? 128 : (bytes.size() - file_offset);
    auto first = bytes.cbegin() + file_offset;
    auto last = bytes.cbegin() + file_offset + size + 1;
    std::vector<std::uint8_t> buf(first, last);
    blob.writeBytes(session, blob_offset + file_offset, buf);
    file_offset += size;
  }
  return blob_offset + file_offset;
}


int main()
{
  if (getuid()) {
    std::cout << "Error! Please run the program with root priviledges" << std::endl;
    return EXIT_FAILURE;
  }
  auto ipmi = ipmiblob::IpmiHandler::CreateIpmiHandler();
  ipmiblob::BlobHandler blob(std::move(ipmi));
  int count = blob.getBlobCount();
  for (int i = 0; i < count; i++) {
    std::string blobId = blob.enumerateBlob(i);
    std::cout << "BLOB " << static_cast<int>(i) << ": " << blobId << std::endl;
    if (blobId == "/smbios") {
      std::uint16_t session = blob.openBlob(blobId, ipmiblob::StateFlags::open_write);
      std::cout << "SessionID = " << static_cast<int>(session) << std::endl;
      std::uint32_t blob_offset = 0;
      blob_offset = writeFileToBlob(blob, session, blob_offset, "/sys/firmware/dmi/tables/DMI");
      blob_offset = writeFileToBlob(blob, session, blob_offset, "/sys/firmware/dmi/tables/smbios_entry_point");    
      blob.commit(session);
      blob.closeBlob(session);
    }
  }
  return EXIT_SUCCESS;
}
