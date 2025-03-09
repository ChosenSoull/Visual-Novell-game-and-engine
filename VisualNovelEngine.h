#ifndef VISUAL_NOVEL_ENGINE_H
#define VISUAL_NOVEL_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <SDL2/SDL.h>
#include <stdexcept>
#include <QtCore/QSettings>
#include <QtCore/QDir>

#ifdef _WIN32
#define MODULE_EXT ".dll"
#else
#define MODULE_EXT ".so"
#endif

struct ProjectConfig {
    std::string path;
    std::string renderApi; // "opengl" или "vulkan"
    std::vector<std::string> libraries; // Список модулей
};

struct DisplayImage {
    std::string name;
    int x, y, w, h;
};

class RenderModule {
public:
    virtual ~RenderModule() = default;
    virtual bool init(const ProjectConfig& config) = 0;
    virtual void render(const std::vector<DisplayImage>& images) = 0;
    virtual void cleanup() = 0;
    virtual void loadImage(const std::string& imageName, SDL_Surface* surface) = 0;
    virtual void renderText(const std::string& textKey, SDL_Surface* surface, int x, int y, int w, int h) = 0;
};

class Module {
public:
    virtual ~Module() = default;
    virtual bool init(const QSettings& settings) = 0;
    virtual void shutdown() = 0;
};

class SaveModule : public Module {
public:
    virtual bool save(const std::string& data) = 0;
    virtual std::string load() = 0;
};

class VisualNovelEngine {
private:
    std::unique_ptr<RenderModule> renderModule;
    std::unique_ptr<SaveModule> saveModule;
    std::vector<std::string> scriptLines;
    std::vector<DisplayImage> currentImages;
    size_t currentLineIndex = 0;
    ProjectConfig config;
    std::vector<std::pair<void*, std::unique_ptr<Module>>> customModules;

    void loadRenderModule();
    void loadCustomModules();

public:
    VisualNovelEngine(const ProjectConfig& config);
    ~VisualNovelEngine();

    bool init(const std::string& scriptPath, const std::string& savePath);
    void render();
    bool loadScript(const std::string& scriptPath);
    bool nextLine();
    void loadImage(const std::string& imageName, SDL_Surface* surface);
    void renderText(const std::string& textKey, SDL_Surface* surface, int x, int y, int w, int h);
    void start();
};

#endif // VISUAL_NOVEL_ENGINE_H