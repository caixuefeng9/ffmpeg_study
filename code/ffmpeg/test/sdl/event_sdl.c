#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
	SDL_Window *window = NULL;
	SDL_Renderer *render = NULL;
	bool quit = true;
	SDL_Event event;
	SDL_Texture *target = NULL;
	SDL_Rect rect;
	
	rect.w = 30;
	rect.h = 30;

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

	//SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
	//SDL_RenderClear(render);

	//SDL_RenderPresent(render);

	target = SDL_CreateTexture(render, 
							SDL_PIXELFORMAT_RGBA8888,
							SDL_TEXTUREACCESS_TARGET,
							1280,
							720);
	if (!target) {
		SDL_Log("Failed to Create Texture!\n");
		goto __DRENDER;
	}
	
	do {
		SDL_WaitEvent(&event);

		switch (event.type) {
		case SDL_QUIT:
			quit = false;
			break;
		default:
			SDL_Log("event type is %d", event.type);
			break;
		}
		
		rect.x = rand() % 1250;
		rect.y = rand() % 690;

		SDL_SetRenderTarget(render, target);
		SDL_SetRenderDrawColor(render, 0, 0, 0, 0);
		SDL_RenderClear(render);

		SDL_RenderDrawRect(render, &rect);
		SDL_SetRenderDrawColor(render, 255, 0, 0, 0);
		SDL_RenderFillRect(render, &rect);

		SDL_SetRenderTarget(render, NULL);
		SDL_RenderCopy(render, target, NULL, NULL);

		SDL_RenderPresent(render);

	} while(quit);

	//SDL_Delay(3000);
	SDL_DestroyTexture(target);
__DRENDER:
	SDL_DestroyRenderer(render);

__DWINDOW:
	SDL_DestroyWindow(window);

__EXIT:
	SDL_Quit();

	return 0;
}
