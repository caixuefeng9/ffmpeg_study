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

int main(int argc, char *argv[])
{
	int ret = -1;

	if (SDL_Init(SDL_INIT_AUDIO)) {
		SDL_Log("Failet to initail!");
		return ret;
	}

	SDL_Quit();
	return 0;
}

