#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "uart_tracelib.h"

#include "menu.h"

const char *not_found = "----";


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
 * Returns the index of the slot if found, otherwise returns -1.
 */
const char* metadata_get_filename(const struct metadata_slot *slots, uint8_t slot_count, uint8_t wanted_slot_id) {
    for (size_t i = 0; i < slot_count; i++) {
        if(slots[i].slot_id == wanted_slot_id) {
            return slots[i].file_name;
        }
    }

    return not_found;
}

const char* metadata_get_comment(const struct metadata_slot *slots, uint8_t slot_count, uint8_t wanted_slot_id) {
    for (size_t i = 0; i < slot_count; i++) {
        if(slots[i].slot_id == wanted_slot_id) {
            return slots[i].comment;
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
               metadata_get_filename(slots, slot_count, i),
               metadata_get_comment(slots, slot_count, i));    
    }
    printf("=========================\n");

    do {
        printf("Enter your choice: \n");
        choice = uart_read_int();
    } while (choice >= slot_count);  

    return choice;
}
