#include "opengl.h"
#include <stdexcept>

OpenGLRenderModule::OpenGLRenderModule() : renderer(nullptr), glContext(nullptr) {}

OpenGLRenderModule::~OpenGLRenderModule() {
    cleanup();
}

bool OpenGLRenderModule::init(RenderContext& ctx) {
    context = ctx;

    // Настройка атрибутов OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Создание окна с поддержкой OpenGL
    context.window = SDL_CreateWindow("Visual Novel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!context.window) {
        throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
        return false;
    }

    // Создание рендера
    renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        throw std::runtime_error("Failed to create SDL renderer: " + std::string(SDL_GetError()));
        return false;
    }

    // Создание контекста OpenGL
    glContext = SDL_GL_CreateContext(context.window);
    if (!glContext) {
        throw std::runtime_error("Failed to create OpenGL context: " + std::string(SDL_GetError()));
        return false;
    }

    // Загрузка GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
        return false;
    }

    // Установка области просмотра
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    return true;
}

void OpenGLRenderModule::render(const std::vector<DisplayImage>& images) {
    // Очистка экрана
    SDL_RenderClear(renderer);

    // Отрисовка всех изображений
    for (const auto& img : images) {
        if (textures.find(img.name) != textures.end()) {
            SDL_Rect rect = {img.x, img.y, img.w, img.h};
            SDL_RenderCopy(renderer, textures[img.name], nullptr, &rect);
        }
    }

    // Презентация результата
    SDL_RenderPresent(renderer);
}

void OpenGLRenderModule::cleanup() {
    // Очистка текстур
    for (auto& texture : textures) {
        SDL_DestroyTexture(texture.second);
    }
    textures.clear();

    // Уничтожение контекста и рендера
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (context.window) {
        SDL_DestroyWindow(context.window);
        context.window = nullptr;
    }
}

void OpenGLRenderModule::loadImage(const std::string& imageName, SDL_Surface* surface) {
    if (textures.find(imageName) == textures.end()) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            throw std::runtime_error("Failed to create texture from surface: " + std::string(SDL_GetError()));
        }
        textures[imageName] = texture;
    }
}

void OpenGLRenderModule::renderText(const std::string& textKey, SDL_Surface* surface, int x, int y, int w, int h) {
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!textTexture) {
        throw std::runtime_error("Failed to create text texture: " + std::string(SDL_GetError()));
    }
    textures[textKey] = textTexture;
}