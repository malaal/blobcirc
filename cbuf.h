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
 *
 *  To provide thread-safety, the caller of these functions should wrap them in appropriate mutexes.
 */

/** If CBUF_ALLOW_PARTIAL is defined, the cbuf also allows a secondary mode where the buffer can be
 * "opened" for multiple sequential writes that aren't the full buffer size. This would allow using
 * it similar to a standard circular buffer, while still maintaining the arbitrary-length behavior.
 * When "open" reads are still allowed but buffer-writes are blocked. cbuf_read will not read the
 * currently open buffer
 */
//#define CBUF_ALLOW_PARTIAL

/** Structure that holds the metadata for a circular buffer */
typedef struct {
    uint32_t ridx;      //Read index
    uint32_t widx;      //Write index
    uint32_t count;     //Count of items in the buffer
    uint32_t len;       //Length of buffer (in bytes)
    uint8_t *buf;       //Pointer to buffer memory
#if defined(CBUF_ALLOW_PARTIAL)
    uint8_t  open;      //True if the cbuf is open for partial writes
    uint32_t hidx;      //Index to the header of the open item
#endif
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
*    data         data to read from the buffer. Use cbuf_peek_len() first to get the length of this data
*                 Can be NULL to remove the next item from the buffer without storing it elsewhere
* Returns the number of messages on the buffer _before_ the read.
*/
uint32_t cbuf_read(cbuf_t *cbuf, void *data);

/** Get the length of the next data blob to be read from the circular buffer
*    cbuf         pointer to the circular buffer struct
*    len          length of the data
* Returns the number of messages on the buffer.
*/
uint32_t cbuf_peek_len(cbuf_t *cbuf, uint32_t *len);

/** Returns the number of data blobs in the circular buffer */
uint32_t cbuf_count(cbuf_t *cbuf);

#if defined(CBUF_TEST)
/** Print a visualization of the buffer state to the screen. Looks super cool, but really for debug only. */
void cbuf_viz(cbuf_t *cbuf);
#endif

#if defined(CBUF_ALLOW_PARTIAL)
/** Open a blob for multiple sequential writes using cbuf_write.
 *  Returns true if successful
 */
bool cbuf_open(cbuf_t *cbuf, bool allow_overwrite, uint32_t *count_overwrite);

/** Close the currently open buffer and mark is as a complete blob.
 *  Returns true if successful */
bool cbuf_close(cbuf_t *cbuf);
#endif

#endif

