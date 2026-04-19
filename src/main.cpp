#include "/code/v64-Cortex/src/cortex.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>  // Standard C IO, not C++ <iostream>
#include <string.h> // Standard C strings, not the C++ <string> STL

#define FONT_PATH "/usr/share/fonts/my-fonts/Fustat/static/Fustat-Regular.ttf"

enum class Action {
    None,
    Write,
    Backspace,
};

struct Buffer {
    Arena allocator{};
    DynArray<char> text{&allocator, 1024 * 16};
    int byteLength;
    Action action = Action::None;
    int pos = 0;

    void backspace() {
        if (text.len == 0) return;

        // Step back one byte
        text.len--;

        // Check for UTF-8 continuation bytes.
        // In UTF-8, continuation bytes always start with binary '10' (0x80 to 0xBF).
        // Bitwise AND with 0xC0 (11000000) checks if those top two bits are '10'.
        // We keep stepping back until we hit the start byte of the character.
        while (text.len > 0 && (text[text.len] & 0xC0) == 0x80) {
            text.len--;
        }

        // Terminate the string at the new end
        text[text.len] = '\0';
    }

    u32 append(const char* s) {
        auto _pos = text.len;
        text.append(s, strlen(s));
        return _pos;
    }

    u16 operator[](u32 index) const { return text[index]; }

    u16 operator[](u32 index) { return text[index]; }
};

struct Editor {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int width;
    int height;
    int pos;
    float line = 0;
    Editor(const char* name, int width, int height) {
        window = SDL_CreateWindow(name, width, height, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, nullptr);
        SDL_SetRenderVSync(renderer, 1);
    }
    ~Editor() {
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
    }
};

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "sdl_error:%s\n", SDL_GetError());
        return 1;
    };
    if (!TTF_Init()) {
        fprintf(stderr, "sdl_error:%s\n", SDL_GetError());
        return 1;
    };

    defer(TTF_Quit());
    defer(SDL_Quit());

    Editor ed("Arabic Text Editor", 800, 600);
    ed.pos = 800;

    TTF_Font* font = TTF_OpenFont(FONT_PATH, 36);
    defer(TTF_CloseFont(font));

    if (!font) {
        printf("Failed to load font!\n");
        return 1;
    }

    // Enable Arabic Shaping and RTL direction
    TTF_SetFontScript(font, TTF_StringToTag("Arab"));
    TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_RIGHT);
    TTF_SetFontDirection(font, TTF_DIRECTION_RTL);

    Buffer displaybuffer{};
    Buffer buffer{};
    SDL_Color textColor = {255, 255, 255, 255}; // White text

    // We only want to recreate the texture when the text actually changes
    bool needsRenderUpdate = true;
    SDL_Texture* textTexture = nullptr;
    defer(SDL_DestroyTexture(textTexture));

    float textWidth = 0;
    float textHeight = 0;

    // Start listening for text input specifically
    if (!SDL_StartTextInput(ed.window)) {
        printf("error: %s", SDL_GetError());
        exit(1);
    }

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        SDL_WaitEvent(&e);
        do {
            switch (e.type) {
            case SDL_EVENT_QUIT: {
                quit = true;
                break;
            }
            case SDL_EVENT_TEXT_INPUT: {
                buffer.append(e.text.text);
                int bytelen = strlen(e.text.text);

                // printf(" = %s ", e.text.text);
                // for (int i = 0; i < bytelen; ++i) {
                //     printf("[0x%x]", e.text.text[i]);
                // }
                // puts("\n");

                needsRenderUpdate = true;
                buffer.action = Action::Write;
            } break;
            case SDL_EVENT_KEY_DOWN: {
                if (e.key.key == SDLK_BACKSPACE) {
                    buffer.backspace();
                    needsRenderUpdate = true;
                } else if ((e.key.mod & SDL_KMOD_SHIFT) && e.key.scancode == SDL_SCANCODE_G) {
                    buffer.append("ل"); // Manually inject the UTF-8 bytes for ﻷ
                    buffer.append("أ");
                    needsRenderUpdate = true;
                    buffer.action = Action::Write;
                } else if (e.key.key == SDLK_KP_ENTER or e.key.key == SDLK_RETURN) {
                    puts("NEWLINE\n");
                    // ed.line += textHeight;
                    buffer.append("\n");
                }

            } break;
            }
        } while (SDL_PollEvent(&e) != 0);
        // 3. Update the texture only if the text changed
        if (needsRenderUpdate) {
            if (textTexture) {
                SDL_DestroyTexture(textTexture); // Clean up the old texture
            }

            SDL_Surface* surface =
                TTF_RenderText_Blended_Wrapped(font, buffer.text.ptr, buffer.text.len, textColor, ed.width);
            if (surface) {
                textTexture = SDL_CreateTextureFromSurface(ed.renderer, surface);
                textWidth = surface->w;
                textHeight = surface->h;
                SDL_DestroySurface(surface);
            }
            needsRenderUpdate = false;
        }

        // 4. Render to Screen
        SDL_SetRenderDrawColor(ed.renderer, 30, 30, 30, 255); // Dark grey background
        SDL_RenderClear(ed.renderer);
        if (textTexture) {
            // Center the text horizontally, place it slightly down from the top
            // int currentWinWidth, currentWinHeight;
            SDL_GetWindowSize(ed.window, &ed.width, &ed.height);
            float r_margin = 20.0f;
            float targetX = (float)ed.width - textWidth - r_margin;

            SDL_FRect renderQuad = {targetX, ed.line, textWidth, textHeight};
            SDL_RenderTexture(ed.renderer, textTexture, nullptr, &renderQuad);
        }

        SDL_RenderPresent(ed.renderer);
    }
    printf("memory = %u\n", buffer.text.capacity);
    return 0;
}
