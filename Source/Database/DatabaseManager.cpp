#include "Database/DatabaseManager.h"

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::initialize(const juce::File &dbFile) {
  close();

  // Ensure directory exists
  dbFile.getParentDirectory().createDirectory();

  if (sqlite3_open(dbFile.getFullPathName().toUTF8(), &db) != SQLITE_OK) {
    DBG("Can't open database: " << sqlite3_errmsg(db));
    return false;
  }

  // Enable WAL for performance
  char *zErrMsg = 0;
  sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, &zErrMsg);
  if (zErrMsg)
    sqlite3_free(zErrMsg);

  createTables();
  return true;
}

void DatabaseManager::close() {
  if (insertStmt) {
    sqlite3_finalize(insertStmt);
    insertStmt = nullptr;
  }

  if (db) {
    sqlite3_close(db);
    db = nullptr;
  }
}

void DatabaseManager::createTables() {
  if (!db)
    return;

  const char *sql =
      "CREATE TABLE IF NOT EXISTS songs ("
      "id TEXT PRIMARY KEY, "
      "title TEXT, "
      "artist TEXT, "
      "key TEXT, "
      "tempo INTEGER, "
      "path TEXT, "
      "lyrics TEXT"
      ");"

      "DROP TABLE IF EXISTS songs_fts;"
      "DROP TRIGGER IF EXISTS songs_ai;"
      "DROP TRIGGER IF EXISTS songs_ad;"
      "DROP TRIGGER IF EXISTS songs_au;"

      // FTS5 Virtual Table
      "CREATE VIRTUAL TABLE IF NOT EXISTS songs_fts USING fts5(id, title, "
      "artist);"

      // Triggers to keep FTS table in sync
      "CREATE TRIGGER IF NOT EXISTS songs_ai AFTER INSERT ON songs BEGIN "
      "  INSERT INTO songs_fts(id, title, artist) VALUES (new.id, new.title, "
      "new.artist); "
      "END;"

      "CREATE TRIGGER IF NOT EXISTS songs_ad AFTER DELETE ON songs BEGIN "
      "  DELETE FROM songs_fts WHERE id = old.id; "
      "END;"

      "CREATE TRIGGER IF NOT EXISTS songs_au AFTER UPDATE ON songs BEGIN "
      "  UPDATE songs_fts SET title = new.title, artist = new.artist WHERE id "
      "= old.id; "
      "END;"

      "CREATE INDEX IF NOT EXISTS idx_title ON songs(title);"
      "CREATE INDEX IF NOT EXISTS idx_artist ON songs(artist);"
      "CREATE INDEX IF NOT EXISTS idx_id ON songs(id);";

  char *zErrMsg = 0;
  if (sqlite3_exec(db, sql, 0, 0, &zErrMsg) != SQLITE_OK) {
    DBG("SQL error generating tables: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
}

bool DatabaseManager::beginBulkInsert() {
  if (!db)
    return false;

  // Clear existing
  sqlite3_exec(db, "DELETE FROM songs; DELETE FROM songs_fts;", 0, 0, 0);
  sqlite3_exec(db, "VACUUM;", 0, 0, 0);

  // Begin transaction
  sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

  const char *insertSql = "INSERT INTO songs (id, title, artist, key, tempo, "
                          "path, lyrics) VALUES (?, ?, ?, ?, ?, ?, ?)";
  sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, 0);

  return true;
}

bool DatabaseManager::insertSong(const SongRecord &song) {
  if (!insertStmt)
    return false;

  sqlite3_bind_text(insertStmt, 1, song.id.toUTF8(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 2, song.title.toUTF8(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 3, song.artist.toUTF8(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 4, song.key.toUTF8(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(insertStmt, 5, song.tempo);
  sqlite3_bind_text(insertStmt, 6, song.path.toUTF8(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 7, song.lyrics.toUTF8(), -1, SQLITE_TRANSIENT);

  int rc = sqlite3_step(insertStmt);
  sqlite3_reset(insertStmt);

  return (rc == SQLITE_DONE);
}

bool DatabaseManager::commitBulkInsert() {
  if (!db)
    return false;

  if (insertStmt) {
    sqlite3_finalize(insertStmt);
    insertStmt = nullptr;
  }

  return sqlite3_exec(db, "COMMIT;", 0, 0, 0) == SQLITE_OK;
}

bool DatabaseManager::rollbackBulkInsert() {
  if (!db)
    return false;

  if (insertStmt) {
    sqlite3_finalize(insertStmt);
    insertStmt = nullptr;
  }

  return sqlite3_exec(db, "ROLLBACK;", 0, 0, 0) == SQLITE_OK;
}

int DatabaseManager::getTotalCount() {
  if (!db)
    return 0;

  int count = 0;
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM songs", -1, &stmt, 0) ==
      SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }
  return count;
}

juce::Array<SongRecord> DatabaseManager::searchSongs(const juce::String &query,
                                                     int limit) {
  juce::Array<SongRecord> results;
  if (!db)
    return results;

  sqlite3_stmt *stmt;
  juce::String sql;

  if (query.trim().isEmpty()) {
    sql =
        "SELECT id, title, artist, key, tempo, path, lyrics FROM songs LIMIT " +
        juce::String(limit);
    if (sqlite3_prepare_v2(db, sql.toUTF8(), -1, &stmt, 0) == SQLITE_OK) {
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.add(readRecordFromStatement(stmt));
      }
      sqlite3_finalize(stmt);
    }
  } else {
    // FTS Match Query: Add * to each word for prefix matching
    juce::StringArray parts;
    parts.addTokens(query, " ", "\"");

    juce::String ftsQuery;
    for (int i = 0; i < parts.size(); ++i) {
      ftsQuery << "\"" << parts[i] << "\"* ";
      if (i < parts.size() - 1)
        ftsQuery << "AND ";
    }

    sql = "SELECT s.id, s.title, s.artist, s.key, s.tempo, s.path, s.lyrics "
          "FROM songs_fts f "
          "JOIN songs s ON f.id = s.id "
          "WHERE songs_fts MATCH ? "
          "ORDER BY rank "
          "LIMIT ?";

    if (sqlite3_prepare_v2(db, sql.toUTF8(), -1, &stmt, 0) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, ftsQuery.toUTF8(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 2, limit);

      while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.add(readRecordFromStatement(stmt));
      }
      sqlite3_finalize(stmt);
    }

    // Fallback for FTS empty
    if (results.isEmpty()) {
      sql = "SELECT id, title, artist, key, tempo, path, lyrics FROM songs "
            "WHERE id LIKE ? OR title LIKE ? OR artist LIKE ? LIMIT ?";
      if (sqlite3_prepare_v2(db, sql.toUTF8(), -1, &stmt, 0) == SQLITE_OK) {
        juce::String likeQ = "%" + query + "%";
        sqlite3_bind_text(stmt, 1, likeQ.toUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, likeQ.toUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, likeQ.toUTF8(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
          results.add(readRecordFromStatement(stmt));
        }
        sqlite3_finalize(stmt);
      }
    }
  }

  return results;
}

SongRecord DatabaseManager::readRecordFromStatement(sqlite3_stmt *stmt) {
  SongRecord record;

  // Note: sqlite3_column_text can return null, so check before casting
  const unsigned char *cId = sqlite3_column_text(stmt, 0);
  const unsigned char *cTitle = sqlite3_column_text(stmt, 1);
  const unsigned char *cArtist = sqlite3_column_text(stmt, 2);
  const unsigned char *cKey = sqlite3_column_text(stmt, 3);
  const unsigned char *cPath = sqlite3_column_text(stmt, 5);
  const unsigned char *cLyrics = sqlite3_column_text(stmt, 6);

  if (cId)
    record.id = juce::String::fromUTF8(reinterpret_cast<const char *>(cId));
  if (cTitle)
    record.title =
        juce::String::fromUTF8(reinterpret_cast<const char *>(cTitle));
  if (cArtist)
    record.artist =
        juce::String::fromUTF8(reinterpret_cast<const char *>(cArtist));
  if (cKey)
    record.key = juce::String::fromUTF8(reinterpret_cast<const char *>(cKey));
  record.tempo = sqlite3_column_int(stmt, 4);
  if (cPath)
    record.path = juce::String::fromUTF8(reinterpret_cast<const char *>(cPath));
  if (cLyrics)
    record.lyrics =
        juce::String::fromUTF8(reinterpret_cast<const char *>(cLyrics));

  return record;
}

