#include <stdio.h>
#include <string.h>
#include <SDL.h>

#define BLOCK_SIZE 4096000

#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define QUIT_EVENT  (SDL_USEREVENT + 2)

const uint32_t video_width = 1280;
const uint32_t video_height = 720;
const uint32_t yuv_frame_len = video_width * video_height * 3 / 2;
uint8_t video_buf[BLOCK_SIZE];

int thread_exit = 0;

int refresh_video_handler(void *udata){

    thread_exit=0;

    while (!thread_exit) {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }

    thread_exit = 0;

    //push quit event
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent(&event);

    return 0;
}

void refresh_video(SDL_Texture * texture, SDL_Renderer *renderer, FILE *video_fd, int w_width, int w_height)
{
    SDL_Rect rect;
    static uint32_t remain_data_len;
    uint32_t read_len;
    static uint8_t *video_pos = video_buf;
    static uint8_t *video_end = video_buf;
    static uint32_t remain_buf_len = BLOCK_SIZE;

    if ((video_pos + yuv_frame_len) > video_end) {
        remain_data_len = video_end - video_pos;

        if (remain_data_len && !remain_buf_len) {
            memcpy(video_buf, video_pos, remain_data_len);

            remain_buf_len = BLOCK_SIZE - remain_data_len;
            video_pos = video_buf;
            video_end = video_buf + remain_data_len;
        }
  

        if(video_end == (video_buf + BLOCK_SIZE)) {
            video_pos = video_buf;
            video_end = video_buf;
            remain_buf_len = BLOCK_SIZE;
        }

        if((read_len = fread(video_end, 1, remain_buf_len, video_fd)) <= 0) {
            fprintf(stderr, "Failed to read data from yuv file err:%d!\n", read_len);
            thread_exit = 1;
            goto __EXIT;
        }

        video_end += read_len;
        remain_buf_len -= read_len;
    }

    fprintf(stderr, "not enought data: pos:%p, video_end:%p, remain_buf_len:%d\n", video_pos, video_end, remain_buf_len);

    SDL_UpdateTexture(texture, NULL, video_pos, video_width);

    //FIX: If window is resize
    rect.x = 0;
    rect.y = 0;
    rect.w = w_width;
    rect.h = w_height;

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_RenderPresent(renderer);

    video_pos += yuv_frame_len;

__EXIT:

    return;
}

int main(int argc, char *argv[])
{
	char *src;
	FILE *video_fd = NULL;
	SDL_Window *win = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Texture *texture = NULL;
	SDL_Thread *thread_id = NULL;
	SDL_Event event;
	SDL_Rect rect;

	int w_width = 1280, w_height = 720;

	if (argc < 1) {
		fprintf(stderr, "The count of params should be more than one!\n");
		return -1;
	}

	src = argv[1];

	if (!src) {
		fprintf(stderr, "src in null!\n");
		return -1;
	}

	video_fd = fopen(src, "rb");
	if (!video_fd) {
		fprintf(stderr, "Failed to open yuv file!\n");
		return -2;
	}

	if (SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		goto __FAIL;
	}

	win = SDL_CreateWindow("YUV Player",
						   SDL_WINDOWPOS_UNDEFINED,
						   SDL_WINDOWPOS_UNDEFINED,
						   w_width, w_height,
						   SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (!win) {
		fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
		goto __FAIL;
	}

	renderer = SDL_CreateRenderer(win, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Failed to create render: %s\n", SDL_GetError());
		goto __FAIL;
	}

	texture = SDL_CreateTexture(renderer, 
								SDL_PIXELFORMAT_IYUV,
								SDL_TEXTUREACCESS_STREAMING,
								video_width,
								video_height);
	if (!texture) {
		fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
		goto __FAIL;
	}

	thread_id = SDL_CreateThread(refresh_video_handler,
								 NULL,
								 NULL);

	do {
		SDL_WaitEvent(&event);
		switch(event.type) {
		case REFRESH_EVENT:
            refresh_video(texture, renderer, video_fd, w_width, w_height);
			break;
		case SDL_WINDOWEVENT:
			SDL_GetWindowSize(win, &w_width, &w_height);
			break;
		case SDL_QUIT:
            thread_exit = 1;
			break;
		case QUIT_EVENT:
			break;
		}

        if (thread_exit) {
            break;
        }
	} while(1);

__FAIL:
	if (video_fd) {
		fclose(video_fd);
	}

	if (win) {
		SDL_DestroyWindow(win);
	}

	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	if (texture) {
		SDL_DestroyTexture(texture);
	}
}

