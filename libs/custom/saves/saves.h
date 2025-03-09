#ifndef SAVES_MODULE_H
#define SAVES_MODULE_H

#include "VisualNovelEngine.h"
#include <QtCore/QSettings>
#include <sqlite3.h>

extern "C" Module* createModule(); // Экспортируем функцию для dlopen

class SavesModule : public SaveModule {
private:
    sqlite3* db;
    std::string databaseName;
    int maxSlots;
    std::string savePath;

public:
    SavesModule() : db(nullptr), databaseName("savegame.db"), maxSlots(10), savePath("") {}

    bool init(const QSettings& settings) override;
    bool save(const std::string& data) override;
    std::string load() override;
    void shutdown() override;
    ~SavesModule();
};

extern "C" Module* createModule() {
    return new SavesModule();
}

#endif // SAVES_MODULE_H