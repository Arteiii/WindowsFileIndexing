#pragma once

class FileHashGenerator {
 public:
  FileHashGenerator();
  ~FileHashGenerator();

  std::wstring generateSHA(const std::wstring& filePath);
  std::wstring generateMD4(const std::wstring& filePath);

  std::wstring generateHash(const std::wstring& filePath, ALG_ID hashAlgorithm);

 private:
  std::wifstream fileStream;
  HCRYPTPROV hProv;  // Move these declarations here
  HCRYPTHASH hHash;  // from FileHashGenerator.cpp
  HCRYPTHASH createHash(ALG_ID hashAlgorithm) const;
  // other private members/functions...
};
