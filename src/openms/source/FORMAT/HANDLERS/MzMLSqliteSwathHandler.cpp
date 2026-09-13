// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/HANDLERS/MzMLSqliteSwathHandler.h>

#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/DATASTRUCTURES/StringUtils.h>

#include <OpenMS/FORMAT/SqliteConnector_impl.h>
#include <sqlite3.h>

#include <memory>

namespace OpenMS::Internal
{


    namespace Sql = Internal::SqliteHelper;

    namespace
    {
      /// Finalises the prepared statement on every exit, including a failed step
      using StatementGuard = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
    }

    // All three accessors open the file read-only: the default mode created a missing file, whose
    // query then failed on a missing table instead of on the missing file. They advance with
    // Sql::nextRow, which throws on a failed step: taking a NULL first column for the end of the
    // result made an error look like a complete (possibly empty) result.

    std::vector<OpenSwath::SwathMap> MzMLSqliteSwathHandler::readSwathWindows()
    {
      std::vector<OpenSwath::SwathMap> swath_maps;
      SqliteConnector conn(filename_, SqliteConnector::SqlOpenMode::READ_ONLY);
      sqlite3_stmt * stmt;

      // DISTINCT applies to the whole row (centre and both bounds); a precursor without an
      // isolation target has no window and used to end the loop as if it were the last row
      std::string select_sql;
      select_sql = "SELECT " \
                    "DISTINCT(ISOLATION_TARGET)," \
                    "ISOLATION_TARGET - ISOLATION_LOWER," \
                    "ISOLATION_TARGET + ISOLATION_UPPER " \
                    "FROM PRECURSOR " \
                    "INNER JOIN SPECTRUM ON SPECTRUM_ID = SPECTRUM.ID " \
                    "WHERE MSLEVEL == 2 AND ISOLATION_TARGET IS NOT NULL "\
                    ";";

      Internal::SqliteHelper::prepareStatement(conn, &stmt, select_sql);
      StatementGuard guard(stmt, &sqlite3_finalize);

      Sql::SqlState state = Sql::SqlState::SQL_ROW;
      while ((state = Sql::nextRow(stmt, state)) == Sql::SqlState::SQL_ROW)
      {
        OpenSwath::SwathMap map;
        Sql::extractValue<double>(&map.center, stmt, 0);
        Sql::extractValue<double>(&map.lower, stmt, 1);
        Sql::extractValue<double>(&map.upper, stmt, 2);
        swath_maps.push_back(map);
      }

      return swath_maps;
    }

    std::vector<int> MzMLSqliteSwathHandler::readMS1Spectra()
    {
      std::vector< int > indices;
      SqliteConnector conn(filename_, SqliteConnector::SqlOpenMode::READ_ONLY);
      sqlite3_stmt * stmt;

      std::string select_sql;
      select_sql = "SELECT ID " \
                   "FROM SPECTRUM " \
                   "WHERE MSLEVEL == 1;";

      Internal::SqliteHelper::prepareStatement(conn, &stmt, select_sql);
      StatementGuard guard(stmt, &sqlite3_finalize);

      Sql::SqlState state = Sql::SqlState::SQL_ROW;
      while ((state = Sql::nextRow(stmt, state)) == Sql::SqlState::SQL_ROW)
      {
        indices.push_back(sqlite3_column_int(stmt, 0));
      }

      return indices;
    }

    std::vector<int> MzMLSqliteSwathHandler::readSpectraForWindow(const OpenSwath::SwathMap& swath_map)
    {
      std::vector< int > indices;
      const double center = swath_map.center;

      SqliteConnector conn(filename_, SqliteConnector::SqlOpenMode::READ_ONLY);
      sqlite3_stmt * stmt;

      // chromatogram precursors share the table and have no SPECTRUM_ID: the writer stores
      // chromatograms first, so one with a precursor in the window ended the loop early
      std::string select_sql = "SELECT " \
                          "SPECTRUM_ID " \
                          "FROM PRECURSOR " \
                          "WHERE SPECTRUM_ID IS NOT NULL AND ISOLATION_TARGET BETWEEN ";

      select_sql +=StringUtils::toStr(center - 0.01) + " AND " + StringUtils::toStr(center + 0.01) + ";";
      Internal::SqliteHelper::prepareStatement(conn, &stmt, select_sql);
      StatementGuard guard(stmt, &sqlite3_finalize);

      Sql::SqlState state = Sql::SqlState::SQL_ROW;
      while ((state = Sql::nextRow(stmt, state)) == Sql::SqlState::SQL_ROW)
      {
        indices.push_back(sqlite3_column_int(stmt, 0));
      }

      return indices;
    }

} // namespace OpenMS  // namespace Internal

