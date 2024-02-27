#include "pch.h"

#include "FileHashGenerator.h"
#include "SQLiteDB.h"

std::vector<std::wstring> GetAvailableDrives();

std::wstring SelectDriveToIndex();

void enumerateAndStoreFiles(const std::wstring& directory, SQLiteDB& db);

void enumerateAndStoreFilesForAllDrives(SQLiteDB& db);

void searchAndPrintFiles(const std::string& fileName,
                         const std::string& fileHash, SQLiteDB& db);

std::string ConvertWStringToUTF8(const std::wstring& wstr);

int displayMainMenu();

int main() {
  try {
    std::string dbPath = "Index.db";
    SQLiteDB db(dbPath);

    int choice;

    do {
      choice = displayMainMenu();

      system("cls");
      switch (choice) {
        case 1: {
          std::wstring driveToIndex = SelectDriveToIndex();
          system("cls");

          if (driveToIndex.empty()) {
            std::wcout << L"Indexing all drives..." << std::endl;

            auto indexingStart = std::chrono::high_resolution_clock::now();

            enumerateAndStoreFilesForAllDrives(db);
            auto indexingEnd =
                std::chrono::high_resolution_clock::now();

            auto indexingDuration =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    indexingEnd - indexingStart);
            std::cout << "Indexing Time: " << indexingDuration.count()
                      << " milliseconds" << std::endl;
          } else {
            std::wcout << L"Indexing drive: " << driveToIndex << std::endl;

            auto indexingStart = std::chrono::high_resolution_clock::now();

            enumerateAndStoreFiles(driveToIndex, db);

            auto indexingEnd =
                std::chrono::high_resolution_clock::now();

            auto indexingDuration =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    indexingEnd - indexingStart);
            std::cout << "indexing Time: " << indexingDuration.count()
                      << " milliseconds" << std::endl;
          }
          break;
        }
        case 2: {
          auto searchStart =
              std::chrono::high_resolution_clock::now();

          std::string filenameToSearch;
          std::cout << "Enter filename to search: ";
          std::cin >> filenameToSearch;

          searchAndPrintFiles(filenameToSearch, "", db);

          auto searchEnd =
              std::chrono::high_resolution_clock::now();

          auto searchDuration =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  searchEnd - searchStart);
          std::cout << "Search Time: " << searchDuration.count()
                    << " milliseconds" << std::endl;

          system("pause");

          break;
        }
        case 3: {
          auto searchStart =
              std::chrono::high_resolution_clock::now();

          std::string hashToSearch;
          std::cout << "Enter hash to search: ";
          std::cin >> hashToSearch;

          searchAndPrintFiles("", hashToSearch, db);

          auto searchEnd =
              std::chrono::high_resolution_clock::now();

          auto searchDuration =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  searchEnd - searchStart);
          std::cout << "Search Time: " << searchDuration.count()
                    << " milliseconds" << std::endl;
        }

        break;
        case 4:
          std::wcout << L"Exiting the program." << std::endl;
          break;
        default:
          std::wcout << L"Invalid choice. Please try again." << std::endl;
      }

    } while (choice != 3);

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}

int displayMainMenu() {
  system("cls");

  std::wcout << L"1. Index Drive\n"
             << L"2. Search for Filename\n"
             << L"3. Search for Hash\n"
             << L"4. Quit\n"
             << L"Enter your choice: ";

  int choice;
  std::wcin >> choice;

  return choice;
}

std::string ConvertWStringToUTF8(const std::wstring& wstr) {
  int size_needed =
      WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                          static_cast<int>(wstr.length()), NULL, 0, NULL, NULL);
  std::string result(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()),
                      &result[0], size_needed, NULL, NULL);
  return result;
}

std::vector<std::wstring> GetAvailableDrives() {
  std::vector<std::wstring> drives;
  DWORD drivesMask = GetLogicalDrives();

  for (char i = 'A'; i <= 'Z'; ++i) {
    if (drivesMask & 1) {
      std::wstring drivePath = L"";
      drivePath += i;
      drivePath += L":\\";
      drives.push_back(drivePath);
    }
    drivesMask >>= 1;
  }

  return drives;
}

std::wstring SelectDriveToIndex() {
  std::vector<std::wstring> drives = GetAvailableDrives();

  if (drives.empty()) {
    std::wcout << L"No drives found." << std::endl;
    return L"";
  }

  std::wcout << L"Available drives:" << std::endl;
  for (size_t i = 0; i < drives.size(); ++i) {
    std::wcout << i + 1 << L". " << drives[i] << std::endl;
  }

  std::wcout
      << L"Enter the number of the drive to index (or 'A' to index all): ";

  std::wstring userInput;
  std::wcin >> userInput;

  std::transform(userInput.begin(), userInput.end(), userInput.begin(),
                 ::towupper);

  if (userInput == L"A") {
    return L"";
  }

  size_t driveIndex;
  try {
    driveIndex = std::stoul(userInput);
    if (driveIndex > 0 && driveIndex <= drives.size()) {
      return drives[driveIndex - 1];
    }
  } catch (const std::exception&) {
    // invalid input, fall through to default
  }

  return L"";
}

void enumerateAndStoreFiles(const std::wstring& directory, SQLiteDB& db) {
  WIN32_FIND_DATA findFileData;
  HANDLE hFind = FindFirstFile((directory + L"*").c_str(), &findFileData);

  if (hFind != INVALID_HANDLE_VALUE) {
    FileHashGenerator hashGenerator;

    do {
      if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wstring filePath = directory + findFileData.cFileName;

        try {
          std::wifstream fileStream(filePath, std::ios::binary);
          if (!fileStream.is_open()) {
            std::wcerr << L"Error opening file: " << filePath << std::endl;
            continue;
          }

#ifndef NDEBUG
          std::wcout << L"Hashing file: " << filePath << std::endl;
#endif
          std::wstring fileHash = hashGenerator.generateMD4(filePath);

          db.insertRecord(ConvertWStringToUTF8(filePath),
                          ConvertWStringToUTF8(fileHash));

#ifndef NDEBUG

          std::wcout << L"Stored in database: " << filePath << std::endl;
#endif
        } catch (const std::exception& e) {
          std::wcerr << L"Error processing file: " << filePath << " - "
                     << e.what() << std::endl;
          continue;
        }
      } else if (wcscmp(findFileData.cFileName, L".") != 0 &&
                 wcscmp(findFileData.cFileName, L"..") != 0) {
        std::wstring subdirectory = directory + findFileData.cFileName + L"\\";
        enumerateAndStoreFiles(subdirectory, db);
      }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
  }
}

void enumerateAndStoreFilesForAllDrives(SQLiteDB& db) {
  std::vector<std::wstring> drives = GetAvailableDrives();

  for (const auto& drive : drives) {
    std::wcout << L"Indexing drive: " << drive << std::endl;
    enumerateAndStoreFiles(drive, db);
  }
}

void searchAndPrintFiles(const std::string& fileName,
                         const std::string& fileHash, SQLiteDB& db) {
  auto result = db.searchRecord(fileName, fileHash);

  std::unordered_map<std::string, int> hashColors;

  std::vector<std::pair<std::string, int>> hashCounts;

  for (const auto& file : result) {
    int color;
    if (hashColors.find(file.second) == hashColors.end()) {
      color = hashColors.size() % 14 + 1;  // Colors 1-14
      hashColors[file.second] = color;
    } else {
      color = hashColors[file.second];
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);

    std::cout << "Hash: " << file.second << " ";

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | FOREGROUND_RED |
                                FOREGROUND_GREEN | FOREGROUND_BLUE);

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::cout << "File Path: ";

    size_t startPos = file.first.find(fileName);
    if (startPos != std::string::npos) {
      std::cout << file.first.substr(0, startPos);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_GREEN);
      std::cout << file.first.substr(
          startPos, fileName.length());
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_BLUE | FOREGROUND_INTENSITY);
      std::cout << file.first.substr(startPos +
                                     fileName.length());
    } else {
      std::cout << file.first;
    }

    std::cout << std::endl;

    auto it = std::find_if(
        hashCounts.begin(), hashCounts.end(),
        [&](const auto& entry) { return entry.first == file.second; });

    if (it != hashCounts.end()) {
      it->second++;
    } else {
      hashCounts.emplace_back(file.second, 1);
    }
  }

  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                          FOREGROUND_INTENSITY | FOREGROUND_RED |
                              FOREGROUND_GREEN | FOREGROUND_BLUE);

  std::cout << "\n    =============== Common Hashes: =============== \n"
            << std::endl;

  std::sort(
      hashCounts.begin(), hashCounts.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.second > rhs.second; });

  int count = 0;
  for (const auto& entry : hashCounts) {
    std::cout << "Hash: ";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            hashColors[entry.first]);
    std::cout << entry.first << " ";
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_INTENSITY | FOREGROUND_RED |
                                FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::cout << "Count: " << entry.second << std::endl;

    count++;
    if (count >= 5) {
      break;
    }
  }

  size_t totalFiles = result.size();
  std::cout << "\nTotal Files: \n" << totalFiles << std::endl;
}
