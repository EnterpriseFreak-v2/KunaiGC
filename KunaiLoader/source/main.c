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
#include "kunaigc/kunaigc.h"
#include "bootdol.h"
#include "menus/bootmenu.h"

int screenheight;
int vmode_60hz = 0;
u32 retraceCount;

#include "gfx/kunai_logo.h"
#include "gfx/gfx.h"
#define KUNAI_VERSION "1.0"

struct shortcut {
  u16 pad_buttons;
  char *path;
} shortcuts[] = {
  {PAD_BUTTON_A,     "/a.dol"    },
  {PAD_BUTTON_B,     "/b.dol"    },
  {PAD_BUTTON_X,     "/x.dol"    },
  {PAD_BUTTON_Y,     "/y.dol"    },
  {PAD_BUTTON_LEFT,  "/left.dol" },
  {PAD_BUTTON_RIGHT, "/right.dol"},
  {PAD_BUTTON_UP,    "/up.dol"   },
  // Down is reserved for debuging (delaying exit).
  // NOTE: Shouldn't use L, R or Joysticks as analog inputs are calibrated on boot.
};
int num_shortcuts = sizeof(shortcuts)/sizeof(shortcuts[0]);

extern u8 __xfb[];

GXRModeObj *rmode = NULL;

int main()
{
	VIDEO_Init ();		/*** ALWAYS CALL FIRST IN ANY LIBOGC PROJECT!
					     Not only does it initialise the video
					     subsystem, but also sets up the ogc os
	 ***/

	PAD_Init ();			/*** Initialise pads for input ***/

	// get default video mode
	rmode = VIDEO_GetPreferredMode(NULL);

	switch (rmode->viTVMode >> 2)
	{
	case VI_PAL:
		// 576 lines (PAL 50Hz)
		// display should be centered vertically (borders)
		//Make all video modes the same size so menus doesn't screw up
		rmode = &TVPal576IntDfScale;
		rmode->xfbHeight = 480;
		rmode->viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
		rmode->viHeight = 480;


		vmode_60hz = 0;
		break;

	case VI_NTSC:
		// 480 lines (NTSC 60hz)
		vmode_60hz = 1;
		break;

	default:
		// 480 lines (PAL 60Hz)
		vmode_60hz = 1;
		break;
	}

	/* we have component cables, but the preferred mode is interlaced
	 * why don't we switch into progressive?
	 * (user may not have progressive compatible display but component input)*/
	if(VIDEO_HaveComponentCable()) rmode = &TVNtsc480Prog;
	// configure VI
	VIDEO_Configure (rmode);

	/*** Clear framebuffer to black ***/
	VIDEO_ClearFrameBuffer (rmode, __xfb, COLOR_BLACK);

	/*** Set the framebuffer to be displayed at next VBlank ***/
	VIDEO_SetNextFramebuffer (__xfb);

	/*** Get the PAD status updated by libogc ***/
	//		VIDEO_SetPostRetraceCallback (updatePAD);
	VIDEO_SetBlack (0);

	/*** Update the video for next vblank ***/
	VIDEO_Flush ();

	VIDEO_WaitVSync ();		/*** Wait for VBL ***/
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync ();

	CON_Init(__xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	kprintf("\n\nKunaiLoader - based on iplboot\n");

	kunai_disable();

	// Set the timebase properly for games
	// Note: fuck libogc and dkppc
	u32 t = ticks_to_secs(SYS_Time());
	settime(secs_to_ticks(t));

	PAD_ScanPads();
	u16 buttonsHeld = PAD_ButtonsHeld(PAD_CHAN0);


	if (buttonsHeld & PAD_TRIGGER_Z) {
		bootmenu();
		while(buttonsHeld) {

			PAD_ScanPads();
			buttonsHeld = PAD_ButtonsHeld(PAD_CHAN0);
		};
	}

	kprintf("\n\nKunaiLoader - based on iplboot\n");

	if (loaddol_lfs("swiss.dol")) goto launch;
	if (loaddol_fat("Slot A", &__io_gcsda)) goto launch;
	if (loaddol_fat("Slot A", &__io_gcsdb)) goto launch;
	if (loaddol_fat("SD2SP2", &__io_gcsd2)) goto launch;
	if (loaddol_usb('A')) goto launch;
	if (loaddol_usb('B')) goto launch;

	launch:
	launchDol();
	// If we reach here, all attempts to load a DOL failed
	// Since we've disabled the Qoob, we wil reboot to the Nintendo IPL
	return 0;
}
