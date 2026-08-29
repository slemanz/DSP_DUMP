#include "fifo.h"

static volatile fifo_item_t  store[FIFO_SIZE];
static volatile fifo_item_t *put_at;
static volatile fifo_item_t *get_at;

void fifo_init(void)
{
    put_at = &store[0];
    get_at = &store[0];
}

uint8_t fifo_put(fifo_item_t item)
{
    volatile fifo_item_t *next = put_at + 1;

    if (next == &store[FIFO_SIZE])
    {
        next = &store[0];
    }

    if (next == get_at)
    {
        return FIFO_FULL;
    }

    *put_at = item;
    put_at  = next;

    return FIFO_OK;
}

uint8_t fifo_get(fifo_item_t *pItem)
{
    if (put_at == get_at)
    {
        return FIFO_EMPTY;
    }

    *pItem = *get_at;
    get_at++;

    if (get_at == &store[FIFO_SIZE])
    {
        get_at = &store[0];
    }

    return FIFO_OK;
}

uint32_t fifo_count(void)
{
    if (put_at >= get_at)
    {
        return (uint32_t)(put_at - get_at);
    }

    return (uint32_t)(FIFO_SIZE - (uint32_t)(get_at - put_at));
}

/* what it actually holds, which is not what it was declared as */
uint32_t fifo_capacity(void)
{
    return FIFO_SIZE - 1U;
}
