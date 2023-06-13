#ifndef __BOOTDOL_H
#define __BOOTDOL_H

void dol_alloc(int size);
int loaddol_usb(char slot);
int loaddol_fat(const char *slot_name, const DISC_INTERFACE *iface_);
int loaddol_lfs(const char * filePath);
void launchDol();
#endif
