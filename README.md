# VisualNovelEngine

`VisualNovelEngine` — это движок для создания визуальных новелл с поддержкой графики (Vulkan/OpenGL), звука, видео и системы сохранений. Управление игрой осуществляется через C++ код, без использования внешних скриптов.

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

qmake game_creator.pro
make
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

qmake game_creator.pro
make

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