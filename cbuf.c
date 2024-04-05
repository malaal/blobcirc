#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cbuf.h"

typedef struct
{
    uint32_t len;
    uint8_t data[];
} cbuf_item_t;

/** Write data from src to the cbuf, accounting for wrap. This overwrites any data there
 *  so we have to check in advance that the room exists for the data
 *
 * Results in the write index being updated after the write
 */
static void _write(cbuf_t *cbuf, const void *src, uint32_t len)
{
    uint32_t wb = 0;             //Bytes read
    uint8_t *s = (uint8_t*)src;  //Source pointer
    while (wb < len)
    {
        cbuf->buf[cbuf->widx] = *s;
        s++;
        cbuf->widx = (cbuf->widx + 1) % cbuf->len;
        wb++;
    }
}

/** Read data from the cbuf to dst, account for wrap.
 *  Dst should be large enough to hold len bytes.
 *
 *  Results in the read index being updated after the read
 */
static void _read(cbuf_t *cbuf, void* dst, uint32_t len)
{
    uint32_t rb = 0;             //Bytes read
    uint8_t *d = (uint8_t*)dst;  //Destination pointer
    while (rb < len)
    {
        if (d)
        {
            *d = cbuf->buf[cbuf->ridx];
            d++;
        }
        cbuf->ridx = (cbuf->ridx + 1) % cbuf->len;
        rb++;
    }
}

void cbuf_init(cbuf_t *cbuf, uint8_t *mem, uint32_t len)
{
    assert(cbuf != NULL);

    cbuf->ridx  = 0;
    cbuf->widx  = 0;
    cbuf->count = 0;
    cbuf->len   = len;
    cbuf->buf   = mem;
}

bool cbuf_write(cbuf_t *cbuf, const void *data, uint32_t data_len, bool allow_overwrite, uint32_t *count_overwrite)
{
    assert(cbuf != NULL);

    //Count of data items overwritten during insertion
    uint32_t overwrite = 0;

    //Ensure we're not writing more than is possible
    if ((sizeof(cbuf_item_t) + data_len) >= (cbuf->len - 1))
    {
        return false;
    }

    //Calculate whether writing this much data would cause an overwrite
    bool would_overwrite = false;
    uint32_t next_widx = (cbuf->widx + (sizeof(cbuf_item_t) + data_len)); //We will account for wrap later on
    bool wrap = next_widx > cbuf->len; //True if we'd wrap around the buffer if we added this many to the index
    do {
        //These complicated logic statements are derived from listing every possible case and keeping those that result
        //in no overwrite. I can't figure out a way to make them smaller.
        if (    (!wrap && (cbuf->widx >= cbuf->ridx) && (next_widx > cbuf->ridx)) ||
                (!wrap && (cbuf->widx < cbuf->ridx)  && (next_widx < cbuf->ridx)) ||
                ( wrap && (cbuf->widx >= cbuf->ridx) && (next_widx <= (cbuf->ridx + cbuf->len)))
           )
        {
            would_overwrite = false;
        }
        else
        {
            would_overwrite = true;
            if (allow_overwrite)
            {
                overwrite++;
                //Dump the next read message off the queue and repeat the loop until there's no overwrite
                cbuf_read(cbuf, NULL);
            }
            else
            {
                //Not allowed to overwrite
                return false;
            }
        }
    } while (would_overwrite);

    //Write the header
    cbuf_item_t hdr = {0};
    hdr.len = data_len;
    _write(cbuf, &hdr, sizeof(cbuf_item_t));

    //Write the body
    _write(cbuf, data, data_len);

    //Increment the count
    cbuf->count++;

    //output
    if (count_overwrite)
    {
        *count_overwrite = overwrite;
    }

    return true;
}

uint32_t cbuf_read(cbuf_t *cbuf, void *data)
{
    assert(cbuf != NULL);
    uint32_t count = cbuf->count;

    //Check if empty
    if ((count == 0) || (cbuf->ridx == cbuf->widx))
    {
        return count;
    }

    //Copy out the header
    cbuf_item_t item;
    _read(cbuf, &item, sizeof(cbuf_item_t));

    //Copy the output if there's a destination
    //read() handles a NULL data internally
    _read(cbuf, data, item.len);

    //Decrement the count
    cbuf->count--;

    //Return the count of messages we had _before_ the read
    return count;
}

uint32_t cbuf_count(cbuf_t *cbuf)
{
    return cbuf->count;
}

#if 0
void cbuf_viz(cbuf_t *cbuf)
{
    assert(cbuf != NULL);
    const uint32_t W = 100; //Width of characters to display
    uint32_t widx = W * cbuf->widx / cbuf->len;
    uint32_t ridx = W * cbuf->ridx / cbuf->len;
    char v[W + 1];

    //Draw the Write pointer
    printf("%*s%c%*s\n", widx, "", 'W', (cbuf->len - widx - 1), "");

    //Draw a horizontal line
    memset(v, '-', 100);
    v[W] = '\0';

    //Draw a represenation of the buffer contents
    //Make a copy of the cbuf struct so we can read it non-destructively
    cbuf_t copy = *cbuf;
    while (1)
    {
        if (copy.ridx == copy.widx) break;

        //read out the data to get the start and end indices
        cbuf_item_t item;
        uint32_t item_idx = W * copy.ridx / copy.len;
        read(&copy, &item, sizeof(cbuf_item_t));
        read(&copy, NULL, item.len);
        uint32_t end_idx = W * copy.ridx / copy.len;

        //Draw the data on the line
        for (uint32_t idx=item_idx; idx != end_idx; idx = (idx + 1) % W)
        {
            v[idx] = '=';
        }
        v[item_idx] = '|';

        if (copy.ridx == cbuf->ridx) break;
    }
    printf("%s\n", v);

    //Draw the read pointer
    printf("%*s%c%*s\n", ridx, "", 'R', (cbuf->len - ridx - 1), "");
}
#endif
