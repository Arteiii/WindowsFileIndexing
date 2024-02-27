#include "pch.h"

#include "SQLiteDB.h"

SQLiteDB::SQLiteDB(const std::string& dbPath) {
  int result = sqlite3_open(dbPath.c_str(), &db);
  if (result != SQLITE_OK) {
    throw std::runtime_error("Error opening SQLite database");
  }

  const char* createTableQuery =
      "CREATE TABLE IF NOT EXISTS Files ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "path TEXT NOT NULL,"
      "hash TEXT NOT NULL"
      ");";

  result = sqlite3_exec(db, createTableQuery, 0, 0, 0);
  if (result != SQLITE_OK) {
    throw std::runtime_error("Error creating Files table");
  }
}

SQLiteDB::~SQLiteDB() {
  if (db) {
    sqlite3_close(db);
  }
}

void SQLiteDB::createTable() {
  const std::string query =
      "CREATE TABLE IF NOT EXISTS Files ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "path TEXT NOT NULL,"
      "hash TEXT NOT NULL);";
  executeQuery(query);
}

void SQLiteDB::insertRecord(const std::string& filePath,
                            const std::string& fileHash) {
  const std::string query =
      "INSERT OR REPLACE INTO Files (path, hash) VALUES (?, ?);";

  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Error preparing statement: " +
                             std::string(sqlite3_errmsg(db)));
  }

  sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, fileHash.c_str(), -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    throw std::runtime_error("Error inserting record: " +
                             std::string(sqlite3_errmsg(db)));
  }

  sqlite3_finalize(stmt);
}

std::vector<std::pair<std::string, std::string>> SQLiteDB::searchRecord(
    const std::string& fileName, const std::string& fileHash) {
  std::vector<std::pair<std::string, std::string>> result;

  std::string query = "SELECT path, hash FROM Files WHERE 1";

  if (!fileName.empty()) {
    query += " AND path LIKE '%" + fileName + "%'";
  }

  if (!fileHash.empty()) {
    query += " AND hash LIKE '%" + fileHash + "%'";
  }

  query += ";";

  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Error preparing statement: " +
                             std::string(sqlite3_errmsg(db)));
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* filePath =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    const char* fileHash =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    result.emplace_back(filePath, fileHash);
  }

  sqlite3_finalize(stmt);

  return result;
}

void SQLiteDB::executeQuery(const std::string& query) {
  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Error executing query: " +
                             std::string(sqlite3_errmsg(db)));
  }
}
