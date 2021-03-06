#ifndef CIRBUF_H
#define CIRBUF_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct {
    unsigned long int size;
    unsigned int head, tail;
    void *data;
} cirbuf_t;

#ifndef MAP_ANONYMOUS /* macOS specific */
#define MAP_ANONYMOUS MAP_ANON
#endif

static void create_buffer_mirror(cirbuf_t *cb)
{
    char path[] = "/tmp/cirbuf-XXXXXX";
    int fd = mkstemp(path), rt;
    if (fd == -1) {
        perror("Failed to mkstemp");
        goto mkstemp_fail;
    }

    rt = unlink(path);
    if (rt) {
        perror("Failed to unlink");
        goto unlink_fail;
    }

    rt = ftruncate(fd, cb->size);
    if (rt) {
        perror("Failed to ftruncate");
        goto ftruncate_fail;
    }

    /* create the array of data */
    cb->data = mmap(NULL, cb->size << 1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE,
                    -1, 0);
    if (cb->data == MAP_FAILED) {
        perror("Failed to mmap (create)");
        goto mmap_fail_ct; // mmap create
    }

    void *address = mmap(cb->data, cb->size, PROT_READ | PROT_WRITE,
                         MAP_FIXED | MAP_SHARED, fd, 0);
    if (address != cb->data) {
        perror("Failed to mmap (main, addr != cb->data)");
        goto mmap_fail_main;
    }

    address = mmap(cb->data + cb->size, cb->size, PROT_READ | PROT_WRITE,
                   MAP_FIXED | MAP_SHARED, fd, 0);
    if (address != cb->data + cb->size) {
        perror("Failed to mmap (mirror)");
        goto mmap_fail_mirror;
    }

    close(fd);
    return; // success

mmap_fail_mirror:
mmap_fail_main:
    munmap(cb->data, cb->size << 1);

mmap_fail_ct:
ftruncate_fail:    
unlink_fail:
    close(fd);

mkstemp_fail:
    exit(-1);
}

/** Create new circular buffer.
 * @param size Size of the circular buffer.
 * @return pointer to new circular buffer
 */
static inline void cirbuf_new(cirbuf_t *dst, const unsigned long int size)
{
    dst->size = size;
    dst->head = dst->tail = 0;
    create_buffer_mirror(dst);
}

/** Free memory used by circular buffer
 * @param cb The circular buffer. */
static inline void cirbuf_free(cirbuf_t *cb)
{
    munmap(cb->data, cb->size << 1);
    free(cb);
}

static inline int cirbuf_unusedspace(const cirbuf_t *cb);

/** Write data to the tail of the circular buffer.
 * Increases the position of the tail.
 * This copies data to the circular buffer using memcpy.
 * After calling you should free the data if you don't need it anymore.
 *
 * @param cb The circular buffer.
 * @param data Buffer to be written to the circular buffer.
 * @param size Size of the buffer to be added to the circular buffer in bytes.
 * @return number of bytes offered
 */
static inline int cirbuf_offer(cirbuf_t *cb,
                               const unsigned char *data,
                               const int size)
{
    /* prevent buffer from getting completely full or over commited */
    if (cirbuf_unusedspace(cb) <= size)
        return 0;

    memcpy(cb->data + cb->tail, data, size);
    cb->tail += size;
    
    if (cb->tail >= cb->size)
        cb->tail %= cb->size;

    return size;
}

/** Tell if the circular buffer is empty.
 * @param cb The circular buffer.
 * @return true if empty; otherwise false
 */
static inline bool cirbuf_is_empty(const cirbuf_t *cb)
{
    return cb->head == cb->tail;
}

/** Look at data at the circular buffer's head.
 * Use cirbuf_usedspace to determine how much data in bytes can be read.
 * @param cb The circular buffer.
 * @return pointer to the head of the circular buffer
 */
static inline unsigned char *cirbuf_peek(const cirbuf_t *cb)
{
    if (cirbuf_is_empty(cb))
        return NULL;

    return cb->data + cb->head;
}

/** Release data at the head from the circular buffer.
 * Increase the position of the head.
 *
 * WARNING: it would be a dangling call if:
 *  1. You are using the returned data pointer.
 *  2. Another thread has offerred data to the circular buffer.
 *
 * If you want to access the data from the returned pointer, you are better off
 * using cirbuf_peek.
 *
 * @param cb The circular buffer.
 * @param size Number of bytes to release
 * @return pointer to data; NULL if we can not poll this much data
 */
static inline unsigned char *cirbuf_poll(cirbuf_t *cb, const unsigned int size)
{
    if (cirbuf_is_empty(cb))
        return NULL;

    void *end = cb->data + cb->head;
    cb->head += size;
    if (cb->head >= cb->size)
        cb->head -= cb->size;
    return end;
}

/** Size in bytes of the circular buffer.
 * Is equal to 2 ^ order.
 *
 * @param cb The circular buffer.
 * @return size of the circular buffer in bytes
 */
static inline int cirbuf_size(const cirbuf_t *cb)
{
    return cb->size;
}

/** Tell how much data has been written in bytes to the circular buffer.
 * @param cb The circular buffer.
 * @return number of bytes of how data has been written
 */
static inline int cirbuf_usedspace(const cirbuf_t *cb)
{
    if (cb->head <= cb->tail)
        return cb->tail - cb->head;
    return cb->size - (cb->head - cb->tail);
}

/** Tell how much data we can write in bytes to the circular buffer.
 * @param cb The circular buffer.
 * @return number of bytes of how much data can be written
 */
static inline int cirbuf_unusedspace(const cirbuf_t *cb)
{
    return cb->size - cirbuf_usedspace(cb);
}

#endif /* CIRBUF_H */
