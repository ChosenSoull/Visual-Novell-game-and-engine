#ifndef OPENGL_RENDER_MODULE_H
#define OPENGL_RENDER_MODULE_H

#include <SDL2/SDL.h>
#include "files/include/glad/glad.h"
#include <map>

class OpenGLRenderModule : public IRenderModule {
private:
    SDL_Renderer* renderer;
    SDL_GLContext glContext;
    std::map<std::string, SDL_Texture*> textures;
    RenderContext context;

public:
    OpenGLRenderModule();
    ~OpenGLRenderModule() override;

    bool init(RenderContext& context) override;
    void render(const std::vector<DisplayImage>& images) override;
    void cleanup() override;
    void loadImage(const std::string& imageName, SDL_Surface* surface) override;
    void renderText(const std::string& textKey, SDL_Surface* surface, int x, int y, int w, int h) override;
};

#endif // OPENGL_RENDER_MODULE_H