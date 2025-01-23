#ifndef MENU_H
#define MENU_H

#include "metadata.h"
#include <bootutil/bootutil.h>
/**
 * Displays the bootloader menu and gets the user's choice.
 * @param slots Array of metadata slots.
 * @param slot_count Number of available slots.
 * @return User's choice or -1 if no valid choice is made.
 */
uint8_t display_menu_and_get_choice(const struct metadata_slot *slots, const struct image_header *hdr, uint8_t slot_count);

#endif // MENU_H
