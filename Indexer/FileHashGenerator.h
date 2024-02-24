#pragma once

#include "pch.h"

class FileHashGenerator
{
public:
  static std::wstring generateSHA(const std::wstring& filePath);
  static std::wstring generateMD5(const std::wstring& filePath);

private:
  static std::wstring generateHash(const std::wstring& filePath,
                                   ALG_ID hashAlgorithm);
};
