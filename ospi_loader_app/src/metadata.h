#ifndef METADATA_PARSER_H
#define METADATA_PARSER_H

#include <stddef.h>

#define MAX_STRING_LEN 32

struct metadata_slot{
    uint32_t id;
    uint32_t size;
    
    uint8_t ver_major;
    uint8_t ver_minor;
    uint16_t ver_revision;

    char file_name[MAX_STRING_LEN];
    char comment[MAX_STRING_LEN];
};


size_t get_metadata(size_t addr, struct metadata_slot *slots, int max_slots);
void set_metadata_to_default(struct metadata_slot *slots, int max_slots);

#endif // METADATA_PARSER_H
