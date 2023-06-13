#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include "../menu.h"
#include "../bootdol.h"

void launchSwissDOL() {
    launchDol();
}

void launchIPL() {
    exit(0);
}

void bootmenu() {
    struct menu bootmenu;
    bootmenu.maxItems = 16;
    initializeMenu(&bootmenu);

    addItemToMenu(&bootmenu, "KunaiGC", NULL, 0);
    addItemToMenu(&bootmenu, "Boot on-board swiss.dol", launchSwissDOL, 1);
    addItemToMenu(&bootmenu, "Boot GameCube IPL", launchIPL, 1);

    while(1) {
        processMenu(&bootmenu);
        VIDEO_WaitVSync();
    }
}
