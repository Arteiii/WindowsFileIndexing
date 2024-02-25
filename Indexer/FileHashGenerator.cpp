#include "pch.h"

#include "FileHashGenerator.h"

std::wstring
FileHashGenerator::generateSHA(const std::wstring& filePath)
{
  return generateHash(filePath, CALG_SHA);
}

std::wstring
FileHashGenerator::generateMD4(const std::wstring& filePath)
{
  return generateHash(filePath, CALG_MD4);
}
}

std::wstring
FileHashGenerator::generateHash(const std::wstring& filePath,
                                ALG_ID hashAlgorithm)
{
  HCRYPTPROV hProv;
  HCRYPTHASH hHash;

  if (!CryptAcquireContext(
        &hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
    throw std::runtime_error("Error acquiring cryptographic context");
  }

  if (!CryptCreateHash(hProv, hashAlgorithm, 0, 0, &hHash)) {
    CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error creating hash");
  }

  std::wifstream fileStream(filePath, std::ios::binary);

  if (!fileStream) {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error opening file");
  }

  // Use a larger buffer size for potentially faster reading and hashing
  const size_t bufferSize = 8192;
  std::vector<wchar_t> buffer(bufferSize);

  while (fileStream.read(buffer.data(), bufferSize)) {
    if (!CryptHashData(
          hHash,
          reinterpret_cast<BYTE*>(buffer.data()),
          static_cast<DWORD>(fileStream.gcount() * sizeof(wchar_t)),
          0)) {
      CryptDestroyHash(hHash);
      CryptReleaseContext(hProv, 0);
      throw std::runtime_error("Error hashing file data");
    }
  }

  DWORD hashSize = 0;
  DWORD hashSizeSize = sizeof(DWORD);

  if (!CryptGetHashParam(hHash,
                         HP_HASHSIZE,
                         reinterpret_cast<BYTE*>(&hashSize),
                         &hashSizeSize,
                         0)) {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error getting hash size");
  }

  std::vector<BYTE> hashBuffer(hashSize);

  if (!CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer.data(), &hashSize, 0)) {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error getting hash value");
  }

  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);

  std::wstringstream hashStream;
  hashStream << std::hex << std::setfill(L'0');
  for (BYTE byte : hashBuffer) {
    hashStream << std::setw(2) << static_cast<unsigned>(byte);
  }

  return hashStream.str();
}
