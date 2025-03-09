#include "saves.h"
#include <iostream>

bool SavesModule::init(const QSettings& settings) {
    databaseName = settings.value("Settings/DatabaseName", "savegame.db").toString().toStdString();
    maxSlots = settings.value("Settings/MaxSlots", 10).toInt();
    savePath = settings.value("Settings/SavePath", "").toString().toStdString();

    std::string fullPath = savePath.empty() ? databaseName : savePath + "/" + databaseName;
    int rc = sqlite3_open(fullPath.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS saves (slot INTEGER PRIMARY KEY, data TEXT);";
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }
    return true;
}

bool SavesModule::save(const std::string& data) {
    if (!db) return false;
    std::string sql = "INSERT OR REPLACE INTO saves (slot, data) VALUES (1, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, data.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
    return false;
}

std::string SavesModule::load() {
    if (!db) return "";
    std::string sql = "SELECT data FROM saves WHERE slot = 1;";
    sqlite3_stmt* stmt;
    std::string data = "";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }
    return data;
}

void SavesModule::shutdown() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

SavesModule::~SavesModule() {
    shutdown();
}