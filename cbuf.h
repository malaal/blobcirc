#ifndef __CBUF_H__
#define __CBUF_H__

#include <stdint.h>
#include <stdbool.h>

/** Here we define a circular buffer into which data of arbitrary length can be written at once.
 *  If writing that data would cause the buffer to wrap, one or more of the oldest messages are
 *  erased, if allowed.
 *
 *  Data is written and read into the buffer as arbitrary-length items. The whole item must be
 *  written or read at once. Ensure your pointers have sufficient space to do so.
 */

/** Structure that holds the metadata for a circular buffer */
typedef struct {
    uint32_t ridx;      //Read index
    uint32_t widx;      //Write index
    uint32_t count;     //Count of items in the buffer
    uint32_t len;       //Length of buffer (in bytes)
    uint8_t *buf;       //Pointer to buffer memory
} cbuf_t;

/** Intialize a circular buffer struct.
 *    cbuf         pointer to the circular buffer struct
 *    mem          pointer to the memory space that will store the data
 *    len          length of the memory space, in bytes
 */
void cbuf_init(cbuf_t *cbuf, uint8_t *mem, uint32_t len);

/** Write a data blob to the circular buffer
*    cbuf         pointer to the circular buffer struct
*    data         data to write to the buffer
*    data_len     length of the data to write, in bytes
*    allow_overwrite    set true if the write operation can overwrite old data to write new data
*    count_overwrite    returns the number of old messages erased to make room for the new data
* Returns true if the data was written, false otherwise.
*/
bool cbuf_write(cbuf_t *cbuf, const void *data, uint32_t data_len, bool allow_overwrite, uint32_t *count_overwrite);

/** Read a data blob from the circular buffer
*    cbuf         pointer to the circular buffer struct
*    data         data to read from the buffer. You will not be able to get the data length first,
*                 so ensure the buffer is large enough
*                 Can be NULL to remove the next item from the buffer without storing it elsewhere
* Returns true if the data was read, false otherwise (ie: the buffer is empty).
*/
bool cbuf_read(cbuf_t *cbuf, void *data);

/** Print a visualization of the buffer state to the screen. Looks super cool, but really for debug only. */
void cbuf_viz(cbuf_t *cbuf);

//PLACEFULDERS
#define assert(x) x

#endif

