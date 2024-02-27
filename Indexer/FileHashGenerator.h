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
  HCRYPTPROV hProv;
  HCRYPTHASH hHash;
  HCRYPTHASH createHash(ALG_ID hashAlgorithm) const;
};
