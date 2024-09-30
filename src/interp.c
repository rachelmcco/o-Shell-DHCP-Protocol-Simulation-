#include <stdio.h>
#include <stdlib.h>

#include "dhcp.h"
#include "format.h"

/* This work complies with the JMU Honor Code.
 * References and Acknowledgments: I received no outside
 * help with this programming assignment. */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        return EXIT_FAILURE;
    }
    FILE *file = fopen(argv[1], "rb");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    uint8_t *buffer = calloc(file_size, sizeof(uint8_t));
    fread(buffer, 1, file_size, file);
    fclose(file);

    uint8_t *magic_cookie_ptr = buffer + 236;
    uint8_t expected_cookie[] = {0x63, 0x82, 0x53, 0x63};
    bool options = true;
    for (int i = 0; i < sizeof(expected_cookie); ++i)
    {
        if (magic_cookie_ptr[i] != expected_cookie[i])
        {
            options = false;
            break;
        }
    }
    dump_packet(buffer, file_size);
    if (options)
    {
        dump_options(buffer, file_size, true);
    }
    free(buffer);

    return EXIT_SUCCESS;
}
