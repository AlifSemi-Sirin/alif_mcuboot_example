#include <stdio.h>
#include <string.h>
#include <bootutil/bootutil.h>
#include "sysflash/sysflash.h"
#include "metadata.h"

#define MAX_METADATA_SIZE 4096 

__attribute__((section(".metadata"))) static char metadata_buff[MAX_METADATA_SIZE] = {0};

struct flash_area metadata_area = {
    .fa_device_id = FLASH_DEVICE_OSPI,
    .fa_size = MAX_METADATA_SIZE
};


static int parse_json_slots(const char *metadata, struct metadata_slot *slots, int max_slots) {
    const char *slots_key = "\"slots\"";
    const char *slot_id_key = "\"slot_id\"";
    const char *file_key = "\"file\"";
    const char *comment_key = "\"comment\"";

    int slot_count = 0;
    const char *p = strstr(metadata, slots_key);
    if (!p) {
        printf("Error: 'slots' key not found in metadata.\n");
        return -1;
    }

    while ((p = strstr(p, "{")) && slot_count < max_slots) {
        memset(&slots[slot_count], 0, sizeof(struct metadata_slot)); // Clear slot

        const char *slot_id_pos = strstr(p, slot_id_key);
        if (slot_id_pos) sscanf(slot_id_pos, "\"slot_id\": %d", &slots[slot_count].slot_id);

        const char *file_pos = strstr(p, file_key);
        if (file_pos) sscanf(file_pos, "\"file\": \"%255[^\"]\"", slots[slot_count].file_name);

        const char *comment_pos = strstr(p, comment_key);
        if (comment_pos) sscanf(comment_pos, "\"comment\": \"%255[^\"]\"", slots[slot_count].comment);

        slot_count++;
        p++;
    }

    return slot_count;
}

size_t get_metadata(size_t addr, struct metadata_slot *slots, int max_slots) {

    metadata_area.fa_off = addr;

    int rc = flash_area_read(&metadata_area, 0, metadata_buff, MAX_METADATA_SIZE);
    if (rc != 0) {
        printf("Error: Failed to read metadata.\n");
        return -1;
    }

    int slot_count = parse_json_slots(metadata_buff, slots, max_slots);
    if (slot_count <= 0) {
        printf("Error: No valid slots found in metadata.\n");
        return -1;
    }

    return slot_count;
}

void set_metadata_defaults(struct metadata_slot *slots, int max_slots) {
    const char *default_value = "no metadata";

    for (int i = 0; i < max_slots; i++) {
        slots[i].slot_id = -1; // Indicates no valid slot ID

        strncpy(slots[i].file_name, default_value, sizeof(slots[i].file_name) - 1);
        slots[i].file_name[sizeof(slots[i].file_name) - 1] = '\0'; // Ensure null-termination

        strncpy(slots[i].comment, default_value, sizeof(slots[i].comment) - 1);
        slots[i].comment[sizeof(slots[i].comment) - 1] = '\0'; // Ensure null-termination
    }
}


