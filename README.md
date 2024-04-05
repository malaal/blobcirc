# Arbirary-length circular buffer

Most statically-allocated, non-list-based circular buffers operate on items of a fixed size. If we are storing elements of arbirary length, 
this forces us to either write one byte at a time and parse the output stream, or dedicate a known "max size" element and assume all elements
fit in this max. This latter option is a huge waste of space if you're allocating for a potential worst case.

In this implementation, a fixed size buffer is used to hold data circularly in blobs. When a new blob is written that would cause an overrun
of existing data, one or more entire blobs are deleted to make the necessary space. The write function can be set to return an error
instead of overwrite.
