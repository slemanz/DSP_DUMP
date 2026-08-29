#ifndef INC_FIFO_H_
#define INC_FIFO_H_

#include <stdint.h>

#define FIFO_OK             1U
#define FIFO_FULL           0U
#define FIFO_EMPTY          0U

/*
 * A ring buffer written the way the lesson writes it: two pointers walking
 * round an array, full when advancing the write pointer would land on the read
 * pointer.
 *
 * That test is what makes the structure work without a separate count, and it
 * costs one slot. An array of FIFO_SIZE holds FIFO_SIZE - 1 items, because the
 * state where the two pointers are equal has already been spent on meaning
 * empty and cannot also mean full.
 *
 * Losing that one slot is fine. Not knowing about it is not: ask for FIFO_SIZE
 * items and the last one never arrives, and what it leaves behind is whatever
 * was in the destination already.
 */
#define FIFO_SIZE           64U

typedef uint16_t fifo_item_t;

void    fifo_init(void);
uint8_t fifo_put(fifo_item_t item);
uint8_t fifo_get(fifo_item_t *pItem);
uint32_t fifo_count(void);
uint32_t fifo_capacity(void);

#endif /* INC_FIFO_H_ */
