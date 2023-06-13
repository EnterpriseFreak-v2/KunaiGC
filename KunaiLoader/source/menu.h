#ifndef __MENU_H
#define __MENU_H

struct menu {
    int maxItems;
    int currentSlot;
    int cursor;
    struct menuItem* items;
};

struct menuItem {
    char label[32];
    void* function;
    int selectable;
};

int initializeMenu(struct menu *menu);
int destroyMenu(struct menu *menu);
int addItemToMenu(struct menu *menu, const char *label, void *function, int selectable);
void processMenu(struct menu* menu);

#endif
