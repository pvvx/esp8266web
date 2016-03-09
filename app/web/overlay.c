/*
 * overlay.c
 *
 *  Created on: 01/02/2016
 *      Author: PVV
 */
#include "user_config.h"
#ifdef USE_OVERLAY
#include "bios.h"
#include "hw/esp8266.h"
#include "webfs.h"
#include "sdk/rom2ram.h"
#include "overlay.h"
//#include "web_srv_int.h"

#define mMIN(a, b)  ((a<b)?a:b)

tovl_call * ovl_call DATA_IRAM_ATTR;

int ICACHE_FLASH_ATTR ovl_loader(uint8 *filename)
{
	struct SPIFlashHeader fhead;
	struct SPIFlashHeadSegment fseg;
	WEBFS_HANDLE ffile;
	uint8 rambuf[64];
	uint32 len;
	int ret = -2;
	if((ffile = WEBFSOpen(filename)) != WEBFS_INVALID_HANDLE) {
		if(WEBFSGetArray(ffile, (uint8*)&fhead, sizeof(fhead)) == sizeof(fhead)
		&& fhead.head.id == LOADER_HEAD_ID
		) {
			if(fhead.entry_point >= IRAM_BASE  && ovl_call != NULL) {
				ovl_call(0); // close прошлый оверлей
				ovl_call = NULL;
			}
			while(fhead.head.number_segs) {
				if(WEBFSGetArray(ffile, (uint8*)&fseg, sizeof(fseg)) == sizeof(fseg)
				&& fseg.segment_size != 0
				) while(fseg.segment_size) {
					len = mMIN(fseg.segment_size, sizeof(rambuf));
					if(WEBFSGetArray(ffile, rambuf, len) == len) {
						ret = -1; // ошибка
						break;
					}
					copy_s1d4((void *)fseg.memory_offset, rambuf, len);
					fseg.memory_offset += len;
					fseg.segment_size -= len;
				}
			}
			if(fhead.entry_point >= IRAM_BASE) {
				ovl_call = (tovl_call *)fhead.entry_point;
				ret = 0;
			}
			else ret = 1;
		}
		else ret = -1;
		WEBFSClose(ffile);
	}
	return ret;
}

#endif
