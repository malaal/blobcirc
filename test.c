#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "cbuf.h"

#define MESSAGE_HEADER_MAX_LEN (32)
#define DEBUG_MESSAGE_MAX_LEN (128)
#define MESSAGE_Q_LEN (256)

// Length of a full message is the header + the user's message + \r\n + \0
static char message_output_buf[MESSAGE_HEADER_MAX_LEN + DEBUG_MESSAGE_MAX_LEN + 3];
//#define MESSAGE_Q_LEN (sizeof(message_output_buf) * 16)

typedef enum {
    DEBUG_LEVEL_NONE,
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARNING,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG,
} debug_level_t;

typedef struct {
    debug_level_t level;
    time_t timestamp;
    float fracsec;
    char message[];
} debug_header_t;

typedef struct {
    debug_header_t hdr;
    char message[DEBUG_MESSAGE_MAX_LEN];
} debug_message_t;

static cbuf_t cbuf;
static uint8_t mbuf[MESSAGE_Q_LEN];

static void init(void)
{
    cbuf_init(&cbuf, mbuf, MESSAGE_Q_LEN);
}

void _debug_message(debug_level_t level, const char *fmt, ...) {
    va_list args;
    debug_message_t msg;

    //Print the message into the debug queue
    va_start(args, fmt);
    snprintf(msg.message, DEBUG_MESSAGE_MAX_LEN, fmt, args) > 0;
    va_end(args);

    msg.hdr.level = level;
    msg.hdr.timestamp = 0;
    msg.hdr.fracsec = 0;

    uint32_t count_overwrite = 0;
    cbuf_write(&cbuf, &msg, sizeof(debug_header_t) + strlen(msg.message) + 1, true, &count_overwrite);

    printf("Enqueued a message of %lu bytes (overwrote %d)\n", sizeof(debug_header_t) + strlen(msg.message) + 1, count_overwrite);
    cbuf_viz(&cbuf);
}

int main(void)
{
    printf("Message queue is %lu bytes\n", sizeof(mbuf));
    init();

    _debug_message(0, "bytes 0");
    _debug_message(0, "bytes 1 lorem ipsum dolor sit amet");
    _debug_message(0, "bytes 2");
    _debug_message(0, "bytes 3 but also something longer");
    _debug_message(0, "bytes 4");
    _debug_message(0, "bytes 6");
    _debug_message(0, "bytes 7 and some stuff");
    _debug_message(0, "bytes 8 but why");
    _debug_message(0, "bytes 9");
    // _debug_message(0, "bytes");
    // _debug_message(0, "bytes");
    // _debug_message(0, "bytes");

    debug_message_t msg;
    while (cbuf_read(&cbuf, &msg))
    {
        printf("%s\n", msg.message);
        cbuf_viz(&cbuf);
    }
}
