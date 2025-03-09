#include "VisualNovelEngine.h"
#include "libs/standard/opengl/opengl.h"
#include "libs/standard/vulkan/vulkan.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <fstream>
#include <stdexcept>
#include <iostream>

typedef Module* (*CreateModuleFunc)();

VisualNovelEngine::VisualNovelEngine(const ProjectConfig& config) : config(config) {
    loadRenderModule();
    loadCustomModules();
}

VisualNovelEngine::~VisualNovelEngine() {
    if (renderModule) renderModule->cleanup();
    for (auto& [handle, module] : customModules) {
        if (module) module->shutdown();
#ifdef _WIN32
        if (handle) FreeLibrary((HMODULE)handle);
#else
        if (handle) dlclose(handle);
#endif
    }
}

void VisualNovelEngine::loadRenderModule() {
    if (config.renderApi == "vulkan") {
        renderModule = std::make_unique<VulkanRenderModule>();
    } else if (config.renderApi == "opengl") {
        renderModule = std::make_unique<OpenGLRenderModule>();
    } else {
        throw std::runtime_error("Unsupported render API: " + config.renderApi);
    }
}

void VisualNovelEngine::loadCustomModules() {
    QDir dir("libs/custom");
    if (!dir.exists()) {
        std::cerr << "Custom modules directory not found: libs/custom\n";
        return;
    }

    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& libName : config.libraries) {
        if (!dirs.contains(QString::fromStdString(libName))) {
            std::cerr << "Module " << libName << " not found in libs/custom\n";
            continue;
        }

        std::string cfgPath = "libs/custom/" + libName + "/" + libName + ".cfg";
        QSettings settings(QString::fromStdString(cfgPath), QSettings::IniFormat);

        std::string moduleName = settings.value("Module/Name", libName).toString().toStdString();
        std::cout << "Loading custom module: " << moduleName << "\n";

        std::string libPath = "libs/custom/" + libName + "/lib" + libName + MODULE_EXT;
        void* handle = nullptr;
#ifdef _WIN32
        handle = (void*)LoadLibraryA(libPath.c_str());
        if (!handle) {
            std::cerr << "Failed to load DLL " << libPath << ": " << GetLastError() << "\n";
            continue;
        }
        CreateModuleFunc createFunc = (CreateModuleFunc)GetProcAddress((HMODULE)handle, "createModule");
#else
        handle = dlopen(libPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Failed to load SO " << libPath << ": " << dlerror() << "\n";
            continue;
        }
        CreateModuleFunc createFunc = (CreateModuleFunc)dlsym(handle, "createModule");
#endif
        if (!createFunc) {
            std::cerr << "Failed to load createModule function for " << libName << "\n";
#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
            continue;
        }

        std::unique_ptr<Module> module(createFunc());
        if (!module->init(settings)) {
            std::cerr << "Failed to initialize custom module: " << libName << "\n";
#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
            continue;
        }

        SaveModule* saveMod = dynamic_cast<SaveModule*>(module.get());
        if (saveMod) {
            saveModule = std::unique_ptr<SaveModule>(saveMod);
            module.release();
        }

        customModules.emplace_back(handle, std::move(module));
    }
}

// Остальные методы остаются без изменений (init, render, loadScript, nextLine, loadImage, renderText, start)
bool VisualNovelEngine::init(const std::string& scriptPath, const std::string& savePath) {
    if (!renderModule->init(config)) throw std::runtime_error("Failed to initialize render module");
    if (saveModule) {
        QSettings settings("save.cfg", QSettings::IniFormat);
        settings.setValue("Settings/SavePath", QString::fromStdString(savePath));
        if (!saveModule->init(settings)) std::cerr << "Warning: Save module initialization failed\n";
    }
    return loadScript(scriptPath);
}

void VisualNovelEngine::render() {
    if (renderModule) renderModule->render(currentImages);
    else std::cerr << "No render module loaded!\n";
}

bool VisualNovelEngine::loadScript(const std::string& scriptPath) {
    std::ifstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "Failed to open script file: " << scriptPath << "\n";
        return false;
    }
    std::string line;
    while (std::getline(scriptFile, line)) scriptLines.push_back(line);
    scriptFile.close();
    currentLineIndex = 0;
    return true;
}

bool VisualNovelEngine::nextLine() {
    if (currentLineIndex >= scriptLines.size()) return false;
    currentImages.clear();
    currentLineIndex++;
    return true;
}

void VisualNovelEngine::loadImage(const std::string& imageName, SDL_Surface* surface) {
    if (renderModule) {
        renderModule->loadImage(imageName, surface);
        currentImages.push_back({imageName, 0, 0, surface->w, surface->h});
    }
}

void VisualNovelEngine::renderText(const std::string& textKey, SDL_Surface* surface, int x, int y, int w, int h) {
    if (renderModule) {
        renderModule->renderText(textKey, surface, x, y, w, h);
        currentImages.push_back({textKey, x, y, w, h});
    }
}

void VisualNovelEngine::start() {
    while (nextLine()) render();
}