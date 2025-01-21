#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "uart_tracelib.h"

#include "menu.h"

const char *not_found = "----";
#define PRIMARY_SLOT_ID     1


static int uart_read_int(void) {
    char buffer[16]; 
    unsigned int index = 0;
    char ch;

    // Read characters until newline ('\n')
    while (1) {
        receive_str(&ch, 1); // Receive one character at a time

        if (ch == '\n' || ch == '\r') {
            buffer[index] = '\0'; // Null-terminate the string
            break;
        }

        if (isdigit(ch) && index < sizeof(buffer) - 1) {
            buffer[index++] = ch;
        }
    }

    return atoi(buffer);
}


/**
 * Searches for a file name and comment in the slots.
 */
const char* get_metadata_filename(const struct metadata_slot *slots, uint8_t slot_count, const struct image_header *hdr) {
    for (size_t i = 0; i < slot_count; i++) {
        // iv_build_num - contains slot_id !!!
        if (hdr->ih_ver.iv_build_num == PRIMARY_SLOT_ID) {
            if(hdr->ih_img_size == slots[i].size && 
                hdr->ih_ver.iv_major == slots[i].ver_major &&
                hdr->ih_ver.iv_minor == slots[i].ver_minor &&
                hdr->ih_ver.iv_revision == slots[i].ver_revision) {
                    return slots[i].file_name;
            }
        }
        else {
            if (hdr->ih_ver.iv_build_num == slots[i].id) {
                return slots[i].file_name;
            }
        }
    }

    return not_found;
}

const char* metadata_get_comment(const struct metadata_slot *slots, uint8_t slot_count, const struct image_header *hdr) {
    for (size_t i = 0; i < slot_count; i++) {
        // iv_build_num - contains slot_id !!!
        if(hdr->ih_ver.iv_build_num == PRIMARY_SLOT_ID) {
            if(hdr->ih_img_size == slots[i].size && 
                hdr->ih_ver.iv_major == slots[i].ver_major &&
                hdr->ih_ver.iv_minor == slots[i].ver_minor &&
                hdr->ih_ver.iv_revision == slots[i].ver_revision) {
                    return slots[i].comment;
            }
        }
        else {
            if(hdr->ih_ver.iv_build_num == slots[i].id) {
                return slots[i].comment;
            }
        }
    }

    return not_found;    
}

/**
 * Displays the menu and retrieves the user's choice.
 * Handles invalid inputs and ensures a valid choice is returned.
 */
uint8_t display_menu_and_get_choice(const struct metadata_slot *slots, const struct image_header *hdr, uint8_t slot_count) {
    uint8_t choice = 0;

    printf("\n==== Bootloader Menu ====\n");
    for (size_t i = 0; i < slot_count; i++) {
            printf("%d. %s Image: v%d.%d.%d, size %d, slot_id %d, File: %s, Comment: %s\n", 
                i, 
                i == 0 ? "Start Primary" : "Copy Secondary",
               (int)hdr[i].ih_ver.iv_major, 
               (int)hdr[i].ih_ver.iv_minor, 
               (int)hdr[i].ih_ver.iv_revision, 
               (int)hdr[i].ih_img_size,
               (int)hdr[i].ih_ver.iv_build_num,
               get_metadata_filename(slots, slot_count, &hdr[i]),
               metadata_get_comment(slots, slot_count, &hdr[i]));    
    }
    printf("=========================\n");

    do {
        printf("Enter your choice: \n");
        choice = uart_read_int();
    } while (choice >= slot_count);  

    return choice;
}
