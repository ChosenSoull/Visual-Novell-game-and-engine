# VisualNovelEngine

`VisualNovelEngine` — это движок для создания визуальных новелл с поддержкой графики (Vulkan/OpenGL), звука, видео и системы сохранений. Управление игрой осуществляется через C++ код, без использования внешних скриптов. Движок поддерживает состояния (`MENU`, `GAME`, `PAUSE`), приостанавливая текст и видео в паузе, сохраняя воспроизведение музыки.

## Требования

- **Библиотеки:**
  - SDL2 (`SDL2`, `SDL2_ttf`, `SDL2_image`, `SDL2_mixer`)
  - FFmpeg (`libavcodec`, `libavformat`, `libavutil`, `libswscale`, `libswresample`)
  - SQLite3
  - Vulkan SDK (для Vulkan-рендеринга)
  - GLAD (для OpenGL-рендеринга)

- **Компилятор:** C++17 или новее (`g++`, `clang++`, `MSVC`).

- **Файлы ресурсов:**
  - Изображения (`.png`, `.jpg`)
  - Звуки (`.wav`)
  - Музыка (`.mp3`)
  - Видео (`.mp4`)
  - Шрифт (`font.ttf`)
  - Шейдеры GLSL (`shader.vert`, `shader.frag`) для Vulkan

## Команда компилирования

### Linux
1. Скомпилируйте шейдеры для Vulkan:
   ```bash
   glslc shader.vert -o vert.spv
   glslc shader.frag -o frag.spv

    Скомпилируйте движок:
    bash

g++ -std=c++17 engine.c files/src/glad.c -o VisualNovelEngine     -I/usr/include/SDL2 -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer     -lavcodec -lavformat -lavutil -lswscale -lswresample     -lsqlite3 -lvulkan     -pthread
Для OpenGL замените -lvulkan на -lGL -lGLU -lGLEW.
(Опционально) Укажите рендеринг через переменную окружения:
bash

    export RENDER_API=opengl  # или vulkan (по умолчанию)
    ./VisualNovelEngine

Windows

    Скомпилируйте шейдеры для Vulkan (требуется Vulkan SDK):
    cmd

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
Скомпилируйте движок (пример для MSYS2/MinGW):
cmd

    g++ -std=c++17 engine.c -o VisualNovelEngine.exe \
        -I"C:\SDL2\include" -L"C:\SDL2\lib" -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer \
        -lavcodec -lavformat -lavutil -lswscale -lswresample \
        -lsqlite3 -lvulkan-1 \
        -pthread
    Укажите правильные пути к библиотекам. Для OpenGL замените -lvulkan-1 на -lopengl32 -lglu32 -lglew32.

Запуск
bash
./VisualNovelEngine  # Linux
VisualNovelEngine.exe  # Windows
Конфигурация

Движок использует файл config.ini для выбора рендеринга, расположенный в:

    Linux: ~/Documents/NovelGame/config.ini
    Windows: %USERPROFILE%\Documents\NovelGame\config.ini

Если файл отсутствует, он создается автоматически с настройкой по умолчанию:
text
[Rendering]
api=vulkan
Поддерживаемые значения api:

    vulkan (по умолчанию)
    opengl

В Linux также можно переопределить рендеринг через переменную окружения RENDER_API.
Сохранения

Сохранения хранятся в базе данных SQLite savegame.db в той же папке:

    Linux: ~/Documents/NovelGame/savegame.db
    Windows: %USERPROFILE%\Documents\NovelGame\savegame.db

Структура проекта

    engine.c: Основной файл с кодом движка (C++).
    shader.vert, shader.frag: Исходные шейдеры для Vulkan.
    Ресурсы (изображения, звуки, музыка, видео) должны быть в рабочей директории.

Описание публичных функций
Конструктор и запуск

    VisualNovelEngine()
        Конструктор движка. Автоматически определяет API рендеринга из config.ini или RENDER_API.
        Пример: VisualNovelEngine engine;
    bool start()
        Запускает движок и игровой цикл.
        Возвращает: true при успехе, false при ошибке.
        Пример: engine.start();

Графические функции

    void GShowImage(const std::string& imageName, int x, int y, int w, int h)
        Отображает изображение.
        Параметры: imageName (без расширения), x, y (координаты), w, h (размеры).
        Пример: engine.GShowImage("background", 0, 0, 1920, 1080);
    void GTextToScreen(const std::string& text, const std::string& fontName, int x, int y, SDL_Color color, const std::string& backgroundImage = "")
        Выводит текст с опциональным фоном.
        Пример: engine.GTextToScreen("Hello!", "default", 100, 900, {255, 255, 255, 255}, "dialog_bg");
    void GClearScreen()
        Очищает экран.
        Пример: engine.GClearScreen();
    void GPlayVideo(const std::string& videoName, int x = 0, int y = 0, int w = 1920, int h = 1080)
        Воспроизводит видео. Останавливается в PAUSE.
        Пример: engine.GPlayVideo("intro");

Аудио-функции

    void MPlaySound(const std::string& soundName)
        Воспроизводит звук.
        Пример: engine.MPlaySound("click");
    void MPlayBackgroundMusic(const std::string& musicName, int loops = -1)
        Воспроизводит музыку (работает в PAUSE).
        Пример: engine.MPlayBackgroundMusic("bgm", -1);
    void MStopBackgroundMusic()
        Останавливает музыку.
        Пример: engine.MStopBackgroundMusic();
    void MSetVolume(int newVolume)
        Устанавливает громкость (0–128).
        Пример: engine.MSetVolume(64);

Сохранение и загрузка

    void SSaveGame(int slot = 1)
        Сохраняет игру в слот.
        Пример: engine.SSaveGame(1);
    void SLoadGame(int slot = 1)
        Загружает игру из слота.
        Пример: engine.SLoadGame(1);