#!/bin/bash

gcc -g -Werror -Wall -DCBUF_TEST test.c cbuf.c -o test
gcc -g -Werror -Wall -DCBUF_TEST -DCBUF_ALLOW_PARTIAL test_partial.c cbuf.c -o test_partial
