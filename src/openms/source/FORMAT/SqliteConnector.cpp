// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/SqliteConnector_impl.h>

#include <OpenMS/CONCEPT/Exception.h>

#include <cstring> // for strcmp
#include <iostream>

namespace OpenMS
{
  SqliteConnector::SqliteConnector(const std::string& filename, const SqlOpenMode mode)
  {
    openDatabase_(filename, mode);
  }

  SqliteConnector::~SqliteConnector()
  {
    int rc = sqlite3_close_v2(static_cast<sqlite3*>(db_));
    if (rc != SQLITE_OK)
    {
      std::cout << " Encountered error in ~SqliteConnector: " << rc << std::endl;
    }
  }

  namespace
  {
    /// Finalises a prepared statement on every exit, including an exception.
    struct StatementGuard
    {
      sqlite3_stmt* stmt = nullptr;
      ~StatementGuard() { sqlite3_finalize(stmt); }
    };

    /// Quotes an SQL identifier: SQLite doubles an embedded quote inside "..."
    std::string quoteIdentifier(const std::string& name)
    {
      std::string quoted = "\"";
      for (const char c : name)
      {
        if (c == '"') quoted += '"';
        quoted += c;
      }
      return quoted + '"';
    }
  }

  void SqliteConnector::openDatabase_(const std::string& filename, const SqlOpenMode mode)
  {
    // Open database
    int flags = 0;
    switch (mode)
    {
      case SqlOpenMode::READ_ONLY:
        flags = SQLITE_OPEN_READONLY;
        break;
      case SqlOpenMode::READWRITE:
        flags = SQLITE_OPEN_READWRITE;
        break;
      case SqlOpenMode::READWRITE_OR_CREATE:
        flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        break;
    }
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(filename.c_str(), &db, flags, nullptr);
    if (rc)
    {
      // sqlite3_open_v2 hands back an allocated handle even when it fails, and the
      // destructor never runs for an object whose constructor throws.
      sqlite3_close_v2(db);
      db_ = nullptr;
      throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Could not open sqlite db '" + filename + "' in mode " + StringUtils::toStr(int(mode)));
    }
    db_ = db;
  }

  bool SqliteConnector::tableExists(const std::string& tablename)
  {
    return Internal::SqliteHelper::tableExists(static_cast<sqlite3*>(db_), tablename);
  }

  bool SqliteConnector::columnExists(const std::string& tablename, const std::string& colname)
  {
    return Internal::SqliteHelper::columnExists(static_cast<sqlite3*>(db_), tablename, colname);
  }

  void SqliteConnector::executeStatement(const std::string& statement)
  {
    Internal::SqliteHelper::executeStatement(static_cast<sqlite3*>(db_), statement);
  }

  void SqliteConnector::executeBindStatement(const std::string& prepare_statement, const std::vector<std::string>& data)
  {
    Internal::SqliteHelper::executeBindStatement(static_cast<sqlite3*>(db_), prepare_statement, data);
  }

  Size SqliteConnector::countTableRows(const std::string& table_name)
  {
    sqlite3* db = static_cast<sqlite3*>(db_);
    StatementGuard guard;
    std::string select_runs = "SELECT count(*) FROM " + quoteIdentifier(table_name) + ";";
    Internal::SqliteHelper::prepareStatement(db, &guard.stmt, select_runs);
    // an error from the step is not an empty table: without this check a locked or broken
    // database read as a NULL count, and the throw below leaked the statement
    if (sqlite3_step(guard.stmt) != SQLITE_ROW || sqlite3_column_type(guard.stmt, 0) == SQLITE_NULL)
    {
      throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Could not retrieve " + table_name + " table count!");
    }
    return sqlite3_column_int64(guard.stmt, 0);
  }

  namespace Internal::SqliteHelper
    {
      bool columnExists(sqlite3 *db, const std::string& tablename, const std::string& colname)
      {
        StatementGuard guard;
        // the table name is an identifier, not text: interpolating it unquoted made a name
        // with SQL punctuation a syntax error or a different query
        prepareStatement(db, &guard.stmt, "PRAGMA table_info(" + quoteIdentifier(tablename) + ")");

        // Go through all columns and check whether the required column exists
        while (sqlite3_step(guard.stmt) == SQLITE_ROW)
        {
          const unsigned char* name = sqlite3_column_text(guard.stmt, 1);
          if (name != nullptr && colname == reinterpret_cast<const char*>(name))
          {
            return true;
          }
        }
        return false;
      }

      bool tableExists(sqlite3 *db, const std::string& tablename)
      {
        StatementGuard guard;
        // bound, not interpolated: a name containing an apostrophe ended the string literal
        prepareStatement(db, &guard.stmt, "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?;");
        sqlite3_bind_text(guard.stmt, 1, tablename.c_str(), static_cast<int>(tablename.size()), SQLITE_TRANSIENT);
        // a row means the table exists; any status other than SQLITE_ROW means it does not
        return sqlite3_step(guard.stmt) == SQLITE_ROW;
      }

      void executeStatement(sqlite3 *db, const std::string& statement)
      {
        char *zErrMsg = nullptr;
        int rc = sqlite3_exec(db, statement.c_str(), nullptr /* callback */, nullptr, &zErrMsg);
        if (rc != SQLITE_OK)
        {
          std::string error(zErrMsg);
          std::cerr << "Error message after sqlite3_exec" << std::endl;
          std::cerr << "Prepared statement " << statement << std::endl;
          sqlite3_free(zErrMsg);
          throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, error);
        }
      }

      void prepareStatement(sqlite3 *db, sqlite3_stmt** stmt, const std::string& prepare_statement)
      {
        int rc = sqlite3_prepare_v2(db, prepare_statement.c_str(), (int)prepare_statement.size(), stmt, nullptr);
        if (rc != SQLITE_OK)
        {
          std::cerr << "Error message after sqlite3_prepare_v2" << std::endl;
          std::cerr << "Prepared statement " << prepare_statement << std::endl;
          throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, sqlite3_errmsg(db));
        }
      }

      void executeBindStatement(sqlite3 *db, const std::string& prepare_statement, const std::vector<std::string>& data)
      {
        // the guard finalises the statement on every exit, including the two throws below
        StatementGuard guard;
        prepareStatement(db, &guard.stmt, prepare_statement);
        for (Size k = 0; k < data.size(); k++)
        {
          // Fifth argument is a destructor for the blob.
          // SQLITE_STATIC because the statement is finalized
          // before the buffer is freed:
          int rc = sqlite3_bind_blob(guard.stmt, k+1, data[k].c_str(), (int)data[k].size(), SQLITE_STATIC);
          if (rc != SQLITE_OK)
          {
            std::cerr << "SQL error after sqlite3_bind_blob at iteration " << k << std::endl;
            std::cerr << "Prepared statement " << prepare_statement << std::endl;
            throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, sqlite3_errmsg(db));
          }
        }

        if (sqlite3_step(guard.stmt) != SQLITE_DONE)
        {
          std::cerr << "SQL error after sqlite3_step" << std::endl;
          std::cerr << "Prepared statement " << prepare_statement << std::endl;
          throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, sqlite3_errmsg(db));
        }
      }

      template <> bool extractValue<double>(double* dst, sqlite3_stmt* stmt, int pos) //explicit specialization
      {
        if (sqlite3_column_type(stmt, pos) != SQLITE_NULL)
        {
          *dst = sqlite3_column_double(stmt, pos);
          return true;
        }
        return false;
      }

      template <> bool extractValue<int>(Int32* dst, sqlite3_stmt* stmt, int pos) //explicit specialization
      {
        if (sqlite3_column_type(stmt, pos) != SQLITE_NULL)
        {
          *dst = sqlite3_column_int(stmt, pos); // sqlite3_column_int returns 32bit integers
          return true;
        }
        return false;
      }
      template <> bool extractValue<Int64>(Int64* dst, sqlite3_stmt* stmt, int pos) //explicit specialization
      {
        if (sqlite3_column_type(stmt, pos) != SQLITE_NULL)
        {
          *dst = sqlite3_column_int64(stmt, pos);
          return true;
        }
        return false;
      }

      template <> bool extractValue<std::string>(std::string* dst, sqlite3_stmt* stmt, int pos) //explicit specialization
      {
        if (sqlite3_column_type(stmt, pos) != SQLITE_NULL)
        {
          const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, pos));
          // by length, not as a C string: a TEXT value may contain NUL bytes
          *dst = std::string(text, static_cast<size_t>(sqlite3_column_bytes(stmt, pos)));
          return true;
        }

        return false;
      }

      SqlState nextRow(sqlite3_stmt* stmt, SqlState current)
      {
        if (current != SqlState::SQL_ROW)
        { // querying a new row after the last invocation gave 'SQL_DONE' might loop around
          // to the first entry and give an infinite loop!!!
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Sql operation requested on SQL_DONE/SQL_ERROR state. This should never happen. Please file a bug report!");
        }
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
          return SqlState::SQL_ROW;
        }
        if (rc == SQLITE_DONE)
        {
          return SqlState::SQL_DONE;
        }
        if (rc == SQLITE_ERROR)
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Sql operation failed with SQLITE_ERROR!");
        }
        if (rc == SQLITE_BUSY)
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Sql operation failed with SQLITE_BUSY!");
        }
        if (rc == SQLITE_MISUSE)
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Sql operation failed with SQLITE_MISUSE!");
        }
        throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Sql operation failed with unexpected error code!");
      }

      /// Special case: store integer in a string data value
      bool extractValueIntStr(std::string* dst, sqlite3_stmt* stmt, int pos)
      {
        if (sqlite3_column_type(stmt, pos) == SQLITE_INTEGER)
        {
          *dst = StringUtils::toStr(sqlite3_column_int64(stmt, pos)); // SQLite INTEGER is 64 bit
          return true;
        }
        return false;
      }

      double extractDouble(sqlite3_stmt* stmt, int pos)
      {
        double res;
        if (!extractValue<double>(&res, stmt, pos))
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Conversion of column " + StringUtils::toStr(pos) + " to double failed");
        }
        return res;
      }

      float extractFloat(sqlite3_stmt* stmt, int pos)
      {
        double res; // there is no sqlite3_column_float.. so we extract double and convert
        if (!extractValue<double>(&res, stmt, pos))
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Conversion of column " + StringUtils::toStr(pos) + " to double/float failed");
        }
        return (float)res;
      }

      int extractInt(sqlite3_stmt* stmt, int pos)
      {
        int res;
        if (!extractValue<int>(&res, stmt, pos))
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Conversion of column " + StringUtils::toStr(pos) + " to int failed");
        }
        return res;
      }

      Int64 extractInt64(sqlite3_stmt* stmt, int pos)
      {
        Int64 res;
        if (!extractValue<Int64>(&res, stmt, pos))
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Conversion of column " + StringUtils::toStr(pos) + " to Int64 failed");
        }
        return res;
      }

      std::string extractString(sqlite3_stmt* stmt, int pos)
      {
        std::string res;
        if (!extractValue<std::string>(&res, stmt, pos))
        {
          throw Exception::SqlOperationFailed(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "Conversion of column " + StringUtils::toStr(pos) + " to std::string failed");
        }
        return res;
      }

      char extractChar(sqlite3_stmt* stmt, int pos)
      {
        return extractString(stmt, pos)[0];
      }

      bool extractBool(sqlite3_stmt* stmt, int pos)
      {
        return extractInt(stmt, pos) != 0;
      }

    }

}
