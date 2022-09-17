#include <SDL.h>

int main(int argc, char *argv[])
{
	SDL_Window *window = NULL;
	SDL_Renderer *render = NULL;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("SDL2 Window",
					200,
					200,
					1280,
					720,
					SDL_WINDOW_SHOWN);
	
	if (!window) {
		printf("Failed to Create window!\n");
		goto __EXIT;
	}

	render = SDL_CreateRenderer(window, -1, 0);
	if (!render) {
		SDL_Log("Failed to Create Render!\n");
		goto __DWINDOW;
	}

	SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
	SDL_RenderClear(render);

	SDL_RenderPresent(render);
	SDL_Delay(3000);

__DWINDOW:
	SDL_DestroyWindow(window);

__EXIT:
	SDL_Quit();

	return 0;
}
