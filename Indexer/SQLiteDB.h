#pragma once

#include "pch.h"

class SQLiteDB
{
public:
  SQLiteDB(const std::string& dbPath);
  ~SQLiteDB();

  // Create the table if it doesn't exist
  void createTable();

  // Insert a new record into the database
  void insertRecord(const std::string& filePath, const std::string& fileHash);

  // Search for a record by either file name or file hash using optional regex
  std::vector<std::pair<std::string, std::string>> searchRecord(
    const std::string& fileName,
    const std::string& fileHash);

private:
  sqlite3* db;

  // Helper function to execute SQL queries
  void executeQuery(const std::string& query);
};
