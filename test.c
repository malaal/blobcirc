#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "cbuf.h"

#define MESSAGE_MAX_LEN (64)
#define MESSAGE_Q_LEN (256)

static cbuf_t cbuf;
static uint8_t mbuf[MESSAGE_Q_LEN];

static void init(void)
{
    cbuf_init(&cbuf, mbuf, MESSAGE_Q_LEN);
}

void write_blob(const char* msg)
{
    uint32_t count_overwrite = 0;
    cbuf_write(&cbuf, msg, strlen(msg) + 1, true, &count_overwrite);

    printf("Enqueued a message of %lu bytes (overwrote %d)\n", strlen(msg) + 1, count_overwrite);
    cbuf_viz(&cbuf);
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

    write_blob("bytes 0" PAD);
    write_blob("bytes 1 lorem ipsum dolor sit amet" PAD);
    write_blob("bytes 2" PAD);
    write_blob("bytes 3 but also something longer" PAD);
    write_blob("bytes 4" PAD);
    write_blob("bytes 6" PAD);
    write_blob("bytes 7 and some stuff" PAD);
    write_blob("bytes 8 but why" PAD);
    write_blob("bytes 9" PAD);

    read_all();
}
