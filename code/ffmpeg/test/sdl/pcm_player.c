#include <SDL.h>
#include <stdio.h>

#define ElementType uint8_t
enum {
    CQ_OK = 0,
    CQ_ERROR,
};

typedef struct {
    int front;      // Records the queue header element position
    int rear;       // Records the end of the queue element position
    int size;       // The number of data elements to store
    int len;
    ElementType *data;
} Queue;

#define BLOCK_SIZE 4096000
static uint8_t audio_buf[BLOCK_SIZE];
static Queue* audio_queue = NULL;

Queue *queue_init(ElementType *buf, uint32_t size)
{
    Queue* q = (Queue *)malloc(sizeof(Queue));
    if (!q) {
        printf("Failed to malloc queue\n");
        return NULL;
    }

    q->size = size;
    q->data = buf;
    q->len = 0;
    q->front = q->rear = 0;

    return q;
}

int is_empty_queue(Queue *q)
{
    return (q->len == 0);
}

int is_full_queue(Queue *q)
{
    return (q->len == q->size);
}

int get_queue_size(Queue* q)
{
    return q->len;
}

int queue_push(Queue *q, ElementType *data, uint32_t len)
{
    uint32_t bytes_to_end;

    if (q->size - q->len < len) {
        return CQ_ERROR;
    }

    if (!data) {
        return CQ_ERROR;
    }

    q->len += len;
    bytes_to_end = q->size - q->rear;

    if (bytes_to_end > len) {
        memcpy((uint8_t *)&q->data[q->rear], (uint8_t *)data, len);
        q->rear += len;
    } else {
        memcpy((uint8_t *)&q->data[q->rear], (uint8_t *)data, bytes_to_end);
        memcpy((uint8_t *)&q->data[0], ((uint8_t *)data) + bytes_to_end, len - bytes_to_end);
        q->rear = len - bytes_to_end;
    }

    return CQ_OK;
}

int queue_pop(Queue *q, ElementType *data, uint32_t len)
{
    uint32_t bytes_to_end;

    if (q->len < len) {
        return CQ_ERROR;
    }

    q->len -= len;
    if (data != NULL) {
        bytes_to_end = q->size - q->front;
        if (bytes_to_end > len) {
            memcpy((uint8_t *)data, (uint8_t *)&q->data[q->front], len);
            q->front += len;
        } else {
            memcpy((uint8_t *)data, (uint8_t *)&q->data[q->front], bytes_to_end);
            memcpy(((uint8_t *)data + bytes_to_end), (uint8_t *)&q->data[0], len - bytes_to_end);
            q->front = len - bytes_to_end;
        }
    } else {
        if (q->size > 0) {
            q->front = (q->front + len) % q->size;
        } else {
            q->front = 0;
        }
    }
}

void queue_free()
{

}

//callback function for audio devcie
void read_audio_data(void* udata, uint8_t* stream, int len) 
{
    uint8_t buf[4096];
    SDL_mutex* lock;
    
    if (is_empty_queue(audio_queue)) {
        return;
    }

    SDL_memset(stream, 0, len);

    len = (len < get_queue_size(audio_queue)) ? len : get_queue_size(audio_queue);
    printf("len=%d\n", len);

    //SDL_mutexP(lock);
    queue_pop(audio_queue, buf, len);
    //SDL_mutexV(lock);

    SDL_MixAudio(stream, buf, len, SDL_MIX_MAXVOLUME);
}

int main(int argc, char *argv[])
{
	int ret = -1, status;
    FILE* audio_fd = NULL;
    uint8_t* src_patch = NULL;
    SDL_AudioSpec spec;
    uint8_t buf[4096];
    uint32_t len;
    SDL_mutex* lock;

    if (argc < 2) {
        printf("The count of params should be more than two!");
    }

    src_patch = argv[1];

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		SDL_Log("Failet to initailize SDL - %s\n", SDL_GetError());
		return ret;
	}
 
    audio_fd = fopen(src_patch, "rb");
    if (!audio_fd) {
        fprintf(stderr, "Failed to open pcm file!\n");
        goto __FAIL;
    }

    audio_queue = queue_init(audio_buf, BLOCK_SIZE);
    if (!audio_queue) {
        fprintf(stderr, "Failed to init audio_queue!\n");
        goto __FAIL;
    }

    spec.freq = 48000;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.silence = 0;
    spec.samples = 1024;
    spec.callback = read_audio_data;
    spec.userdata = NULL;

    if (SDL_OpenAudio(&spec, NULL)) {
        fprintf(stderr, "Failed to open audio device, %s\n", SDL_GetError());
        goto __FAIL;
    }

    SDL_PauseAudio(0);

    do {
        len = fread(buf, 1, 4096, audio_fd);
        fprintf(stderr, "read size is %zu\n", len);
        printf("read size is %zu\n", len);
        fflush(stdout);
        while (1) {
            //SDL_mutexP(lock);
            status = queue_push(audio_queue, buf, len);
            //SDL_mutexV(lock);
            if (status == CQ_OK)
                break;

            SDL_Delay(1);
        }
    } while (len != 0);

    while (1) {

        if (is_empty_queue(audio_queue)) {
            SDL_CloseAudio();
            break;
        }
        SDL_Delay(10);
    }

__FAIL:
    if (audio_fd) {
        fclose(audio_fd);
    }

    if (audio_queue) {
        queue_free();
    }
    fflush(stdout);
    fflush(stderr);

	SDL_Quit();
	return 0;
}

