#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "cbuf.h"

#define MESSAGE_Q_LEN (128)

static cbuf_t cbuf;
static uint8_t mbuf[MESSAGE_Q_LEN];

static void init(void)
{
    cbuf_init(&cbuf, mbuf, MESSAGE_Q_LEN);
}

//Write a string that ends in a NULL
void write_blob(const char* msg)
{
    uint32_t count_overwrite = 0;
    if (cbuf_write(&cbuf, msg, strlen(msg) + 1, true, &count_overwrite))
    {
        printf("Enqueued a message of %lu bytes (overwrote %d)\n", strlen(msg) + 1, count_overwrite);
    }
    else
    {
        printf("Failed to enqueue a message of %lu bytes (overwrote %d)\n", strlen(msg) + 1, count_overwrite);
    }

    cbuf_viz(&cbuf); printf("\n");
}

//Write a string that doesn't end in a NULL
void write_partial(const char* msg)
{
    uint32_t count_overwrite = 0;
    if (cbuf_write(&cbuf, msg, strlen(msg), true, &count_overwrite))
    {
        printf("Enqueued a message of %lu bytes (overwrote %d)\n", strlen(msg), count_overwrite);
    }
    else
    {
        printf("Failed to enqueue a message of %lu bytes (overwrote %d)\n", strlen(msg), count_overwrite);
    }

    cbuf_viz(&cbuf); printf("\n");
}

void open_blob()
{
    uint32_t count_overwrite = 0;
    cbuf_open(&cbuf, true, &count_overwrite);

    printf("Opened a new message (overwrote %d)\n", count_overwrite);
    cbuf_viz(&cbuf); printf("\n");
}

void close_blob()
{
    cbuf_close(&cbuf);
    printf("Closed\n");
    cbuf_viz(&cbuf); printf("\n");
}

void read_one(void)
{
    char msg[MESSAGE_Q_LEN];
    uint32_t len;
    if (cbuf_peek_len(&cbuf, &len) > 0)
    {
        printf("Length to read: %d\n", len);
        cbuf_read(&cbuf, msg);
        printf("%s\n", msg);
        cbuf_viz(&cbuf); printf("\n");
    }
    else
    {
        printf("Nothing to read\n");
    }
}

void read_all(void)
{
    char msg[MESSAGE_Q_LEN];
    uint32_t len;
    while(cbuf_peek_len(&cbuf, &len))
    {
        printf("Length to read: %d\n", len);
        cbuf_read(&cbuf, msg);
        printf("%s\n", msg);
        cbuf_viz(&cbuf); printf("\n");
    }
}

#define PAD "[..................]" //20 charatacters of padding

int main(void)
{
    printf("Message queue is %lu bytes\n", sizeof(mbuf));
    init();

    // write_blob("bytes 0" PAD);
    // write_blob("bytes 1 lorem ipsum dolor sit amet" PAD);
    // write_blob("bytes 2" PAD);
    // write_blob("bytes 3 but also something longer" PAD);
    // write_blob("bytes 4" PAD);
    // write_blob("bytes 6" PAD);
    // write_blob("bytes 7 and some stuff" PAD);
    // write_blob("bytes 8 but why" PAD);
    // write_blob("bytes 9" PAD);

    write_blob("Buffer Fill 1" PAD);
    write_blob("Buffer Fill 2" PAD);
    write_blob("Buffer Fill 3" PAD);

    open_blob();
    write_partial(PAD);
    write_partial(PAD);
    write_partial("hello ");
    write_blob("world!");
    write_partial(PAD);
    write_partial(PAD);
    write_partial(PAD);
    write_partial(PAD);
    write_partial(PAD);
    close_blob();

    read_all();

    write_blob("Buffer Fill 1" PAD);
    write_blob("Buffer Fill 2" PAD);
    write_blob("Buffer Fill 3" PAD);
    open_blob();
    write_partial(PAD);
    read_one();
    write_partial(PAD);
    read_one();
    write_partial(PAD);
    read_one();
    write_blob(PAD);
    read_one();
    close_blob();
    read_all();

}
