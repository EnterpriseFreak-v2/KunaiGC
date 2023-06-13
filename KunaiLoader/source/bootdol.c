#include <stdio.h>
#include <stdlib.h>
#include <sdcard/gcsd.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <fcntl.h>
#include <ogc/system.h>
#include "etc/ffshim.h"
#include "fatfs/ff.h"
#include "etc/stub.h"
#include "spiflash/spiflash.h"
#include "kunaigc/kunaigc.h"

#define STUB_ADDR  0x80001000
#define STUB_STACK 0x80003000

u8* dol = NULL;
char *path = "/KUNAIGC/ipl.dol";

extern lfs_t lfs;
extern lfs_file_t lfs_file;
extern struct lfs_config cfg;

// Allocates a 32-byte aligned buffer for the binary.
void dol_alloc(int size) {
    int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo());
    kprintf("Memory available: %iB\n", mram_size);

    kprintf("DOL size is %iB\n", size);

    if (size <= 0) {
        kprintf("Empty DOL\n");
        return;
    }

    dol = (u8 *) memalign(32, size);

    if (!dol) {
        kprintf("Couldn't allocate memory\n");
    }
}

// Helper function of loaddol_usb()
unsigned int convert_int(unsigned int in)
{
    unsigned int out;
    char *p_in = (char *) &in;
    char *p_out = (char *) &out;
    p_out[0] = p_in[3];
    p_out[1] = p_in[2];
    p_out[2] = p_in[1];
    p_out[3] = p_in[0];
    return out;
}

#define PC_READY 0x80
#define PC_OK    0x81
#define GC_READY 0x88
#define GC_OK    0x89

// Tries to load a .DOL via USB (Good lucking find a USB Gecko...)
int loaddol_usb(char slot) {
    kprintf("Trying USB Gecko in slot %c\n", slot);

    int channel, res = 1;

    switch (slot) {
        case 'B':
            channel = 1;
            break;

        case 'A':
            channel = 0;
            break;

        default:
            channel = 0;
            break;
    }

    if (!usb_isgeckoalive(channel)) {
        kprintf("Not present\n");
        res = 0;
        goto end;
    }

    usb_flush(channel);

    char data;

    kprintf("Sending ready\n");
    data = GC_READY;
    usb_sendbuffer_safe(channel, &data, 1);

    kprintf("Waiting for ack...\n");
    while ((data != PC_READY) && (data != PC_OK))
        usb_recvbuffer_safe(channel, &data, 1);

    if(data == PC_READY) {
        kprintf("Respond with OK\n");
        // Sometimes the PC can fail to receive the byte, this helps
        usleep(100000);
        data = GC_OK;
        usb_sendbuffer_safe(channel, &data, 1);
    }

    kprintf("Getting DOL size\n");
    int size;
    usb_recvbuffer_safe(channel, &size, 4);
    size = convert_int(size);

    dol_alloc(size);
    unsigned char* pointer = dol;

    if(!dol) {
        res = 0;
        goto end;
    }

    kprintf("Receiving file...\n");
    while (size > 0xF7D8) {
        usb_recvbuffer_safe(channel, (void *) pointer, 0xF7D8);
        size -= 0xF7D8;
        pointer += 0xF7D8;
    }
    if(size)
        usb_recvbuffer_safe(channel, (void *) pointer, size);

end:
    return res;
}

// Attempts to load a .DOL from a FAT device (SD Gecko, SD2SP2)
int loaddol_fat(const char *slot_name, const DISC_INTERFACE *iface_) {
    int res = 1;

    kprintf("Trying %s\n", slot_name);

    FATFS fs;
    iface = iface_;
    if (f_mount(&fs, "", 1) != FR_OK) {
        kprintf("Couldn't mount %s\n", slot_name);
        res = 0;
        goto end;
    }

    char name[256];
    f_getlabel(slot_name, name, NULL);
    kprintf("Mounted %s as %s\n", name, slot_name);

    kprintf("Reading %s\n", path);
    FIL file;
    if (f_open(&file, path, FA_READ) != FR_OK) {
        kprintf("Failed to open file\n");
        res = 0;
        goto unmount;
    }

    size_t size = f_size(&file);
    dol_alloc(size);
    if (!dol) {
        res = 0;
        goto unmount;
    }
    UINT _;
    f_read(&file, dol, size, &_);
    f_close(&file);

unmount:
    kprintf("Unmounting %s\n", slot_name);
    iface->shutdown();
    iface = NULL;

end:
    return res;
}

// Tries to load a .DOL file from LFS (KunaiGC built-in flash memory)
int loaddol_lfs(const char * filePath) {
    int res = 1;
    kprintf("Trying lfs\n");

    //update block_count regarding to flash chip
    u32 jedec_id = kunai_get_jedecID();
    cfg.block_count = (((1 << (jedec_id & 0xFFUL)) - KUNAI_OFFS) / cfg.block_size);

	int err = lfs_mount(&lfs, &cfg);

    if (err != LFS_ERR_OK) {
        kprintf("Couldn't mount lfs\n");
        res = 0;
        goto end;
    }

    kprintf("lfs mounted\n");

    kprintf("Reading %s\n", filePath);
    if (lfs_file_open(&lfs, &lfs_file, filePath, LFS_O_RDWR) != LFS_ERR_OK) {
        kprintf("Failed to open file\n");
        res = 0;
        goto unmount;
    }

    size_t size = lfs_file_size(&lfs, &lfs_file);
    dol_alloc(size);
    if (!dol) {
        res = 0;
        goto unmount;
    }
    lfs_file_read(&lfs, &lfs_file, dol, size);
    lfs_file_close(&lfs, &lfs_file);
unmount:
    kprintf("Unmounting lfs\n");
    lfs_unmount(&lfs);
end:
    return res;
}

// Launches the previously load .DOL
void launchDol() {
    if (dol) {
        memcpy((void *) STUB_ADDR, stub, stub_size);
		DCStoreRange((void *) STUB_ADDR, stub_size);

		SYS_ResetSystem(SYS_SHUTDOWN, 0, FALSE);
		SYS_SwitchFiber((intptr_t) dol, 0,
				(intptr_t) NULL, 0,
				STUB_ADDR, STUB_STACK);
    }

    exit(0);
}
