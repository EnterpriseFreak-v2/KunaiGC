#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ogc/system.h>
#include "gfx/gfx.h"
#include "menu.h"

// This function initializes a menu struct for use.
int initializeMenu(struct menu *menu) {
    if (!menu || menu->maxItems == 0) {
        return -1;
    }

    // Allocate memory for the items.
    menu->items = (struct menuItem*)malloc(sizeof(struct menuItem) * menu->maxItems);
    menu->currentSlot = 0;
    menu->cursor = 0;

    // We can't be certain of the state of the memory we just allocated for our items so let's clear their fields.
    for (int item = 0; item < menu->maxItems; item++) {
        memset(menu->items[item].label, '\0', 31);
        menu->items[item].function = NULL;
        menu->items[item].selectable = 0;
    }

    return 0;
}

// This frees the memory allocated for the items in initializeMenu()
int destroyMenu(struct menu *menu) {
    if (!menu->items) {
        return -1;
    }

    free(menu->items);
    return 0;
}

// Adds an item to the current menu as long as there's a slot available for it.
int addItemToMenu(struct menu *menu, const char *label, void *function, int selectable) {
    if (menu->currentSlot >= menu->maxItems) {
        return -1;
    }

    strncpy(menu->items[menu->currentSlot].label, label, 31);
    menu->items[menu->currentSlot].function = function;
    menu->items[menu->currentSlot].selectable = selectable;
    menu->currentSlot++;
    return 0;
}

// Does all the menu stuff.
int processMenu(struct menu* menu) {
    kprintf("\x1b[1;0H");

    // Make sure the cursor stays in-bounds.
    if (menu->cursor > menu->maxItems || menu->cursor < 0) {
        menu->cursor = 0;
    }

    // Print the items
    for (int i = 0; i < menu->currentSlot; i++) {

        kprintf(" %s ", i == menu->cursor ? "*" : " ");
        kprintf("%s\n", (const char*)menu->items[i].label);
    }

    // Handle controller inputs
    PAD_ScanPads();
    unsigned short buttonsDown = PAD_ButtonsDown(PAD_CHAN0);
    unsigned short buttonsHeld = PAD_ButtonsHeld(PAD_CHAN0);

    // DPAD Up / place cursor on previous item
    if (buttonsDown & PAD_BUTTON_UP) {
        menu->cursor--;
    }

    // DPAD Down / place cursor on next item
    if (buttonsDown & PAD_BUTTON_DOWN) {
        menu->cursor++;
    }

    // Make sure the cursor is on an item that's selectable.
    if (!menu->items[menu->cursor].selectable) {
        int foundSelectable = 0;

        // If we're trying to go down in the menu search forwards first and then backwards if we can't find a selectable item forwards.
        if (buttonsDown & PAD_BUTTON_DOWN) {

            // Forward search
            for(int searchForward = menu->cursor; searchForward < menu->currentSlot; searchForward++) {
                if (menu->items[searchForward].selectable) {
                    foundSelectable = 1;
                    menu->cursor = searchForward;
                    break;
                }
            }

            // Backwards search
            if (!foundSelectable) {
                for (int searchBackwards = menu->cursor; searchBackwards >= 0; searchBackwards--) {
                    if (menu->items[searchBackwards].selectable) {
                        foundSelectable = 1;
                        menu->cursor = searchBackwards;
                        break;
                    }
                }
            }
        }

        // If we're trying to go up in the menu search backwards first and then forwards if we can't find a selectable item backwards.
        if (buttonsDown & PAD_BUTTON_UP) {

            // Backwards search
            for (int searchBackwards = menu->cursor; searchBackwards >= 0; searchBackwards--) {
                if (menu->items[searchBackwards].selectable) {
                    foundSelectable = 1;
                    menu->cursor = searchBackwards;
                    break;
                }
            }

            // Forwards search
            if (!foundSelectable) {
                for(int searchForward = menu->cursor; searchForward < menu->currentSlot; searchForward++) {
                    if (menu->items[searchForward].selectable) {
                        foundSelectable = 1;
                        menu->cursor = searchForward;
                        break;
                    }
                }
            }
        }
    }

    // Jump to the function assigned to this item. Make sure it's not NULL first, we don't want to crash the system.
    if (buttonsDown & PAD_BUTTON_A && menu->items[menu->cursor].function != NULL) {
        typedef void fn(void);
        ((fn*)menu->items[menu->cursor].function)();
    }

    // If this menu has enableClosing set and the B button is pressed return PROCESS_MENU_CLOSED to let the caller know the menu has been closed by the user.
    if (buttonsDown & PAD_BUTTON_B && menu->enableClosing) {
        return PROCESS_MENU_CLOSED;
    }

    return PROCESS_MENU_DONE;
}
