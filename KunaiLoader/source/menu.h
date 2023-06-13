#ifndef __MENU_H
#define __MENU_H

// Menu structure
struct menu {
    int maxItems;               // Maximum amount of items allowed in this menu. Also used by initializeMenu().
    int currentSlot;            // Used to keep track of how many item slots are already used.
    int cursor;                 // Position of the cursor in the menu.
    struct menuItem* items;     // Pointer to memory allocated by malloc for the menuItem structs.
};

// Item structure
struct menuItem {
    char label[32];             // Label of this item aka what is shown in the menu.
    void* function;             // Function this item executes, can be NULL but shouldn't be as long as selectable is set to 1.
    int selectable;             // Determines if this item can be selected by the cursor. Set this to 0 if you wish the item to simply represent text and do nothing else.
};

// --- Function calls --- //
// Initializes the menu by allocating memory for (menu->maxItems) items and setting them to a known state.
// Returns 0 when successful, returns -1 when an error occurs.
int initializeMenu(struct menu *menu);

// Frees the memory allocated for (menu->maxItems) items by initializeMenu().
// Returns 0 when successful, returns -1 when the menu hasn't been initialized or has been destroyed already.
int destroyMenu(struct menu *menu);

// Adds an item to the menu as long as there's a slot available for it.
// Returns 0 when successful, -1 otherwise.
//
// const char *label: String that's shown in the menu. Maximum length is 31 characters,
// void *function: Address of the function to execute. May be NULL but shouldn't be null if selectable isn't set to 0.
// int selectable: Determines if the item can be selected by the cursor in the menu. Should be set to '0' if the item isn't supposed to be executed.
int addItemToMenu(struct menu *menu, const char *label, void *function, int selectable);

// Draws the menu and handles inputs for it. Should be called once per frame as long as the menu is meant to be active.
void processMenu(struct menu* menu);

#endif
