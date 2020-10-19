#include <stdio.h>

#include "neogui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
//#include <SDL2/SDL_opengl.h>
//#include <GL/gl.h>


static SDL_Renderer *renderer;
static TTF_Font *font;

void set_fill_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void fill_rect(float x, float y, float w, float h) {
    SDL_RenderFillRect(renderer, &(SDL_Rect){ (int)x, (int)y, (int)w, (int)h });
}


int main(int argc, char *argv[]) {
    SDL_Window *win = SDL_CreateWindow("NeoGui Test", 0, 0, 1024, 768, SDL_WINDOW_RESIZABLE); // SDL_WINDOW_OPENGL
    assert(win);

    if (TTF_Init() < 0) {
        printf("TTF_Init() error\n");
        return 1;    
    }
    
    font = TTF_OpenFont("data/OpenSans-Regular.ttf", 25);
    assert(font);

    {
        int w, h;
        if (TTF_SizeText(font, "Test", &w, &h) == 0) {
            printf("%d x %d\n", w, h);
        }
    }

    //SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
    //assert(gl_ctx);

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    bool fullscreen = false;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case 'f':
                    fullscreen = !fullscreen;
                    if (fullscreen) {
                        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    } else {
                        SDL_SetWindowFullscreen(win, 0);
                    }
                    break;
                default:
                    break;
                }
                break;
            case SDL_QUIT:
                running = false;
                break;
            }
        }

        int win_width, win_height;
        SDL_GetWindowSize(win, &win_width, &win_height);
        
        //glViewport(0, 0, win_width, win_height);
        //glClearColor(1.f, 0.f, 1.f, 0.f);
        //glClear(GL_COLOR_BUFFER_BIT);

        SDL_RenderClear(renderer);
        test_ui((float)win_width, (float)win_height);
        SDL_RenderPresent(renderer);

        //SDL_GL_SwapWindow(win);
    }
	
    TTF_CloseFont(font);
    TTF_Quit();

    return 0;
}