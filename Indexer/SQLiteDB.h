#pragma once

class SQLiteDB {
 public:
  SQLiteDB(const std::string& dbPath);
  ~SQLiteDB();

  void createTable();

  void insertRecord(const std::string& filePath, const std::string& fileHash);

  std::vector<std::pair<std::string, std::string>> searchRecord(
      const std::string& fileName, const std::string& fileHash);

 private:
  sqlite3* db;

  void executeQuery(const std::string& query);
};
