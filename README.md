VisualNovelEngine

VisualNovelEngine — это движок для создания визуальных новелл с поддержкой графики (Vulkan/OpenGL), звука, видео, скриптов и системы сохранений. Движок поддерживает состояния игры (MENU, GAME, PAUSE), позволяя приостанавливать текст и видео в паузе, сохраняя при этом воспроизведение фоновой музыки.
Требования

    Библиотеки:
        SDL2 (SDL2, SDL2_ttf, SDL2_image, SDL2_mixer)
        FFmpeg (libavcodec, libavformat, libavutil, libswscale, libswresample)
        SQLite3
        Vulkan SDK (опционально, для Vulkan-рендеринга)
        GLAD (опционально, для OpenGL-рендеринга)
        nlohmann/json (JSON для C++)
    Компилятор: C++17 или новее (например, g++, clang++, MSVC).
    Файлы ресурсов:
        Изображения (.png, .jpg)
        Звуки (.wav)
        Музыка (.mp3)
        Видео (.mp4)
        Шрифт (font.ttf)
        Шейдеры для Vulkan (vert.spv, frag.spv)

Команда компилирования

Для компиляции с использованием g++ выполните следующую команду (предполагается, что все зависимости установлены):
bash
g++ -std=c++17 engine.c -o VisualNovelEngine \
    -I/usr/include/SDL2 -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer \
    -lavcodec -lavformat -lavutil -lswscale -lswresample \
    -lsqlite3 -lvulkan -lGL -lGLU -lGLEW \
    -pthread
Пояснения:

    -std=c++17: Используется стандарт C++17.
    -I/usr/include/SDL2: Путь к заголовкам SDL2 (может отличаться в зависимости от системы).
    -l<библиотека>: Подключение библиотек.
    -pthread: Поддержка многопоточности.
    Файл engine.c содержит весь код движка (в данном случае расширение .c используется условно, так как код написан на C++).

Для OpenGL вместо Vulkan замените -lvulkan на -lGL -lGLU -lGLEW и укажите false в конструкторе VisualNovelEngine.
Примечание:

    Убедитесь, что пути к библиотекам и заголовкам соответствуют вашей системе (например, на Windows может потребоваться MSYS2 или другая настройка).
    Шейдеры (vert.spv, frag.spv) нужно скомпилировать из GLSL с помощью glslc:
    bash

    glslc shader.vert -o vert.spv
    glslc shader.frag -o frag.spv

Запуск

После компиляции запустите программу:
bash
./VisualNovelEngine
Структура проекта

    engine.c: Основной файл с кодом движка.
    script.txt: Файл скрипта для управления сюжетом.
    Ресурсы (изображения, звуки, музыка, видео) должны быть в рабочей директории.

Описание публичных функций

Движок предоставляет следующие публичные методы для создания визуальной новеллы:
Конструктор и запуск

    VisualNovelEngine(bool useVulkan_ = true)
        Конструктор движка.
        Параметры:
            useVulkan_: Если true, используется Vulkan, иначе OpenGL.
        Пример: VisualNovelEngine engine(true);
    bool start()
        Запускает движок, инициализирует графику, звук и основной цикл.
        Возвращает: true при успешном запуске, false при ошибке.
        Пример: engine.start();

Графические функции

    void GShowImage(const std::string& imageName, int x, int y, int w, int h)
        Отображает изображение на экране.
        Параметры:
            imageName: Имя файла изображения (без расширения, ожидается .png или .jpg).
            x, y: Координаты верхнего левого угла.
            w, h: Ширина и высота изображения.
        Пример: engine.GShowImage("background", 0, 0, 1920, 1080);
    void GTextToScreen(const std::string& text, const std::string& fontName, int x, int y, SDL_Color color, const std::string& backgroundImage = "")
        Выводит текст на экран с опциональным фоном.
        Параметры:
            text: Отображаемый текст.
            fontName: Имя шрифта (например, "default", ожидается font.ttf).
            x, y: Координаты текста.
            color: Цвет текста в формате {r, g, b, a} (например, {255, 255, 255, 255} для белого).
            backgroundImage: Опциональное имя фонового изображения (без расширения).
        Пример: engine.GTextToScreen("Hello!", "default", 100, 900, {255, 255, 255, 255}, "dialog_bg");
    void GClearScreen()
        Очищает экран от всех отображаемых объектов (изображений и текста).
        Пример: engine.GClearScreen();
    void GSetImageEffect(const std::string& imageName, const std::string& effect)
        Применяет эффект к изображению (заглушка, пока поддерживает только "fade").
        Параметры:
            imageName: Имя изображения.
            effect: Название эффекта (например, "fade").
        Пример: engine.GSetImageEffect("background", "fade");
    void GPlayVideo(const std::string& videoName, int x = 0, int y = 0, int w = 1920, int h = 1080)
        Воспроизводит видео с аудио.
        Параметры:
            videoName: Имя файла видео (без расширения, ожидается .mp4).
            x, y: Координаты видео (по умолчанию 0, 0).
            w, h: Размеры видео (по умолчанию 1920x1080).
        Примечание: Видео останавливается в состоянии PAUSE.
        Пример: engine.GPlayVideo("intro", 0, 0, 1920, 1080);

Аудио-функции

    void MPlaySound(const std::string& soundName)
        Воспроизводит звуковой эффект.
        Параметры:
            soundName: Имя файла звука (без расширения, ожидается .wav).
        Пример: engine.MPlaySound("click");
    void MPlayBackgroundMusic(const std::string& musicName, int loops = -1)
        Воспроизводит фоновую музыку.
        Параметры:
            musicName: Имя файла музыки (без расширения, ожидается .mp3).
            loops: Количество повторов (-1 для бесконечного воспроизведения).
        Примечание: Музыка продолжает играть в состоянии PAUSE.
        Пример: engine.MPlayBackgroundMusic("bgm", -1);
    void MStopBackgroundMusic()
        Останавливает воспроизведение фоновой музыки.
        Пример: engine.MStopBackgroundMusic();
    void MSetVolume(int newVolume)
        Устанавливает громкость звука.
        Параметры:
            newVolume: Значение громкости (0–MIX_MAX_VOLUME, обычно 128).
        Пример: engine.MSetVolume(64);

Сохранение и загрузка

    void SSaveGame(int slot = 1)
        Сохраняет текущее состояние игры в указанный слот.
        Параметры:
            slot: Номер слота сохранения (по умолчанию 1).
        Сохраняет: Позицию в скрипте, отображаемые изображения, переменные.
        Пример: engine.SSaveGame(1);
    void SLoadGame(int slot = 1)
        Загружает состояние игры из указанного слота.
        Параметры:
            slot: Номер слота загрузки (по умолчанию 1).
        Пример: engine.SLoadGame(1);

Управление состояниями

Движок автоматически обрабатывает состояния:

    MENU: Отображает главное меню ("Start Game", "Exit").
    GAME: Выполняет скрипт, отображает текст, изображения, видео.
    PAUSE: Приостанавливает текст и видео, сохраняя музыку.

Переходы:

    Из MENU в GAME: Клик по "Start Game".
    Из GAME в PAUSE: Нажатие ESC.
    Из PAUSE в GAME: Нажатие ESC.
    Из PAUSE в MENU: Клик по "Main Menu".

Пример скрипта (script.txt)
text
show image background 0 0 1920 1080
say Hello, welcome to the game!
play music bgm
play video intro
clear
say Goodbye!
Дополнительные заметки

    Ресурсы загружаются асинхронно в отдельном потоке.
    Для Vulkan требуется наличие скомпилированных шейдеров (vert.spv, frag.spv).
    Ошибки логируются в консоль через logError.