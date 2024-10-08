#include "pch.h"

#include "FileHashGenerator.h"

FileHashGenerator::FileHashGenerator() : fileStream(), hProv(0), hHash(0) {
  if (!::CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES,
                             CRYPT_VERIFYCONTEXT)) {
    throw std::runtime_error("Error acquiring cryptographic context");
  }

  if (!::CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash)) {
    ::CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error creating hash");
  }
}

FileHashGenerator::~FileHashGenerator() {
  ::CryptReleaseContext(hProv, 0);
  if (hHash) {
    ::CryptDestroyHash(hHash);
  }
}

std::wstring FileHashGenerator::generateSHA(const std::wstring& filePath) {
  hHash = createHash(CALG_MD4);

  return generateHash(filePath, CALG_SHA);
}

std::wstring FileHashGenerator::generateMD4(const std::wstring& filePath) {
  hHash = createHash(CALG_SHA);

  return generateHash(filePath, CALG_MD4);
}

HCRYPTHASH
FileHashGenerator::createHash(ALG_ID hashAlgorithm) const {
  HCRYPTHASH hash;
  if (!::CryptCreateHash(hProv, hashAlgorithm, 0, 0, &hash)) {
    throw std::runtime_error("Error creating hash");
  }
  return hash;
}

std::wstring FileHashGenerator::generateHash(const std::wstring& filePath,
                                             ALG_ID hashAlgorithm) {
  const size_t bufferSize = 8192;
  std::vector<wchar_t> buffer(bufferSize);

  while (fileStream.read(buffer.data(), bufferSize)) {
    if (!::CryptHashData(
            hHash, reinterpret_cast<BYTE*>(buffer.data()),
            static_cast<DWORD>(fileStream.gcount() * sizeof(wchar_t)), 0)) {
      ::CryptDestroyHash(hHash);
      ::CryptReleaseContext(hProv, 0);
      throw std::runtime_error("Error hashing file data");
    }
  }

  DWORD hashSize = 0;
  DWORD hashSizeSize = sizeof(DWORD);

  if (!::CryptGetHashParam(hHash, HP_HASHSIZE,
                           reinterpret_cast<BYTE*>(&hashSize), &hashSizeSize,
                           0)) {
    ::CryptDestroyHash(hHash);
    ::CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error getting hash size");
  }

  std::vector<BYTE> hashBuffer(hashSize);

  if (!::CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer.data(), &hashSize,
                           0)) {
    ::CryptDestroyHash(hHash);
    ::CryptReleaseContext(hProv, 0);
    throw std::runtime_error("Error getting hash value");
  }

  std::wstringstream hashStream;
  hashStream << std::hex << std::setfill(L'0');
  for (BYTE byte : hashBuffer) {
    hashStream << std::setw(2) << static_cast<unsigned>(byte);
  }

  return hashStream.str();
}
