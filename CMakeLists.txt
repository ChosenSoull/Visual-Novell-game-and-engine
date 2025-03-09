cmake_minimum_required(VERSION 3.10)
project(PhantomGameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_WIN32)
    set(MODULE_EXT ".dll")
else()
    set(MODULE_EXT ".so")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs/standart
    ${CMAKE_SOURCE_DIR}/libs/custom/saves
    ${CMAKE_SOURCE_DIR}/libs/standart/physx/include
)

find_package(Qt5 COMPONENTS Widgets Gui Core REQUIRED)
find_package(SDL2 REQUIRED)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
find_library(VULKAN_LIBRARY NAMES vulkan)
find_library(GLEW_LIBRARY NAMES GLEW glew32)

# Настройка PhysX
set(PHYSX_ROOT_DIR ${CMAKE_SOURCE_DIR}/libs/standart/physx)
if(WIN32)
    set(PHYSX_LIBRARIES
        ${PHYSX_ROOT_DIR}/lib/PhysX_64.lib
        ${PHYSX_ROOT_DIR}/lib/PhysXCommon_64.lib
        ${PHYSX_ROOT_DIR}/lib/PxFoundation_64.lib
    )
else()
    set(PHYSX_LIBRARIES
        ${PHYSX_ROOT_DIR}/lib/libPhysX_64.a
        ${PHYSX_ROOT_DIR}/lib/libPhysXCommon_64.a
        ${PHYSX_ROOT_DIR}/lib/libPxFoundation_64.a
    )
endif()

set(ENGINE_SOURCES
    VisualNovelEngine.cpp
    mainwindow.cpp
    newprojectdialog.cpp
    settingsdialog.cpp
    startupdialog.cpp
    main.cpp
)

set(STANDARD_MODULES
    libs/standart/opengl/opengl.cpp
    libs/standart/vulkan/vulkan.cpp
    libs/standart/physx/physx.cpp
)

add_executable(phantom_engine
    ${ENGINE_SOURCES}
    ${STANDARD_MODULES}
)

target_link_libraries(phantom_engine
    Qt5::Widgets
    Qt5::Gui
    Qt5::Core
    ${SDL2_LIBRARIES}
    ${SQLITE3_LIBRARY}
    ${VULKAN_LIBRARY}
    ${GLEW_LIBRARY}
    OpenGL::GL
    ${PHYSX_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(phantom_engine dl)
endif()

if(WIN32)
    target_link_libraries(phantom_engine user32)
endif()

add_subdirectory(libs/custom/saves)

# Копирование динамических библиотек PhysX для запуска
if(WIN32)
    add_custom_command(TARGET phantom_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PHYSX_ROOT_DIR}/bin/PhysX_64.dll"
        "${PHYSX_ROOT_DIR}/bin/PhysXCommon_64.dll"
        "${PHYSX_ROOT_DIR}/bin/PxFoundation_64.dll"
        $<TARGET_FILE_DIR:phantom_engine>
    )
else()
    add_custom_command(TARGET phantom_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PHYSX_ROOT_DIR}/bin/libPhysX_64.so"
        "${PHYSX_ROOT_DIR}/bin/libPhysXCommon_64.so"
        "${PHYSX_ROOT_DIR}/bin/libPxFoundation_64.so"
        $<TARGET_FILE_DIR:phantom_engine>
    )
endif()