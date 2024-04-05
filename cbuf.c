#include <stdio.h>
#include <string.h>

#include "cbuf.h"

typedef struct
{
    uint32_t len;
    uint8_t data[];
} cbuf_item_t;

static void write(cbuf_t *cbuf, const void *src, uint32_t len)
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

static void read(cbuf_t *cbuf, void* dst, uint32_t len)
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
    bool wrap = next_widx > cbuf->len;
    do {
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
                //Dump the next read message off the queue and test again
                cbuf_read(cbuf, NULL);
            }
            else
            {
                //Not allowed to overwrite
                return false;
            }
        }
        // printf("%d -> %d, %d (%d)%s\n", cbuf->widx, next_widx, cbuf->ridx, (cbuf->ridx + cbuf->len),
        //     wrap ? " wrap" : "");
    } while (would_overwrite);


    //Write the header
    cbuf_item_t hdr;
    hdr.len = data_len;
    printf("Insert %d bytes at %d, over: %d\n", hdr.len, cbuf->widx, overwrite);
    write(cbuf, &hdr, sizeof(cbuf_item_t));

    //Write the body
    write(cbuf, data, data_len);

    //output
    if (count_overwrite)
    {
        *count_overwrite = overwrite;
    }

    return true;
}

bool cbuf_read(cbuf_t *cbuf, void *data)
{
    assert(cbuf != NULL);

    //Check if empty
    if (cbuf->ridx == cbuf->widx)
    {
        return false;
    }

    //Copy out the header
    cbuf_item_t item;
    uint32_t sidx = cbuf->ridx;
    read(cbuf, &item, sizeof(cbuf_item_t));
    // printf("R header @%d: %d b\n", cbuf->ridx, hdr.length);

    //Copy the output if there's a destination
    read(cbuf, data, item.len);

    //printf("Read %d bytes at %d\n", item.len, sidx);

    return true;
}

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

#if 0
int main(void)
{
    cbuf_t CB;
    uint8_t mem[100];
    cbuf_init(&CB, mem, 50);

    char data[32] = "data";
    uint32_t ovw = 0;

    printf("\n");
    strcpy(&data[4], "123");
    printf("Write: %s\n", data);
    cbuf_write(&CB, data, strlen(data), true, &ovw);
    cbuf_viz(&CB);

    printf("\n");
    strcpy(&data[4], "145234523452");
    printf("Write: %s\n", data);
    cbuf_write(&CB, data, strlen(data), true, &ovw);
    cbuf_viz(&CB);

    printf("\n");
    strcpy(&data[4], "145");
    printf("Write: %s\n", data);
    cbuf_write(&CB, data, strlen(data), true, &ovw);
    cbuf_viz(&CB);

    printf("\n");
    strcpy(&data[4], "15234523452345");
    printf("Write: %s\n", data);
    cbuf_write(&CB, data, strlen(data), true, &ovw);
    cbuf_viz(&CB);

    printf("\n");
    strcpy(&data[4], "134344344");
    printf("Write: %s\n", data);
    cbuf_write(&CB, data, strlen(data), true, &ovw);
    cbuf_viz(&CB);

    char rdata[32];
    while (cbuf_read(&CB, rdata))
    {
        printf("\n");
        printf("Read: %s\n", rdata);
        cbuf_viz(&CB);
    }
}
#endif
