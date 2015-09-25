/*********************************************************************
 * WEBFS.c
 * WEB File System v1.0
 ********************************************************************/
#include "user_config.h"
#include "c_types.h"
#include "sdk/add_func.h"
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "osapi.h"
#include "sdk/flash.h"
#include "webfs.h"

// Supports long file names to 64 characters
#define MAX_FILE_NAME_LEN   64 // VarNameSize
uint32 disk_base_addr DATA_IRAM_ATTR;
#define WEBFS_HEAD_ADDR disk_base_addr
/*
 *
 * Structure:
 *
 *     [F][W][E][B][uint8 Ver Hi][uint8 Ver Lo] // заголовок диска
 *     [uint16 Number of Files] // кол-во файлов на диске
 *     [Name Hash 0]...[Name Hash N] // uint16 типа хеш на каждое имя файла 
 *     [File Record 0]...[File Record N] // uint32 указатели на адреса структур файлов, относительно начала диска
 *
 * File Record Structure:
 *     [uint32 Len] размер файла с заголовком
 *     [uint16 HeadLen] длина заголовка, включая размер, флаг, имя (адрес данных - адрес позиции len)
 *     [uint16 Flags] бит 0 =1 - файл сжат GZIP, бит 1 = 1 - парсится - имеет динамические переменные
 *     [File Name, 0] Имя файла с "СИ" терминатором
 *     [File Data] данные файла
 *
 * Name hash (2 uint8s) is calculated as follows:
 *     hash = 0
 *     for each(uint8 in name)
 *	 hash += uint8
 *	 hash <<= 1
 *
 *     Technically this means the hash only includes the
 *     final 15 characters of a name.
 *
 * String FileNmae Structure (1 to 64 uint8s):
 *     ["path/to/file.ext"][0x00]
 *
 *
 * Current version is 1.0
 */
#ifdef USE_MAX_IRAM
// Lock WEBFS access during the upgrade
int isWEBFSLocked DATA_IRAM_ATTR;
// Track the WEBFS File Handles
// WEBFSStubs[0] is reserved for internal use (FAT access)
WEBFS_STUB WEBFSStubs[MAX_WEBFS_OPENFILES+1] DATA_IRAM_ATTR; // + HANDLE = 0
// FAT record cache
WEBFS_FAT_RECORD fatCache;
// ID of currently loaded fatCache
static uint32 fatCacheID DATA_IRAM_ATTR;
// Number of files in this WEBFS image
uint32 numFiles DATA_IRAM_ATTR;
#else
// Lock WEBFS access during the upgrade
bool isWEBFSLocked;
// Track the WEBFS File Handles
// WEBFSStubs[0] is reserved for internal use (FAT access)
WEBFS_STUB WEBFSStubs[MAX_WEBFS_OPENFILES+1]; // + HANDLE = 0
// FAT record cache
WEBFS_FAT_RECORD fatCache;
// ID of currently loaded fatCache
static uint16 fatCacheID;
// Number of files in this WEBFS image
uint16 numFiles;
#endif

LOCAL void GetFATRecord(uint16 fatID);
LOCAL void WEBFS_Update(void);

/*****************************************************************************
  Function: 	void WEBFSInit(void)
  ***************************************************************************/
void ICACHE_FLASH_ATTR WEBFSInit(void)
{
	disk_base_addr = WEBFS_base_addr();
	os_memset((char *) &WEBFSStubs, 0xff, sizeof(WEBFSStubs));
	// Validate the image and load numFiles
	WEBFS_Update();
#if DEBUGSOO > 0
	os_printf("\nDisk init: %d files, addr = %p\n", numFiles, disk_base_addr);
#endif
	// тут надо расчет контрольки тела диска или другой контроль...
	if(numFiles == 0) isWEBFSLocked = true;
	else isWEBFSLocked = false;
}
/*****************************************************************************
  Function:
	WEBFS_HANDLE WEBFSOpen(uint8* cFile)

  Description:
	Opens a file in the WEBFS2 file system.

  Precondition:
	None

  Parameters:
	cFile - a null terminated file name to open

  Returns:
	An WEBFS_HANDLE to the opened file if found, or WEBFS_INVALID_HANDLE
	if the file could not be found or no free handles exist.
  ***************************************************************************/
WEBFS_HANDLE ICACHE_FLASH_ATTR WEBFSOpen(uint8* cFile)
{
	WEBFS_HANDLE hWEBFS;
	uint16 nameHash; 
	int i, len = 0; 
	uint16 hashCache[16];
	uint8 bufname[MAX_FILE_NAME_LEN];
	uint8 *ptr;

	// Make sure WEBFS is unlocked and we got a filename
	if(*cFile == '\0' || isWEBFSLocked == true)
		return WEBFS_INVALID_HANDLE;

	// Calculate the name hash to speed up searching
	for(nameHash = 0, ptr = cFile; *ptr != '\0'; ptr++)
	{
		nameHash += *ptr;
		nameHash <<= 1;
		len++;
	}
	// Find a free file handle to use
	for(hWEBFS = 1; hWEBFS <= MAX_WEBFS_OPENFILES; hWEBFS++)
		if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID) break;
	if(hWEBFS == MAX_WEBFS_OPENFILES)
		return WEBFS_INVALID_HANDLE;
	// Read in hashes, and check remainder on a match.  Store 8 in cache for performance
	for(i = 0; i < numFiles; i++) {
		// For new block of 8, read in data
		if((i & 0x0F) == 0)	{
			WEBFSStubs[0].addr = 12 + i*2;
			WEBFSStubs[0].bytesRem = 32;
			WEBFSGetArray(0, (uint8*)hashCache, 32);
		}
		// If the hash matches, compare the full filename
		if(hashCache[i&0x0F] == nameHash)
		{
			GetFATRecord(i);
			// filename comparison
			WEBFSStubs[0].addr = fatCache.string;
			WEBFSStubs[0].bytesRem = MAX_FILE_NAME_LEN;
			WEBFSGetArray(0, bufname, MAX_FILE_NAME_LEN);
			if(os_strncmp(cFile, bufname, len) == 0) { // Filename matches, so return true
				WEBFSStubs[hWEBFS].addr = fatCache.data;
				WEBFSStubs[hWEBFS].bytesRem = fatCache.len;
				WEBFSStubs[hWEBFS].fatID = i;
				return hWEBFS;
			}
		}
	}
	// No file name matched, so return nothing
	return WEBFS_INVALID_HANDLE;
}
/*****************************************************************************
  Function:	void WEBFSClose(WEBFS_HANDLE hWEBFS)
  Summary: Closes a file.
  Returns:	None
  ***************************************************************************/
void ICACHE_FLASH_ATTR WEBFSClose(WEBFS_HANDLE hWEBFS)
{
	if(hWEBFS != 0 && hWEBFS <= MAX_WEBFS_OPENFILES)
	    WEBFSStubs[hWEBFS].addr = WEBFS_INVALID;
}
/*****************************************************************************
  Function: uint16 WEBFSGetArray(WEBFS_HANDLE hWEBFS, uint8* cData, uint16 wLen)
  Description: Reads a series of uint8s from a file.
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters:
	hWEBFS - the file handle from which to read
	cData - where to store the uint8s that were read
	wLen - how many uint8s to read
  Returns:
	The number of uint8s successfully read.  If this is less than wLen,
	an EOF occurred while attempting to read.
  ***************************************************************************/
uint16 ICACHE_FLASH_ATTR  WEBFSGetArray(WEBFS_HANDLE hWEBFS, uint8* cData, uint16 wLen)
{
	// Make sure we're reading a valid address
	if(hWEBFS > MAX_WEBFS_OPENFILES) return 0;

	// Determine how many we can actually read
	if(wLen > WEBFSStubs[hWEBFS].bytesRem) wLen = WEBFSStubs[hWEBFS].bytesRem;
	// Make sure we're reading a valid address
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID || wLen == 0)  return 0;

	if(cData != NULL)  {

	// Read the data
	  if(spi_flash_read(WEBFSStubs[hWEBFS].addr+WEBFS_HEAD_ADDR, cData, wLen)  != SPI_FLASH_RESULT_OK)
	       return 0;
	};
	WEBFSStubs[hWEBFS].addr += wLen;
	WEBFSStubs[hWEBFS].bytesRem -= wLen;
	return wLen;
}
/*****************************************************************************
  Function:	
   bool WEBFSSeek(WEBFS_HANDLE hWEBFS, uint32 dwOffset, WEBFS_SEEK_MODE tMode)
  Description: Moves the current read pointer to a new location.
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters:
	hWEBFS - the file handle to seek with
	dwOffset - offset from the specified position in the specified direction
	tMode - one of the WEBFS_SEEK_MODE constants
  Returns:
	true - the seek was successful
	false - either the new location or the handle itself was invalid
  ***************************************************************************/
bool ICACHE_FLASH_ATTR WEBFSSeek(WEBFS_HANDLE hWEBFS, uint32 dwOffset, WEBFS_SEEK_MODE tMode)
{
	uint32 temp;

	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES || WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return false;

	switch(tMode)
	{
		// Seek offset uint8s from start
		case WEBFS_SEEK_START:
			temp = WEBFSGetSize(hWEBFS);
			if(dwOffset > temp)
				return false;

			WEBFSStubs[hWEBFS].addr = WEBFSGetStartAddr(hWEBFS) + dwOffset;
			WEBFSStubs[hWEBFS].bytesRem = temp - dwOffset;
			return true;

		// Seek forwards offset uint8s
		case WEBFS_SEEK_FORWARD:
			if(dwOffset > WEBFSStubs[hWEBFS].bytesRem)
				return false;

			WEBFSStubs[hWEBFS].addr += dwOffset;
			WEBFSStubs[hWEBFS].bytesRem -= dwOffset;
			return true;

		// Seek backwards offset uint8s
		case WEBFS_SEEK_REWIND:
			temp = WEBFSGetStartAddr(hWEBFS);
			if(WEBFSStubs[hWEBFS].addr < temp + dwOffset)
				return false;

			WEBFSStubs[hWEBFS].addr -= dwOffset;
			WEBFSStubs[hWEBFS].bytesRem += dwOffset;
			return true;

		// Seek so that offset uint8s remain in file
		case WEBFS_SEEK_END:
			temp = WEBFSGetSize(hWEBFS);
			if(dwOffset > temp)
				return false;

			WEBFSStubs[hWEBFS].addr = WEBFSGetEndAddr(hWEBFS) - dwOffset;
			WEBFSStubs[hWEBFS].bytesRem = dwOffset;
			return true;

		default:
			return false;
	}
}
/*****************************************************************************
  Function:	static void GetFATRecord(uint16 fatID)
  Description: 	Loads the FAT record for a specified handle.
  Precondition:	None
  Parameters:	fatID - the ID of the file whose FAT is to be loaded
  Returns:	None
  Remarks:	The FAT record will be stored in fatCache.
  ***************************************************************************/
LOCAL void ICACHE_FLASH_ATTR GetFATRecord(uint16 fatID)
{
	WEBFS_FHEADER fhead;
	if(fatID == fatCacheID || fatID >= numFiles) return;
	// Read the FAT record to the cache
	WEBFSStubs[0].bytesRem = sizeof(fhead) + 4;
	WEBFSStubs[0].addr = 12 + numFiles*2 + fatID *4;
	WEBFSGetArray(0, (uint8 *)&fatCache.data, 4);
	WEBFSStubs[0].addr = fatCache.data;
	WEBFSGetArray(0, (uint8 *)&fhead, sizeof(fhead));
	fatCache.len = fhead.blksize - fhead.headlen;
	fatCache.string = fatCache.data + 8;
	fatCache.flags = fhead.flags;
	fatCache.data = fatCache.data + fhead.headlen;
	fatCacheID = fatID;
}
/*****************************************************************************
  Function:	uint16 WEBFSGetFlags(WEBFS_HANDLE hWEBFS)
  Description: Reads a file's flags.
  Precondition:	The file handle referenced by hWEBFS is already open.
  Parameters: hWEBFS - the file handle from which to read the metadata
  Returns: The flags that were associated with the file
  ***************************************************************************/
uint16 ICACHE_FLASH_ATTR WEBFSGetFlags(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES	|| WEBFSStubs[hWEBFS].addr == WEBFS_INVALID) 
		return 0;
	
	//move to the point for reading
	GetFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.flags;
}
/*****************************************************************************
  Function: uint32 WEBFSGetSize(WEBFS_HANDLE hWEBFS)
  Description: Reads the size of a file.
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters: hWEBFS - the file handle from which to read the metadata
  Returns: The size that was read as a uint32
  ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFSGetSize(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES	|| WEBFSStubs[hWEBFS].addr == WEBFS_INVALID) 
		return 0;
	
	// Move to the point for reading
	GetFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.len;
}
/*****************************************************************************
  Function: uint32 WEBFSGetBytesRem(WEBFS_HANDLE hWEBFS)
  Description: Determines how many uint8s remain to be read.
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters: hWEBFS - the file handle from which to read the metadata
  Returns: The number of uint8s remaining in the file as a uint32
  ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFSGetBytesRem(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES || WEBFSStubs[hWEBFS].addr == WEBFS_INVALID) 
		return 0;
	return WEBFSStubs[hWEBFS].bytesRem;
}
/*****************************************************************************
  Function:	WEBFS_PTR WEBFSGetStartAddr(WEBFS_HANDLE hWEBFS)
  Description: Reads the starting address of a file.
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters: hWEBFS - the file handle from which to read the metadata
  Returns: The starting address of the file in the WEBFS image
  ***************************************************************************/
WEBFS_PTR ICACHE_FLASH_ATTR WEBFSGetStartAddr(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES || WEBFSStubs[hWEBFS].addr == WEBFS_INVALID) 
		return 0;
	// Move to the point for reading
	GetFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.data;
}
/*****************************************************************************
  Function: WEBFS_PTR WEBFSGetEndAddr(WEBFS_HANDLE hWEBFS)
  Description: Determines the ending address of a file.
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters: hWEBFS - the file handle from which to read the metadata
  Returns:	The address just after the file ends (start address of next file)
  ***************************************************************************/
WEBFS_PTR ICACHE_FLASH_ATTR WEBFSGetEndAddr(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES || WEBFSStubs[hWEBFS].addr == WEBFS_INVALID) 
		return WEBFS_INVALID;
	// Move to the point for reading
	GetFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.data + fatCache.len;
}
/*****************************************************************************
  Function: bool WEBFSGetFilename(WEBFS_HANDLE hWEBFS, uint8* cName, uint16 wLen)
  Description: Reads the file name of a file that is already open.
  Precondition:	The file handle referenced by hWEBFS is already open.
  Parameters: 
    hWEBFS - the file handle from which to determine the file name
	cName - where to store the name of the file
	wLen - the maximum length of data to store in cName
  Returns:
	true - the file name was successfully located
	false - the file handle provided is not currently open
  ***************************************************************************/
bool ICACHE_FLASH_ATTR WEBFSGetFilename(WEBFS_HANDLE hWEBFS, uint8* cName, uint16 wLen)
{
	uint32 addr;

	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES	|| WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return false;

	// Move to the point for reading
	GetFATRecord(WEBFSStubs[hWEBFS].fatID);
	addr = fatCache.string;
	WEBFSStubs[0].addr = addr;
	WEBFSStubs[0].bytesRem = 255;

	// Read the value and return
	WEBFSGetArray(0, cName, wLen);
	return true;
}
/*****************************************************************************
  Function: uint32 WEBFSGetPosition(WEBFS_HANDLE hWEBFS)
  Description: Determines the current position in the file
  Precondition: The file handle referenced by hWEBFS is already open.
  Parameters: hWEBFS - the file handle for which to determine position
  Returns: The position in the file as a uint32 (or WEBFS_PTR)
  ***************************************************************************/
uint32  ICACHE_FLASH_ATTR WEBFSGetPosition(WEBFS_HANDLE hWEBFS)
{
	return WEBFSStubs[hWEBFS].addr - WEBFSGetStartAddr(hWEBFS);
}
/*****************************************************************************
  Function:	void WEBFS_Update(void)
  Summary: Validates the WEBFS Image
  Description: Verifies that the WEBFS image is valid, and reads the number of
	available files from the image header.  This function is called on
	boot, and again after any image is written.
  Parameters: None
  Returns: None
  ***************************************************************************/
LOCAL void ICACHE_FLASH_ATTR WEBFS_Update(void)
{
	// Update numFiles
	WEBFS_DISK_HEADER dhead;
	WEBFSStubs[0].addr = 0;
	WEBFSStubs[0].bytesRem = sizeof(dhead);
	WEBFSGetArray(0, (uint8*)&dhead, sizeof(dhead));
	if(dhead.id == WEBFS_DISK_ID && dhead.ver == WEBFS_DISK_VER) { //"FWEB"1,0 ?
		numFiles = dhead.numFiles; 
	}
	else numFiles = 0;
	fatCacheID = WEBFS_INVALID_FAT;
}
/****************************************************************************
 * WEBFS_max_size()
 ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFS_max_size(void)
{
	uint32 size = spi_flash_real_size();
	if(size > FLASH_MIN_SIZE) size -= WEBFS_DISK_ADDR_BIGFLASH;
	else {
		size = WEBFS_DISK_ADDR_MINFLASH_END - WEBFS_DISK_ADDR_MINFLASH_START;
	}
	return size;
}
/****************************************************************************
 * WEBFS_size()
 ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFS_curent_size(void)
{
	uint32 size = 0;
	if(numFiles) spi_flash_read(disk_base_addr + 8, &size, 4);
	return size;
}
/****************************************************************************
 * WEBFS_size()
 ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFS_base_addr(void)
{
	uint32 addr = WEBFS_DISK_ADDR_MINFLASH_START;
	if(spi_flash_real_size() > FLASH_MIN_SIZE)	addr = WEBFS_DISK_ADDR_BIGFLASH;
	return addr;
}
