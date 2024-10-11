#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cbuf.h"

/** Header structure placed in front of every blob */
typedef struct
{
#if defined(CBUF_ALLOW_PARTIAL)
    uint32_t len  : 31;     //Length of the blob
    uint32_t open : 1;      //Set to 1 if this blob is currently open for sequential writes
#else
    uint32_t len;           //Length of the blob
#endif
    uint8_t data[];
} cbuf_item_t;

#if defined(CBUF_ALLOW_PARTIAL)
#define CBUF_OPEN_FLAG  0x80000000u
#endif

/** Generalized write helper. Writes to the circular buffer at given index, and
 *  increments the index.
 */
static void _generic_write(cbuf_t *cbuf, const void *src, uint32_t len, uint32_t *at)
{
    uint32_t wb = 0;             //Bytes read
    uint8_t *s = (uint8_t*)src;  //Source pointer
    while (wb < len)
    {
        cbuf->buf[*at] = *s;
        s++;
        *at = (*at + 1) % cbuf->len;
        wb++;
    }
}

/** Generalized read helper. Reads to the circular buffer at given index, and
 *  increments the index.
 */
static void _generic_read(cbuf_t *cbuf, void* dst, uint32_t len, uint32_t *at)
{
    uint32_t rb = 0;             //Bytes read
    uint8_t *d = (uint8_t*)dst;  //Destination pointer
    while (rb < len)
    {
        if (d)
        {
            *d = cbuf->buf[*at];
            d++;
        }
        *at = (*at + 1) % cbuf->len;
        rb++;
    }
}

/** Write data from src to the cbuf, accounting for wrap. This overwrites any data there
 *  so we have to check in advance that the room exists for the data
 *
 * Results in the write index being updated after the write
 */
static void _write(cbuf_t *cbuf, const void *src, uint32_t len)
{
    _generic_write(cbuf, src, len, &cbuf->widx);
}

/** Read data from the cbuf to dst, account for wrap.
 *  Dst should be large enough to hold len bytes.
 *
 *  Results in the read index being updated after the read
 */
static void _read(cbuf_t *cbuf, void* dst, uint32_t len)
{
    _generic_read(cbuf, dst, len, &cbuf->ridx);
}

/** Peek at data from the cbuf at a specific index, accounting for wrap
 *
 *  Does NOT update the read index.
 */
static void _peek_at(cbuf_t *cbuf, void* dst, uint32_t len, uint32_t at)
{
    uint32_t pidx = at;
    _generic_read(cbuf, dst, len, &pidx);
}

/** Peek at data from the cbuf to dst, account for wrap
 *  Begins read at the current read index
 *
 *  Does NOT update the read index.
 */
static void _peek(cbuf_t *cbuf, void* dst, uint32_t len)
{
    _peek_at(cbuf, dst, len, cbuf->ridx);
}


#if defined(CBUF_ALLOW_PARTIAL)
/** Overwrite data in the cbuf at a specific index, accounting for wrap
 *
 *  Does NOT update the write index.
 */
static void _poke_at(cbuf_t *cbuf, const void* src, uint32_t len, uint32_t at)
{
    uint32_t pidx = at;
    _generic_write(cbuf, src, len, &pidx);
}
#endif

void cbuf_init(cbuf_t *cbuf, uint8_t *mem, uint32_t len)
{
    assert(cbuf != NULL);

    cbuf->ridx  = 0;
    cbuf->widx  = 0;
    cbuf->count = 0;
    cbuf->len   = len;
    cbuf->buf   = mem;
#if defined(CBUF_ALLOW_PARTIAL)
    cbuf->open  = false;
    cbuf->hidx  = 0;
#endif
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
                //Dump the next read message off the queue and repeat the loop until there's no overwrite
                if (cbuf_read(cbuf, NULL))
                {
                    overwrite++;
                }
                else
                {
                    //If tried to read and failed it's probably because a partial write has exceeded the buffer
                    //size and the code won't overwrite the same open buffer being written
                    return false;
                }
            }
            else
            {
                //Not allowed to overwrite
                return false;
            }
        }
    } while (would_overwrite);

#if defined(CBUF_ALLOW_PARTIAL)
    //If we're open, update the length in the existing header
    if (cbuf->open)
    {
        cbuf_item_t item;
        _peek_at(cbuf, &item, sizeof(item), cbuf->hidx);
        item.len += data_len;
        _poke_at(cbuf, &item, sizeof(item), cbuf->hidx);
    }
    else
#endif
    {
        //Write the new header
        cbuf_item_t hdr = {0};
        hdr.len = data_len;
        _write(cbuf, &hdr, sizeof(cbuf_item_t));
    }

    //Write the body
    _write(cbuf, data, data_len);

#if defined(CBUF_ALLOW_PARTIAL)
    //Don't increment the count if we're open already
    if (!cbuf->open)
#endif
    {
        //Increment the count
        cbuf->count++;
    }

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
    cbuf_item_t item;

    //Check if empty
    if ((count == 0) || (cbuf->ridx == cbuf->widx))
    {
        return count;
    }

    //Copy out the header
    _read(cbuf, &item, sizeof(cbuf_item_t));

    //Copy the output if there's a destination
    //read() handles a NULL data internally
    _read(cbuf, data, item.len);

    //Decrement the count
    cbuf->count--;

    //Return the count of messages we had _before_ the read
    return count;
}

uint32_t cbuf_peek(cbuf_t *cbuf, void *data, uint32_t *len)
{
    assert(cbuf != NULL);
    assert(data != NULL);
    assert(len  != NULL);
    uint32_t count = cbuf->count;

    //Check if empty
    if ((count == 0) || (cbuf->ridx == cbuf->widx))
    {
        return count;
    }

    //Peek at the header
    cbuf_item_t item;
    _peek(cbuf, &item, sizeof(cbuf_item_t));

    //Peek the requested data into the output buffer
    uint32_t rlen = (*len < item.len) ? *len : item.len;
    uint32_t pidx = (cbuf->ridx + sizeof(cbuf_item_t)) % cbuf->len;
    _peek_at(cbuf, data, rlen, pidx);

    *len = rlen;
    return count;
}

uint32_t cbuf_peek_len(cbuf_t *cbuf, uint32_t *len)
{
    assert(cbuf != NULL);
    uint32_t count = cbuf->count;

    //Check if empty
    if ((count == 0) || (cbuf->ridx == cbuf->widx))
    {
        return count;
    }

    //Peek at the header
    cbuf_item_t item;
    _peek(cbuf, &item, sizeof(cbuf_item_t));

    if (len)
    {
        *len = item.len;
    }

    //Return the count of messages we have now
    return count;
}

uint32_t cbuf_count(cbuf_t *cbuf)
{
    return cbuf->count;
}

#if defined(CBUF_ALLOW_PARTIAL)
bool cbuf_open(cbuf_t *cbuf, bool allow_overwrite, uint32_t *count_overwrite)
{
    bool res = false;
    if (!cbuf->open)
    {
        cbuf->hidx = cbuf->widx;

        //Write the header
        bool res = cbuf_write(cbuf, NULL, 0, allow_overwrite, count_overwrite);
        if (res)
        {
            //Write succeeded. Mark the new header as "open"
            cbuf_item_t item = {.open = 1, .len = 0};
            _poke_at(cbuf, &item, sizeof(item), cbuf->hidx);

            //Decrement the count (cbuf_write increases it)
            //TODO: better way to do this
            cbuf->count--;

            //Now that the header is written, mark the blob as open
            cbuf->open = true;
        }
        else
        {
            //Failed to write (likely because an overwrite would be required; close and return failure)
            cbuf->open = false;
        }
    }

    return res;
}

bool cbuf_close(cbuf_t *cbuf)
{
    if (cbuf->open)
    {
        //Clear the open flag from the header
        cbuf_item_t item;
        _peek_at(cbuf, &item, sizeof(item), cbuf->hidx);
        item.open = 0;
        _poke_at(cbuf, &item, sizeof(item), cbuf->hidx);
        //Increment the count on the buffer and indicate closed
        cbuf->count++;
        cbuf->open = false;
        return true;
    }
    else
    {
        return false;
    }
}
#endif /* defined(CBUF_ALLOW_PARTIAL) */

#if defined(CBUF_TEST)
void cbuf_viz(cbuf_t *cbuf)
{
    assert(cbuf != NULL);
    const uint32_t W = 120; //Width of characters to display
    uint32_t widx = W * cbuf->widx / cbuf->len;
    uint32_t ridx = W * cbuf->ridx / cbuf->len;
    char v[W + 1];

    //Draw the Write pointer
    //printf("%*s%c%*s\n", widx, "", 'W', (cbuf->len - widx - 1), "");
    printf("%*s%c (%d)\n", widx, "", 'W', cbuf->widx);

    //Draw a horizontal line
    memset(v, '-', W);
    v[W] = '\0';

    //Draw a represenation of the buffer contents on the horizontal line
    //Make a copy of the cbuf struct so we can read it non-destructively
    cbuf_t copy = *cbuf;
    while (1)
    {
        if (copy.ridx == copy.widx) break;

        //read out the data to get the start and end indices
        cbuf_item_t item;
        uint32_t item_idx = W * copy.ridx / copy.len;
        _read(&copy, &item, sizeof(cbuf_item_t));

#if defined(CBUF_ALLOW_PARTIAL)
        if (item.open)
        {
            //Item is open and being written
            uint32_t end_idx = W * copy.widx / copy.len;

            //Draw the data on the line
            for (uint32_t idx=item_idx; idx != end_idx; idx = (idx + 1) % W)
            {
                v[idx] = '*';
            }
            v[item_idx] = '|';

            //Done reading if we're at the open blob
            break;
        }
        else
#endif
        {
            _read(&copy, NULL, item.len);
            uint32_t end_idx = W * copy.ridx / copy.len;

            //Draw the data on the line
            for (uint32_t idx=item_idx; idx != end_idx; idx = (idx + 1) % W)
            {
                v[idx] = '=';
            }
            v[item_idx] = '|';
        }

        if (copy.ridx == cbuf->ridx) break;
    }
    printf("%s\n", v);

    //Draw the read pointer
    printf("%*s%c (%d)\n", ridx, "", 'R', cbuf->ridx);
}
#endif
