#ifndef METADATA_PARSER_H
#define METADATA_PARSER_H

#include <stddef.h>

#define MAX_STRING_LEN 32

struct metadata_slot{
    int slot_id;
    char file_name[MAX_STRING_LEN];
    char version[MAX_STRING_LEN];
    char comment[MAX_STRING_LEN];
};


size_t get_metadata(size_t addr, struct metadata_slot *slots, int max_slots);
void set_metadata_defaults(struct metadata_slot *slots, int max_slots);

#endif // METADATA_PARSER_H
