#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <bootutil/bootutil.h>
#include <sysflash/sysflash.h>
#include "metadata.h"

#define MAX_METADATA_SIZE 4096 

__attribute__((section(".metadata"))) static char metadata_buff[MAX_METADATA_SIZE] = {0};

struct flash_area metadata_area = {
    .fa_device_id = FLASH_DEVICE_OSPI,
    .fa_size = MAX_METADATA_SIZE
};


static const char *find_key(const char *json, const char *key) {
    const char *pos = strstr(json, key);
    if (!pos) return NULL;
    return strchr(pos, ':') + 1;
}

static void parse_string(const char *json, char *out, size_t max_len) {
    const char *start = strchr(json, '\"');
    if (!start) return;
    const char *end = strchr(start + 1, '\"');
    if (!end) return;
    size_t len = end - start - 1;
    if (len > max_len - 1) len = max_len - 1;
    strncpy(out, start + 1, len);
    out[len] = '\0';
}

static int parse_int(const char *json) {
    while (*json && !isdigit((unsigned char)*json) && *json != '-') json++;
    return atoi(json);
}

static void parse_version(const char *version_str, struct metadata_slot *slot) {
    if (!version_str || !slot) return;

    int a, b, c;

    if (sscanf(version_str, "%d.%d.%d",
               &a,
               &b,
               &c) == 3) {
        slot->ver_major = a;
        slot->ver_minor = b;
        slot->ver_revision = c;
    } else {
        printf("Error: Failed to parse version string '%s'.\n", version_str);
        slot->ver_major = 0;
        slot->ver_minor = 0;
        slot->ver_revision = 0;
    }

    slot->ver_major = a;
    slot->ver_minor = b;
    slot->ver_revision = c;
}

static int parse_json_slots(const char *metadata, struct metadata_slot *slots, int max_slots) {
    const char *slots_key = "\"slots\"";
    const char *slots_array = strstr(metadata, slots_key);
    if (!slots_array) {
        printf("Error: 'slots' key not found in metadata.\n");
        return -1;
    }

    int slot_count = 0;
    const char *p = strchr(slots_array, '[');
    if (!p) {
        return -1;
    }

    while ((p = strchr(p, '{')) && slot_count < max_slots) {
        memset(&slots[slot_count], 0, sizeof(struct metadata_slot)); // Clear slot

        // Parse slot_id
        const char *slot_id_pos = find_key(p, "\"slot_id\"");
        if (slot_id_pos) slots[slot_count].id = parse_int(slot_id_pos);

        // Parse file_name
        const char *file_pos = find_key(p, "\"file\"");
        if (file_pos) parse_string(file_pos, slots[slot_count].file_name, sizeof(slots[slot_count].file_name));

        // Parse img_size
        const char *size_pos = find_key(p, "\"size\"");
        if (size_pos) slots[slot_count].size = parse_int(size_pos);

        // Parse version
        const char *version_pos = find_key(p, "\"version\"");
        if (version_pos) {
            char version_str[32] = {0};
            parse_string(version_pos, version_str, sizeof(version_str));
            parse_version(version_str, &slots[slot_count]);
        }

        // Parse comment
        const char *comment_pos = find_key(p, "\"comment\"");
        if (comment_pos) parse_string(comment_pos, slots[slot_count].comment, sizeof(slots[slot_count].comment));

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

void set_metadata_to_default(struct metadata_slot *slots, int max_slots) {
    const char *default_value = "no metadata";

    for (int i = 0; i < max_slots; i++) {
        slots[i].id = -1; // Indicates no valid slot ID

        strncpy(slots[i].file_name, default_value, sizeof(slots[i].file_name) - 1);
        slots[i].file_name[sizeof(slots[i].file_name) - 1] = '\0';

        strncpy(slots[i].comment, default_value, sizeof(slots[i].comment) - 1);
        slots[i].comment[sizeof(slots[i].comment) - 1] = '\0'; // Ensure null-termination
    }
}


