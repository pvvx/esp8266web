/*********************************************************************
 * WEBFS.c
 * WEB File System v1.0
 ********************************************************************/
#include "c_types.h"
#include "add_sdk_func.h"
#include "bios/ets.h"
#include "hw/esp8266.h"
#include "osapi.h"
#include "flash.h"
#include "WEBFS1.h"

// Supports long file names to 64 characters
#define MAX_FILE_NAME_LEN   64 // VarNameSize
uint32 disk_base_addr;
#define WEBFS_HEAD_ADDR disk_base_addr
/*
 * PVFS Structure:
 *
 *     [F][W][E][B][uint8 Ver Hi][uint8 Ver Lo] // заголовок диска
 *     [uint16 Number of Files] // кол-во файлов на диске
 *
 *     [Name Hash 0][Name Hash 1]...[Name Hash N] // uint16 типа хеш на каждое имя файла :)
 *
 *     [File Record 0][File Record 1]...[File Record N] // uint32 указатели на адреса структур файлов, относительно начала диска
 *
 *     Pointers are absolute addresses within the WEBFS image.
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
/****************************************************************************
  Section:
	Module-Only Globals and Functions
  ***************************************************************************/

// Track the WEBFS File Handles
// WEBFSStubs[0] is reserved for internal use (FAT access)
WEBFS_STUB WEBFSStubs[MAX_WEBFS_OPENFILES+1]; // + HANDLE = 0

// Allows the WEBFS to be locked, preventing access during updates
bool isWEBFSLocked;

// FAT record cache
WEBFS_FAT_RECORD fatCache;

// ID of currently loaded fatCache
static uint16 fatCacheID;

// Number of files in this WEBFS image
uint16 numFiles;


LOCAL void _LoadFATRecord(uint16 fatID);
LOCAL void _Validate(void);

/*****************************************************************************
  Function: 	void WEBFSInit(void)
  ***************************************************************************/
void ICACHE_FLASH_ATTR WEBFSInit(void)
{
/*
	uint16 i;
	for(i = 1; i <= MAX_WEBFS_OPENFILES; i++)
	{
	   WEBFSStubs[i].addr = WEBFS_INVALID;
	} */
	disk_base_addr = WEBFS_base_addr();
	os_memset((char *) &WEBFSStubs, 0xff, sizeof(WEBFSStubs));
	// Validate the image and load numFiles
	_Validate();
#if DEBUGSOO > 0
	os_printf("\nDisk init: %d files, addr = %p\n", numFiles, disk_base_addr);
#endif
	// тут надо расчет контрольки тела диска или другой контроль...
	if(numFiles == 0) isWEBFSLocked = true;
	else isWEBFSLocked = false;
}

/****************************************************************************
  Section:
	Handle Management Functions
  ***************************************************************************/

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
	uint16 nameHash, i, len = 0;
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
		if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
			break;
	if(hWEBFS == MAX_WEBFS_OPENFILES)
		return WEBFS_INVALID_HANDLE;

	// Read in hashes, and check remainder on a match.  Store 8 in cache for performance
	for(i = 0; i < numFiles; i++)
	{
		// For new block of 8, read in data
		if((i & 0x0F) == 0)
		{
			WEBFSStubs[0].addr = 12 + i*2;
			WEBFSStubs[0].bytesRem = 32;
			WEBFSGetArray(0, (uint8*)hashCache, 32);
		}

		// If the hash matches, compare the full filename
		if(hashCache[i&0x0F] == nameHash)
		{
			_LoadFATRecord(i);
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
#if 0
/*****************************************************************************
  Function:
	WEBFS_HANDLE WEBFSOpenID(uint16 hFatID)

  Summary:
	Quickly re-opens a file.

  Description:
	Quickly re-opens a file in the WEBFS2 file system.  Use this function
	along with WEBFSGetID() to quickly re-open a file without tying up
	a permanent WEBFSStub.

  Precondition:
	None

  Parameters:
	hFatID - the ID of a previous opened file in the FAT

  Returns:
	An WEBFS_HANDLE to the opened file if found, or WEBFS_INVALID_HANDLE
	if the file could not be found or no free handles exist.
  ***************************************************************************/
WEBFS_HANDLE ICACHE_FLASH_ATTR WEBFSOpenID(uint16 hFatID)
{
	WEBFS_HANDLE hWEBFS;

	// Make sure WEBFS is unlocked and we got a valid id
	if(isWEBFSLocked == true || hFatID > numFiles)
		return WEBFS_INVALID_HANDLE;

	// Find a free file handle to use
	for(hWEBFS = 1; hWEBFS <= MAX_WEBFS_OPENFILES; hWEBFS++)
		if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
			break;
	if(hWEBFS == MAX_WEBFS_OPENFILES)
		return WEBFS_INVALID_HANDLE;

	// Load the FAT record
	_LoadFATRecord(hFatID);

	// Set up the file handle
	WEBFSStubs[hWEBFS].fatID = hFatID;
	WEBFSStubs[hWEBFS].addr = fatCache.data;
	WEBFSStubs[hWEBFS].bytesRem = fatCache.len;

	return hWEBFS;
}
#endif
/*****************************************************************************
  Function:
	void WEBFSClose(WEBFS_HANDLE hWEBFS)

  Summary:
	Closes a file.

  Description:
	Closes a file and releases its stub back to the pool of available
	handles.

  Precondition:
	None

  Parameters:
	hWEBFS - the file handle to be closed

  Returns:
	None
  ***************************************************************************/
void ICACHE_FLASH_ATTR WEBFSClose(WEBFS_HANDLE hWEBFS)
{
	if(hWEBFS != 0u && hWEBFS <= MAX_WEBFS_OPENFILES)
	    WEBFSStubs[hWEBFS].addr = WEBFS_INVALID;
}
/****************************************************************************
  Section:
	Data Reading Functions
  ***************************************************************************/
#if 0
/*****************************************************************************
  Function:
	bool WEBFSGet(WEBFS_HANDLE hWEBFS, uint8* c)

  Description:
	Reads a uint8 from a file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read
	c - Where to store the uint8 that was read

  Return Values:
	true - The uint8 was successfully read
	false - No uint8 was read because either the handle was invalid or
		the end of the file has been reached.
  ***************************************************************************/
bool ICACHE_FLASH_ATTR WEBFSGet(WEBFS_HANDLE hWEBFS, uint8* cData)
{
	// Make sure we're reading a valid address
	if(hWEBFS > MAX_WEBFS_OPENFILES) return false;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID || WEBFSStubs[hWEBFS].bytesRem == 0) return false;
	if(cData != NULL)
	{
    // Read function for EEPROM
    #if defined(WEBFS_USE_EXTFLASH)
	    // For performance, cache the last read address
		spi_flash_read(WEBFSStubs[hWEBFS].addr + WEBFS_HEAD_ADDR, (uint32 *) c, 1);
    #elif defined(WEBFS_USE_SPI_FLASH)
	  if(spi_flash_read_byte(WEBFSStubs[hWEBFS].addr+WEBFS_HEAD_ADDR, cData) != SPI_FLASH_RESULT_OK)
	    if(spi_flash_read_byte(WEBFSStubs[hWEBFS].addr+WEBFS_HEAD_ADDR, cData) != SPI_FLASH_RESULT_OK)
	      return false;
    #endif
	};
	WEBFSStubs[hWEBFS].addr++;
	WEBFSStubs[hWEBFS].bytesRem--;
	return true;
}
#endif
/*****************************************************************************
  Function:
	uint16 WEBFSGetArray(WEBFS_HANDLE hWEBFS, uint8* cData, uint16 wLen)

  Description:
	Reads a series of uint8s from a file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

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
	#if defined(WEBFS_USE_EXTFLASH)
		spi_flash_read(WEBFSStubs[hWEBFS].addr+WEBFS_HEAD_ADDR, cData, wLen);
	#elif defined(WEBFS_USE_SPI_FLASH)
	  if(spi_flash_read(WEBFSStubs[hWEBFS].addr+WEBFS_HEAD_ADDR, cData, wLen)  != SPI_FLASH_RESULT_OK)
	       return 0;
	#endif
	};
	WEBFSStubs[hWEBFS].addr += wLen;
	WEBFSStubs[hWEBFS].bytesRem -= wLen;
	return wLen;
}
#if 0
/*****************************************************************************
  Function:
	bool WEBFSGetLong(WEBFS_HANDLE hWEBFS, uint32* ul)

  Description:
	Reads a uint32 or Long value from the WEBFS.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read
	ul - where to store the uint32 or long value that was read

  Returns:
	true - The uint8 was successfully read
	false - No uint8 was read because either the handle was invalid or
		the end of the file has been reached.
  ***************************************************************************/
bool ICACHE_FLASH_ATTR WEBFSGetLong(WEBFS_HANDLE hWEBFS, uint32* ul)
{
	return ( WEBFSGetArray(hWEBFS, (uint8*)ul, 4) == 4 );
}
#endif
/*****************************************************************************
  Function:
	bool WEBFSSeek(WEBFS_HANDLE hWEBFS, uint32 dwOffset, WEBFS_SEEK_MODE tMode)

  Description:
	Moves the current read pointer to a new location.

  Precondition:
	The file handle referenced by hWEBFS is already open.

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
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return false;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
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


/****************************************************************************
  Section:
	Data Writing Functions
  ***************************************************************************/

/*****************************************************************************
  Function:
	WEBFS_HANDLE WEBFSFormat(void)

  Summary:
	Prepares the WEBFS image for writing.

  Description:
	Prepares the WEBFS image for writing and locks the image so that other
	processes may not access it.

  Precondition:
	None

  Parameters:
	None

  Returns:
	An WEBFS handle that can be used for WEBFSPut commands, or
	WEBFS_INVALID_HANDLE when the EEPROM failed to initialize for writing.

  Remarks:
	In order to prevent misreads, the WEBFS will be inaccessible until
	WEBFSClose is called.  This function is not available when the WEBFS
	is stored in internal Flash program memory.
  ***************************************************************************/
#if defined(HTTP_WEBFS_UPLOAD) && (defined(WEBFS_USE_EXTFLASH) || defined(WEBFS_USE_SPI_FLASH))
WEBFS_HANDLE WEBFSFormat(void)
{

	uint16 i;

	// Close all files
	for(i = 0; i < MAX_WEBFS_OPENFILES; i++)
		WEBFSStubs[i].addr = WEBFS_INVALID;

	// Lock the image
	isWEBFSLocked = true;

	#if defined(WEBFS_USE_EXTFLASH)
		// Set up SPI Flash for writing
		SPIExtFlashBeginWrite(WEBFS_HEAD_ADDR);
		return 0x00;
	#else
		// Set up SPI Flash for writing
//		spi_flash_erase_sector(sec);
		return 0x00;
	#endif
}
#endif

/*****************************************************************************
  Function:
	uint16 WEBFSPutArray(WEBFS_HANDLE hWEBFS, uint8 *cData, uint16 wLen)

  Description:
	Writes an array of data to the WEBFS image.

  Precondition:
	WEBFSFormat was sucessfully called.

  Parameters:
	hWEBFS - the file handle for writing
	cData - the array of uint8s to write
	wLen - how many uint8s to write

  Returns:
	The number of uint8s successfully written.

  Remarks:
	For EEPROM, the actual write may not initialize until the internal write
	page is full.  To ensure that previously written data gets stored,
	WEBFSPutEnd must be called after the last call to WEBFSPutArray.
  ***************************************************************************/
#if defined(HTTP_WEBFS_UPLOAD) && (defined(WEBFS_USE_EXTFLASH) || defined(WEBFS_USE_SPI_FLASH))
void WEBFSPutArray(WEBFS_HANDLE hWEBFS, uint8* cData, uint16 wLen)
{
	#if defined(WEBFS_USE_EXTFLASH)
		// Write to the SPI Flash
		WEBFSStubs[hWEBFS].addr += wLen;
		SPIExtFlashWriteArray(cData, wLen);
	#else
		// Write to the SPI Flash
		if((WEBFSStubs[hWEBFS].addr+wLen)^WEBFSStubs[hWEBFS].addr)&512)
		  spi_flash_erase_sector((WEBFSStubs[hWEBFS].addr + wLen) >> 9);
		spi_flash_write(WEBFSStubs[hWEBFS].addr, cData, wLen);
		WEBFSStubs[hWEBFS].addr += wLen;
	#endif
}
#endif

/*****************************************************************************
  Function:
	void WEBFSPutEnd(void)

  Description:
	Finalizes an WEBFS writing operation.

  Precondition:
	WEBFSFormat and WEBFSPutArray were sucessfully called.

  Parameters:
	final - true if the application is done writing, false if WEBFS2 called
		this function locally.

  Returns:
	None
  ***************************************************************************/
#if defined(HTTP_WEBFS_UPLOAD) && ( defined(WEBFS_USE_EXTFLASH) || defined(WEBFS_USE_SPI_FLASH))
void WEBFSPutEnd(bool final)
{
	isWEBFSLocked = false;

	if(final)
	{
/*
#ifdef HTTP_WEBFS_DOWNLOAD
	  XEEWriteArray(FRAWEBFS2SIZE, (unsigned char *)&WEBFSStubs[0].addr, 4); // б®еа ­Ёвм а §¬Ґа WEBFS2
#endif
*/
	  _Validate();
	}
}
#endif


/****************************************************************************
  Section:
	Meta Data Accessors
  ***************************************************************************/

/*****************************************************************************
  Function:
	static void _LoadFATRecord(uint16 fatID)

  Description:
	Loads the FAT record for a specified handle.

  Precondition:
	None

  Parameters:
	fatID - the ID of the file whose FAT is to be loaded

  Returns:
	None

  Remarks:
	The FAT record will be stored in fatCache.
  ***************************************************************************/
LOCAL void ICACHE_FLASH_ATTR _LoadFATRecord(uint16 fatID)
{
	WEBFS_FHEADER fhead;
	if(fatID == fatCacheID || fatID >= numFiles)
		return;
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
  Function:
	uint32 WEBFSGetTimestamp(WEBFS_HANDLE hWEBFS)

  Description:
	Reads the timestamp for the specified file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The timestamp that was read as a uint32
  ***************************************************************************/
/*uint32 WEBFSGetTimestamp(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return 0x00000000;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return 0x00000000;

	// Move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.timestamp;
}*/
/*****************************************************************************
  Function:
	uint32 WEBFSGetMicrotime(WEBFS_HANDLE hWEBFS)

  Description:
	Reads the microtime portion of a file's timestamp.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The microtime that was read as a uint32
  ***************************************************************************/
/*uint32 WEBFSGetMicrotime(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return 0x00000000;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return 0x00000000;

	// Move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.microtime;
}
*/
/*****************************************************************************
  Function:
	uint16 WEBFSGetFlags(WEBFS_HANDLE hWEBFS)

  Description:
	Reads a file's flags.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The flags that were associated with the file
  ***************************************************************************/
uint16 ICACHE_FLASH_ATTR WEBFSGetFlags(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return 0x0000;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return 0x0000;

	//move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.flags;
}
/*****************************************************************************
  Function:
	uint32 WEBFSGetSize(WEBFS_HANDLE hWEBFS)

  Description:
	Reads the size of a file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The size that was read as a uint32
  ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFSGetSize(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return 0x00000000;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return 0x00000000;

	// Move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.len;
}

/*****************************************************************************
  Function:
	uint32 WEBFSGetBytesRem(WEBFS_HANDLE hWEBFS)

  Description:
	Determines how many uint8s remain to be read.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The number of uint8s remaining in the file as a uint32
  ***************************************************************************/
uint32 ICACHE_FLASH_ATTR WEBFSGetBytesRem(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return 0x00000000;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return 0x00000000;

	return WEBFSStubs[hWEBFS].bytesRem;
}
/*****************************************************************************
  Function:
	WEBFS_PTR WEBFSGetStartAddr(WEBFS_HANDLE hWEBFS)

  Description:
	Reads the starting address of a file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The starting address of the file in the WEBFS image
  ***************************************************************************/
WEBFS_PTR ICACHE_FLASH_ATTR WEBFSGetStartAddr(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return 0;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return WEBFS_INVALID;

	// Move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.data;
}

/*****************************************************************************
  Function:
	WEBFS_PTR WEBFSGetEndAddr(WEBFS_HANDLE hWEBFS)

  Description:
	Determines the ending address of a file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The address just after the file ends (start address of next file)
  ***************************************************************************/
WEBFS_PTR ICACHE_FLASH_ATTR WEBFSGetEndAddr(WEBFS_HANDLE hWEBFS)
{
	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return WEBFS_INVALID;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return WEBFS_INVALID;

	// Move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	return fatCache.data + fatCache.len;
}
/*****************************************************************************
  Function:
	bool WEBFSGetFilename(WEBFS_HANDLE hWEBFS, uint8* cName, uint16 wLen)

  Description:
	Reads the file name of a file that is already open.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to determine the file name
	cName - where to store the name of the file
	wLen - the maximum length of data to store in cName

  Return Values:
	true - the file name was successfully located
	false - the file handle provided is not currently open
  ***************************************************************************/
bool ICACHE_FLASH_ATTR WEBFSGetFilename(WEBFS_HANDLE hWEBFS, uint8* cName, uint16 wLen)
{
	uint32 addr;

	// Make sure a valid file is open
	if(hWEBFS > MAX_WEBFS_OPENFILES)
		return false;
	if(WEBFSStubs[hWEBFS].addr == WEBFS_INVALID)
		return false;

	// Move to the point for reading
	_LoadFATRecord(WEBFSStubs[hWEBFS].fatID);
	addr = fatCache.string;
	WEBFSStubs[0].addr = addr;
	WEBFSStubs[0].bytesRem = 255;

	// Read the value and return
	WEBFSGetArray(0, cName, wLen);
	return true;
}
/*****************************************************************************
  Function:
	uint32 WEBFSGetPosition(WEBFS_HANDLE hWEBFS)

  Description:
	Determines the current position in the file

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle for which to determine position

  Returns:
	The position in the file as a uint32 (or WEBFS_PTR)

  Remarks:
	Calling WEBFSSeek(hWEBFS, pos, WEBFS_SEEK_START) will return the pointer
	to this position at a later time.  (Where pos is the value returned by
	this function.)
  ***************************************************************************/
uint32  ICACHE_FLASH_ATTR WEBFSGetPosition(WEBFS_HANDLE hWEBFS)
{
	return WEBFSStubs[hWEBFS].addr - WEBFSGetStartAddr(hWEBFS);
}
#if 0
/*****************************************************************************
  Function:
	uint16 WEBFSGetID(WEBFS_HANDLE hWEBFS)

  Description:
	Determines the ID in the FAT for a file.

  Precondition:
	The file handle referenced by hWEBFS is already open.

  Parameters:
	hWEBFS - the file handle from which to read the metadata

  Returns:
	The ID in the FAT for this file

  Remarks:
	Use this function in association with WEBFSOpenID to quickly access file
	without permanently reserving a file handle.
  ***************************************************************************/
uint16 ICACHE_FLASH_ATTR WEBFSGetID(WEBFS_HANDLE hWEBFS)
{
	return WEBFSStubs[hWEBFS].fatID;
}
#endif
/****************************************************************************
  Section:
	Utility Functions
  ***************************************************************************/

/*****************************************************************************
  Function:
	void _Validate(void)

  Summary:
	Validates the WEBFS Image

  Description:
	Verifies that the WEBFS image is valid, and reads the number of
	available files from the image header.  This function is called on
	boot, and again after any image is written.

  Precondition:
	None

  Parameters:
	None

  Returns:
	None
  ***************************************************************************/
LOCAL void ICACHE_FLASH_ATTR _Validate(void)
{
	// Validate the image and update numFiles
	WEBFS_DISK_HEADER dhead;
	WEBFSStubs[0].addr = 0;
	WEBFSStubs[0].bytesRem = sizeof(dhead);
	WEBFSGetArray(0, (uint8*)&dhead, sizeof(dhead));
	if(dhead.id == WEBFS_DISK_ID && dhead.ver == WEBFS_DISK_VER) numFiles = dhead.numFiles; //"FWEB"1,0
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
		size >>= 1;
		size -= WEBFS_DISK_ADDR_MINFLASH;
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
	uint32 addr = WEBFS_DISK_ADDR_MINFLASH;
	if(spi_flash_real_size() > FLASH_MIN_SIZE)	addr = WEBFS_DISK_ADDR_BIGFLASH;
	return addr;
}

