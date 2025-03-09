# CMakeLists.txt в libs/custom/saves/
cmake_minimum_required(VERSION 3.10)
project(saves)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исходные файлы модуля
add_library(saves MODULE
    saves.cpp
)

# Указываем, что это динамическая библиотека
set_target_properties(saves PROPERTIES
    PREFIX ""
    SUFFIX ${MODULE_EXT}
)

# Добавляем пути для include
target_include_directories(saves PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs/custom/saves
)

# Линкуем с SQLite
find_library(SQLITE3_LIBRARY NAMES sqlite3)
target_link_libraries(saves PRIVATE ${SQLITE3_LIBRARY})

# Указываем, куда копировать готовую библиотеку
add_custom_command(TARGET saves POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:saves>
    ${CMAKE_SOURCE_DIR}/libs/custom/saves/libsaves${MODULE_EXT}
)