#include "pch.h"

#include "FileHashGenerator.h"
#include "SQLiteDB.h"

// Forward declarations
std::vector<std::wstring>
GetAvailableDrives();

std::wstring
SelectDriveToIndex();

void
enumerateAndStoreFiles(const std::wstring& directory, SQLiteDB& db);

void
enumerateAndStoreFilesForAllDrives(SQLiteDB& db);

void
searchAndPrintFiles(const std::string& fileName,
                    const std::string& fileHash,
                    SQLiteDB& db);

std::string
ConvertWStringToUTF8(const std::wstring& wstr)
{
  int size_needed = WideCharToMultiByte(CP_UTF8,
                                        0,
                                        wstr.c_str(),
                                        static_cast<int>(wstr.length()),
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);
  std::string result(size_needed, 0);
  WideCharToMultiByte(CP_UTF8,
                      0,
                      wstr.c_str(),
                      static_cast<int>(wstr.length()),
                      &result[0],
                      size_needed,
                      NULL,
                      NULL);
  return result;
}

// Function to display the main menu
int
displayMainMenu()
{
  // system("cls"); // Clear the console screen on Windows

  std::wcout << L"1. Index Drive\n"
             << L"2. Search for File\n"
             << L"3. Quit\n"
             << L"Enter your choice: ";

  int choice;
  std::wcin >> choice;

  return choice;
}

int
main()
{
  try {
    std::string dbPath = "example.db";
    SQLiteDB db(dbPath);

    int choice;

    do {

      choice = displayMainMenu();

      switch (choice) {
        case 1: {
          std::wstring driveToIndex = SelectDriveToIndex();

          if (driveToIndex.empty()) {
            std::wcout << L"Indexing all drives..." << std::endl;
            enumerateAndStoreFilesForAllDrives(db);
          } else {
            std::wcout << L"Indexing drive: " << driveToIndex << std::endl;
            enumerateAndStoreFiles(driveToIndex, db);
          }
          break;
        }
        case 2: {

          std::string hashToSearch;
          std::cout << "Enter hash to search: ";
          std::cin >> hashToSearch;

          searchAndPrintFiles("", hashToSearch, db);
          break;
        }
        case 3:
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

// Function to get available drives using Windows API
std::vector<std::wstring>
GetAvailableDrives()
{
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
std::wstring
SelectDriveToIndex()
{
  std::vector<std::wstring> drives = GetAvailableDrives();

  if (drives.empty()) {
    std::wcout << L"No drives found." << std::endl;
    return L"";
  }

  std::wcout << L"Available drives:" << std::endl;
  for (size_t i = 0; i < drives.size(); ++i) {
    std::wcout << i + 1 << L". " << drives[i] << std::endl;
  }

  std::wcout << L"Enter the number of the drive to index (or leave blank to "
                L"index all): ";

  std::wstring userInput;
  std::wcin >> userInput;

  if (userInput.empty()) {
    return L""; // User wants to index all drives
  }

  size_t driveIndex;
  try {
    driveIndex = std::stoul(userInput);
    if (driveIndex > 0 && driveIndex <= drives.size()) {
      return drives[driveIndex - 1];
    }
  } catch (const std::exception&) {
    // Invalid input, fall through to default case
  }

  std::wcout << L"Invalid input. Indexing all drives." << std::endl;
  return L"";
}

// Function to enumerate and store files using Windows API
void
enumerateAndStoreFiles(const std::wstring& directory, SQLiteDB& db)
{
  WIN32_FIND_DATA findFileData;
  HANDLE hFind = FindFirstFile((directory + L"*").c_str(), &findFileData);

  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wstring filePath = directory + findFileData.cFileName;

        try {
          // Attempt to open the file
          std::wifstream fileStream(filePath, std::ios::binary);
          if (!fileStream.is_open()) {
            // Failed to open the file
            std::wcerr << L"Error opening file: " << filePath << std::endl;
            continue; // Skip to the next file
          }

          // Debug print the file currently getting hashed
          std::wcout << L"Hashing file: " << filePath << std::endl;

          // Read and hash the file content using the file stream
          std::wstring fileHash = FileHashGenerator::generateSHA(filePath);

          // Store file information in the SQLite database
          db.insertRecord(ConvertWStringToUTF8(filePath),
                          ConvertWStringToUTF8(fileHash));

          // Debug print the file stored in the database
          std::wcout << L"Stored in database: " << filePath << std::endl;

        } catch (const std::exception& e) {
          // Exception occurred (e.g., access denied), skip the file
          std::wcerr << L"Error processing file: " << filePath << " - "
                     << e.what() << std::endl;
          continue; // Skip to the next file
        }
      } else if (wcscmp(findFileData.cFileName, L".") != 0 &&
                 wcscmp(findFileData.cFileName, L"..") != 0) {
        // Recursively call the function for subdirectories (excluding '.' and
        // '..')
        std::wstring subdirectory = directory + findFileData.cFileName + L"\\";
        enumerateAndStoreFiles(subdirectory, db);
      }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
  }
}
// Function to enumerate and store files for all drives
void
enumerateAndStoreFilesForAllDrives(SQLiteDB& db)
{
  std::vector<std::wstring> drives = GetAvailableDrives();

  for (const auto& drive : drives) {
    std::wcout << L"Indexing drive: " << drive << std::endl;
    enumerateAndStoreFiles(drive, db);
  }
}

void
searchAndPrintFiles(const std::string& fileName,
                    const std::string& fileHash,
                    SQLiteDB& db)
{
  std::cout << "Searching for files" << std::endl;

  // Search for files in the SQLite database by name and hash
  auto result = db.searchRecord(fileName, fileHash);

  for (const auto& file : result) {
    std::cout << "File Path: " << file.first << ", Hash: " << file.second
              << std::endl;
  }
}
