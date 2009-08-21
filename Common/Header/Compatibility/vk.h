#ifndef XCSOAR_COMPATIBILITY_VK_H
#define XCSOAR_COMPATIBILITY_VK_H

#ifdef GNAV
/* Triadis Altair */

#define VK_APP1 VK_F1
#define VK_APP2 VK_F2
#define VK_APP3 VK_F3
#define VK_APP4 VK_F4
#define VK_APP5 VK_F5
#define VK_APP6 VK_F6

#elif WINDOWSPC > 0

#define VK_APP1 0x31
#define VK_APP2 0x32
#define VK_APP3 0x33
#define VK_APP4 0x34
#define VK_APP5 0x35
#define VK_APP6 0x36

#elif defined(WIN32_PLATFORM_PSPC)
// Pocket PC

	#if (_WIN32_WCE == 300)
	// Pocket PC 2000
		// App keys
		#define VK_APP1     0xC1
		#define VK_APP2     0xC2
		#define VK_APP3     0xC3
		#define VK_APP4     0xC4
		#define VK_APP5     0xC5
		#define VK_APP6     0xC6
	    // Note - note used on most builds...
		// #define VK_APP7     0xC7
		// #define VK_APP8     0xC8
	#endif
#endif

#endif
