/*
    rom.c - rom/disk handler

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m1snd.h"
#include "m1ui.h"
#include "rom.h"
#include "unzip.h"
#include "md5.h"
#include "sha1.h"
#include "chd.h"

#ifdef _WIN32
#include <windows.h>
#endif

// locals
static char *holding;	// holding area for roms
static char path[4096], pathcpy[4096];
static RomRegionT regions[RGN_MAX];
static struct chd_file *disks[8];	// up to 8 disks per game
static int curdisk = -1;

#define unzFastLocateFile unzLocateFile
#define unzReadFastLocateTable(x) 

#define HOLDING_SIZE (16*1024*1024)

static struct chd_interface_file *chdman_open(const char *filename, const char *mode);
static void chdman_close(struct chd_interface_file *file);
static UINT32 chdman_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 chdman_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 chdman_length(struct chd_interface_file *file);

static struct chd_interface zinc_chd_interface =
{
	chdman_open,
	chdman_close,
	chdman_read,
	chdman_write,
	chdman_length
};

/* unzip stuff */

const char *cur_zip_path;

static int OPENZIP(unzFile *zipArchive, char* zipname)
{
	*zipArchive = unzOpen(zipname);
	cur_zip_path = zipname;

	if (NULL == *zipArchive)
	{
		return 0;
	}

	unzReadFastLocateTable(zipArchive);

	return 1;
}

static void CLOSEZIP(unzFile zipArchive)
{
	unzClose(zipArchive);
}

static int LOADROM(unzFile zipArchive, char *path, char* mem, int size, unsigned int crc32)
{
	unz_file_info info;
	int ret;
	char estr[2048];

//	printf("LOADROM: %s, size %d, crc32 %x\n", path, size, crc32);

	ret = unzLocateFile(zipArchive, path, 0);
	if ((ret == UNZ_END_OF_LIST_OF_FILE) && (crc32 != 0)) 
	{
		// Try to load by crc32
		ret = unzGoToFirstFile(zipArchive);
		if(ret != UNZ_OK) {
			sprintf(estr, "unzGoToFirstFile failed with error code %d, corrupt zip file (%s) ?", ret, cur_zip_path);
			m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
			return 0;
		}
		for(;;) {
			ret = unzGetCurrentFileInfo(zipArchive, &info, 0, 0, 0, 0, 0, 0);
			if(ret != UNZ_OK) {
				sprintf(estr, "unzGetCurrentFileInfo failed with error code %d, corrupt zip file (%s) ?\n", ret, cur_zip_path);
				m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
				return 0;
			}

//			printf("CRC scanning: looking for %x, got %lx\n", crc32, info.crc);

			if(info.crc == crc32)
				break;

			ret = unzGoToNextFile(zipArchive);
			if(ret == UNZ_END_OF_LIST_OF_FILE) {
//				sprintf(estr, "Rom file %s (crc32 %08x) not found in %s\n", path, crc32, cur_zip_path);
				return 0;
			} else if(ret != UNZ_OK) {
				sprintf(estr, "unzGoToNextFile failed with error code %d, corrupt zip file (%s) ?\n", ret, cur_zip_path);
				m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
				return 0;
			}
		}
	} else if(ret != UNZ_OK) {
		sprintf(estr, "Could not find file %s in zip %s\n", path, cur_zip_path);
		m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
		return 0;
	}

	ret = unzGetCurrentFileInfo(zipArchive, &info, 0, 0, 0, 0, 0, 0);
	if(ret != UNZ_OK) {
		return 0;
	}

	if(info.uncompressed_size != size) 
	{
		sprintf(estr, "Wrong size for file %s, expected %d and got %ld\n", path, size, info.uncompressed_size);
		m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
		return 0;
	}

	if ((info.crc != crc32) && (crc32 != 0))
	{
		sprintf(estr, "Wrong crc32 for file %s, expected %08x and got %08lx\n", path, crc32, info.crc);
		m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
		return 0;
	}

	ret = unzOpenCurrentFile(zipArchive);
	if(ret != UNZ_OK) {
		sprintf(estr, "unzOpenCurrentFile failed with error code %d for the rom %s, corrupt zip file (%s) ?\n", ret, path, cur_zip_path);
		m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
		return 0;
	}

	ret = unzReadCurrentFile(zipArchive, mem, size);
	if(ret != size) {
		sprintf(estr, "unzReadCurrentFile failed with return %d for the rom %s, corrupt zip file (%s) ?\n", ret, path, cur_zip_path);
		m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
		return 0;
	}

	ret = unzCloseCurrentFile(zipArchive);
	if(ret != UNZ_OK) {
		sprintf(estr, "unzCloseCurrentFile failed with error code %d for the rom %s, corrupt zip file (%s) ?\n", ret, path, cur_zip_path);
		m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);
		return 0;
	}

	return 1;
}

// internal rom functions
static int rom_loadrom(RomEntryT *entry, unzFile zipArc, int region)
{
	unsigned char *loadptr, *src, temp;
	unsigned int *lptr;
	int i, skip, length, gsize;
	struct sha1_ctx sha1ctx;
	UINT8 sha1_bin[20];
	char sha1_txt[41];
	UINT32 *psha;

//	printf("loading ROM %s of length %ld at offset %ld of region %d (zip %x)\n", entry->name, entry->length, entry->loadadr, region, (int)zipArc);

	// make sure the ROM fits in our temporary buffer
	if (entry->length > HOLDING_SIZE)
	{
		printf("ERROR: ROM of size %ld overflows holding bin size %d\n", entry->length, HOLDING_SIZE);
		return 0;
	}

	// make sure the zip is OK
	if (zipArc == (unzFile)NULL)
	{
		return 0;
	}

	// make sure the ROM fits in it's region
	if ((entry->loadadr + entry->length) > regions[region].size)
	{
		char str[256];

		sprintf(str, "ERROR: ROM %s extends past the end of the region!", entry->name);
		m1ui_message(m1ui_this, M1_MSG_ERROR, str, 0);
		return 0;
	}

	// load the ROM's contents into our temporary buffer
	if (!LOADROM(zipArc, entry->name, holding, entry->length, entry->crc))
	{
		return 0;
	}

	// check it's SHA1 if we got one from the xml
	//if (!strcmp(entry->sha1, ""))//entry->sha1[0] != '\0')
	if (entry->sha1[0] != '\0')
	{
		sha1_init(&sha1ctx);
		sha1_update(&sha1ctx, entry->length, (UINT8 *)holding);
		sha1_final(&sha1ctx);
		sha1_digest(&sha1ctx, 20, (UINT8 *)&sha1_bin[0]);

		// now convert the SHA1 to a text format for comparison
		psha = (UINT32 *)&sha1_bin[0];
		sprintf(sha1_txt, "%08x%08x%08x%08x%08x\n",
			mem_readlong_swap(&psha[0]),
			mem_readlong_swap(&psha[1]),
			mem_readlong_swap(&psha[2]),
			mem_readlong_swap(&psha[3]),
			mem_readlong_swap(&psha[4]));

		if (strncmp(sha1_txt, entry->sha1, 40))
		{
			char str[256];

			sprintf(str, "ERROR: ROM %s has an incorrect SHA1", entry->name);
			m1ui_message(m1ui_this, M1_MSG_ERROR, str, 0);
			return 0;
		}
	}

	// figure out where it's really gonna end up
	loadptr = (unsigned char *)regions[region].memory;
	loadptr += entry->loadadr;

	// apply any fun process steps
	// first off, any swapping?
	if (entry->flags & ROM_REVERSE)
	{
		if (!(entry->flags & ROM_WIDTHMASK))
		{
			char str[128];

			sprintf(str, "Warning: ROM_REVERSE with no WIDTHMASK makes no sense!");
			m1ui_message(m1ui_this, M1_MSG_ERROR, str, 0);
		}

		if ((entry->flags & ROM_WIDTHMASK) == ROM_WORD)
		{
//			printf("wordswapping rom %s\n", entry->name);
			for (i = 0; i < entry->length/2; i++)
			{
				temp = holding[(i*2)];
				holding[(i*2)] = holding[(i*2)+1];
				holding[(i*2)+1] = temp;
			}
		}

		if ((entry->flags & ROM_WIDTHMASK) == ROM_DWORD)
		{
//			printf("longswapping rom %s\n", entry->name);
			lptr = (unsigned int *)holding;
			for (i = 0; i < entry->length/4; i++)
			{
				*lptr = mem_readlong_swap(lptr);
			}
		}
	}

	switch (entry->flags & ROM_WIDTHMASK)
	{
		case ROM_WORD:
			gsize = 2;
			break;

		case ROM_DWORD:
			gsize = 4;
			break;

		default:
			gsize = 1;
			break;
	}

	skip = (entry->flags & ROM_SKIPMASK)>>12;

	length = entry->length;
	src = (unsigned char *)holding;
//	printf("gsize %d skip %d loadptr %lx\n", gsize, skip, (long)loadptr);
	while (length > 0)
	{
		// copy one "group-size" unit to the final destination
		for (i = 0; i < gsize; i++)
		{
			*loadptr++ = *src++;
			length--;
		}

		// now do the actual skip if there is one
		if (skip)
		{
			loadptr += skip;
		}
	}	

	return 1;
}

// swap regions defined with a specific endianness
static void swap_regions(void)
{
	int rgn;

	for (rgn = 0; rgn < RGN_MAX; rgn++)
	{
		if (regions[rgn].size)
		{
			// if the region endianness doesn't match the host,
			// we must byteswap it
			#if LSB_FIRST
			if (regions[rgn].flags & RGN_BE)
			#else
			if (regions[rgn].flags & RGN_LE)
			#endif
			{
				if (regions[rgn].flags & RGN_32BIT)
				{
					long len, t;

					for (len = 0; len < regions[rgn].size; len+=4)
					{
						t = mem_readlong_swap_always((unsigned int *)&regions[rgn].memory[len]);
						mem_writelong((unsigned int *)&regions[rgn].memory[len], t);
					}
				}
				else	// 16-bit
				{
					char t;
					long len;

					for (len = 0; len < regions[rgn].size; len+=2)
					{
						t = regions[rgn].memory[len];
						regions[rgn].memory[len] = regions[rgn].memory[len+1];
						regions[rgn].memory[len+1] = t;
					}
				}
			}
		}
	}
}

// public functions

int rom_loadgame(void)
{
	int i, rgn, currgn = -1;
 	unzFile ourRom, parentRom, parentParent;
	int	must_have_parent = 0;
	const struct chd_header *chd_hdr;
	char sha1_txt[41], md5_txt[33];
	UINT32 *psha;

	// reset disk counter
	curdisk = -1; 	

	// make sure CHD interface is set
	chd_set_interface(&zinc_chd_interface);

	ourRom = parentRom = parentParent = (unzFile)NULL;

	// allocate "holding area"
	holding = malloc(HOLDING_SIZE);

	// init all regions
	for (i = 0; i < RGN_MAX; i++)
	{
		regions[i].memory = NULL;
		regions[i].flags = 0;
	}

	// open the original set
	strcpy(path, games[curgame].zipname);
	strcat(path, ".zip");

	// ask the UI to find it in it's ROM path (this is OS-specific)
	strcpy(pathcpy, path);
	if (!m1ui_message(m1ui_this, M1_MSG_MATCHPATH, pathcpy, 0))
	{
		if (games[curgame].parentzip[0] == 0)
		{
			sprintf(path, "ROMs are missing: %s.zip", games[curgame].zipname);
			m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);
			return 0;
		}
		else
		{
			must_have_parent = 1;
		}
	}

	if (!OPENZIP(&ourRom, path))
	{
		if (games[curgame].parentzip[0] == 0)
		{
			sprintf(path, "ROMs are missing: %s.zip", games[curgame].zipname);
			m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);
			return 0;
		}
		else
		{
			must_have_parent = 1;
		}
	}

	// and the parent if any
	parentRom = NULL;
	if (games[curgame].parentzip[0] != 0)
	{
		path[0] = '\0';
		strcpy(path, games[curgame].parentzip);
		strcat(path, ".zip");

		// ask the UI to find it in it's ROM path (this is OS-specific)
		strcpy(pathcpy, path);
		if (!m1ui_message(m1ui_this, M1_MSG_MATCHPATH, pathcpy, 0))
		{
			if (must_have_parent)
			{
				sprintf(path, "Parent ROMs are missing: %s.zip", games[curgame].parentzip);
				m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);
				return 0;		
			}
		}

		if (!OPENZIP(&parentRom, path))
		{
			if (must_have_parent)
			{
				sprintf(path, "Parent ROMs are missing: %s.zip", games[curgame].parentzip);
				m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);
				return 0;
			}
		}
	}

	// ok, roms are open, lets do it
	for (i = 0; i < ROM_MAX; i++)
	{
		if (games[curgame].roms[i].flags & ROM_ENDLIST)
		{
			i = ROM_MAX;
			continue;
		}		

		// handle a region definition
		if (games[curgame].roms[i].flags & ROM_RGNDEF)
		{
			rgn = games[curgame].roms[i].loadadr;

			regions[rgn].size = games[curgame].roms[i].length;
			regions[rgn].flags = (games[curgame].roms[i].flags & RGN_MASK);
			regions[rgn].memory = (char *)malloc(regions[rgn].size);

			// if the region is supposed to be pre-set to some value, do it
			if (regions[rgn].flags & RGN_CLEAR)
			{
				memset(regions[rgn].memory, (regions[rgn].flags>>8)&0xff, regions[rgn].size);
			}

			// set this to the current region
			currgn = rgn;
		}
		else
		{
			if (currgn == -1)
			{
				sprintf(path, "ERROR: loading ROM %s with no region defined", games[curgame].roms[i].name);
				m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);

				free(holding);
				CLOSEZIP(ourRom);
				if (parentRom)
				{
					CLOSEZIP(parentRom);	
				}
				return 0;
			}

			// is this a disk region?
			if (currgn == RGN_DISK)
			{
				// locate the disk
				path[0] = '\0';
				strcpy(path, games[curgame].roms[i].name);
				strcat(path, ".chd");

				// ask the UI to find it in it's ROM path (this is OS-specific)
				strcpy(pathcpy, path);
				if (!m1ui_message(m1ui_this, M1_MSG_MATCHPATH, pathcpy, 0))
				{
					// not directly in a path, try setname/ in the path
					path[0] = '\0';
					strcpy(path, games[curgame].zipname);
					#if WIN32
					strcat(path, "\\");
					#else
					strcat(path, "/");
					#endif
					strcat(path, games[curgame].roms[i].name);
					strcat(path, ".chd");
					strcpy(pathcpy, path);
					if (!m1ui_message(m1ui_this, M1_MSG_MATCHPATH, pathcpy, 0))
					{
						sprintf(path, "Disk is missing: %s.chd", games[curgame].roms[i].name);
						m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);
						return 0;		
					}
				}

				curdisk++;
				disks[curdisk] = chd_open(path, 0, (struct chd_file *)NULL);

				if (disks[curdisk] == (struct chd_file *)NULL)
				{
					sprintf(path, "Couldn't open disk: %s.chd", games[curgame].roms[i].name);
					m1ui_message(m1ui_this, M1_MSG_ERROR, path, 0);
					return 0;		
				}
				else
				{
					chd_hdr = chd_get_header(disks[curdisk]);

					// convert the MD5 to a text format for comparison
					psha = (UINT32 *)&chd_hdr->md5[0];
					sprintf(md5_txt, "%08x%08x%08x%08x",
						mem_readlong_swap(&psha[0]),
						mem_readlong_swap(&psha[1]),
						mem_readlong_swap(&psha[2]),
						mem_readlong_swap(&psha[3]));

					// convert the SHA1 to a text format for comparison
					psha = (UINT32 *)&chd_hdr->sha1[0];
					sprintf(sha1_txt, "%08x%08x%08x%08x%08x",
						mem_readlong_swap(&psha[0]),
						mem_readlong_swap(&psha[1]),
						mem_readlong_swap(&psha[2]),
						mem_readlong_swap(&psha[3]),
						mem_readlong_swap(&psha[4]));

					if ((strncmp(sha1_txt, games[curgame].roms[i].sha1, 40)) || (strncmp(md5_txt, games[curgame].roms[i].md5, 32)))
					{
						char str[256];

						sprintf(str, "ERROR: disk %s checksums don't match", games[curgame].roms[i].name);
						m1ui_message(m1ui_this, M1_MSG_ERROR, str, 0);
						return 0;
					}
				}
			}
			else
			{
				if ((must_have_parent) || (!rom_loadrom(&games[curgame].roms[i], ourRom, currgn)))
				{
					if (!rom_loadrom(&games[curgame].roms[i], parentRom, currgn))
					{
						char estr[2048];

						sprintf(estr, "ERROR: unable to load ROM %s", games[curgame].roms[i].name);
						m1ui_message(m1ui_this, M1_MSG_ERROR, estr, 0);

						free(holding);
						CLOSEZIP(ourRom);
						if (parentRom)
						{
							CLOSEZIP(parentRom);	
						}
						return 0;
					}

				}
			}
		}
	}

	CLOSEZIP(ourRom);
	if (parentRom)
	{
		CLOSEZIP(parentRom);	
	}

	free(holding);

	swap_regions();

   	return 1;
}

// get a region's base
unsigned char *rom_getregion(int rgn)
{
	return ((unsigned char *)regions[rgn].memory);
}

// get a region's size
long rom_getregionsize(int rgn)
{
	return (regions[rgn].size);
}

// free any RGN_DISPOSE regions after the board's init 
void rom_postinit(void)
{
	int i;

	for (i = 0; i < RGN_MAX; i++)
	{
		if (regions[i].flags & RGN_DISPOSE)
		{
			free(regions[i].memory);
			regions[i].memory = NULL;
			regions[i].size = 0;
			regions[i].flags = 0;
		}
	}
}

// dispose all regions
void rom_shutdown(void)
{
	int i;

	if (curdisk >= 0)
	{
		for (i = 0; i < (curdisk+1); i++)
		{
			chd_close(disks[i]);
		}
	}

	for (i = 0; i < RGN_MAX; i++)
	{
		if (regions[i].memory)
		{
 //			printf("rom_shutdown: freeing region %d\n", i);
			free(regions[i].memory);
			regions[i].memory = NULL;
			regions[i].size = 0;
			regions[i].flags = 0;
		}
	}
}

static int rom_findnumber(int game, int romnum)
{
	int rmn = -1, i;

	for (i = 0; i < ROM_MAX; i++)
	{
		// if we hit the end of the list, exit
		if (games[game].roms[i].flags & ROM_ENDLIST)
		{
			i = ROM_MAX;
			break;
		}

		// if it's not a region def, it counts.
		if (!(games[game].roms[i].flags & ROM_RGNDEF))
		{
			rmn++;
		}

		if (rmn == romnum)
		{
			return i;
		}
	}

	return -1;
}

char *rom_getfilename(int game, int romnum)
{
	int rom = rom_findnumber(game, romnum);

	return games[game].roms[rom].name;
}

int rom_getfilesize(int game, int romnum)
{
	int rom = rom_findnumber(game, romnum);

	return games[game].roms[rom].length;
}

int rom_getfilecrc(int game, int romnum)
{
	int rom = rom_findnumber(game, romnum);

	return games[game].roms[rom].crc;
}

int rom_getnum(int game)
{
	int i, nroms;

	nroms = 0;

	for (i = 0; i < ROM_MAX; i++)
	{
		if (games[game].roms[i].flags & ROM_ENDLIST)
		{
			i = ROM_MAX;
			break;
		}

		// if it's not a region def, it counts.
		if (!(games[game].roms[i].flags & ROM_RGNDEF))
		{
			nroms++;
		}
	}

	return nroms;
}

void rom_setpath(char *newpath)
{
	strcpy(path, newpath);
}

/* CHD glue code */

static struct chd_file *special_chd;
static UINT64 special_logicalbytes;
static UINT64 special_original_logicalbytes;
static UINT64 special_bytes_checksummed;
static UINT32 special_error_count;
static struct MD5Context special_md5;
static struct sha1_ctx special_sha1;

#define SPECIAL_CHD_NAME "??SPECIALCHD??"

#ifdef _WIN32
static int is_physical_drive(const char *file)
{
	return !_strnicmp(file, "\\\\.\\physicaldrive", 17);
}
#endif

static struct chd_interface_file *chdman_open(const char *filename, const char *mode)
{
	/* if it's the special CHD filename, just hand back our handle */
	if (!strcmp(filename, SPECIAL_CHD_NAME))
		return (struct chd_interface_file *)special_chd;

	/* otherwise, open normally */
	else
	{
#ifdef _WIN32
		DWORD disposition, access, share = 0;
		HANDLE handle;

		/* convert the mode into disposition and access */
		if (!strcmp(mode, "rb"))
			disposition = OPEN_EXISTING, access = GENERIC_READ, share = FILE_SHARE_READ;
		else if (!strcmp(mode, "rb+"))
			disposition = OPEN_EXISTING, access = GENERIC_READ | GENERIC_WRITE;
		else if (!strcmp(mode, "wb"))
			disposition = CREATE_ALWAYS, access = GENERIC_WRITE;
		else
			return NULL;

		/* if this is a physical drive, we have to share */
		if (is_physical_drive(filename))
		{
			disposition = OPEN_EXISTING;
			share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}

		/* attempt to open the file */
		handle = CreateFile(filename, access, share, NULL, disposition, 0, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			return NULL;
		return (struct chd_interface_file *)handle;
#else
		return (struct chd_interface_file *)fopen(filename, mode);
#endif
	}
}



/*-------------------------------------------------
	chdman_close - close file
-------------------------------------------------*/

static void chdman_close(struct chd_interface_file *file)
{
	/* if it's the special chd handle, do nothing */
	if (file == (struct chd_interface_file *)special_chd)
		return;

#ifdef _WIN32
	CloseHandle((HANDLE)file);
#else
	fclose((FILE *)file);
#endif
}



/*-------------------------------------------------
	chdman_read - read from an offset
-------------------------------------------------*/

static UINT32 chdman_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	/* if it's the special chd handle, read from it */
	if (file == (struct chd_interface_file *)special_chd)
	{
		const struct chd_header *header = chd_get_header(special_chd);
		UINT32 result;

		/* validate the read: we can only handle block-aligned reads here */
		if (offset % header->hunkbytes != 0)
		{
			printf("Error: chdman read from non-aligned offset %08X%08X\n", (UINT32)(offset >> 32), (UINT32)offset);
			return 0;
		}
		if (count % header->hunkbytes != 0)
		{
			printf("Error: chdman read non-aligned amount %08X\n", count);
			return 0;
		}

		/* if we're reading past the logical end, indicate we didn't read a thing */
		if (offset >= special_logicalbytes)
			return 0;

		/* read the block(s) */
		result = header->hunkbytes * chd_read(special_chd, offset / header->hunkbytes, count / header->hunkbytes, buffer);

		/* count errors */
		if (result != count && chd_get_last_error() != CHDERR_NONE)
			special_error_count++;

		/* update the checksums */
		if (offset == special_bytes_checksummed && offset < special_original_logicalbytes)
		{
			UINT32 bytestochecksum = result;

			/* clamp to the original number of logical bytes */
			if (special_bytes_checksummed + bytestochecksum > special_original_logicalbytes)
				bytestochecksum = special_original_logicalbytes - special_bytes_checksummed;

			/* update the two checksums */
			if (bytestochecksum)
			{
				MD5Update(&special_md5, buffer, bytestochecksum);
				sha1_update(&special_sha1, bytestochecksum, buffer);
				special_bytes_checksummed += bytestochecksum;
			}
		}

		/* if we were supposed to read past the end, indicate the poper number of bytes */
		if (offset + result > special_logicalbytes)
			result = special_logicalbytes - offset;
		return result;
	}

	/* otherwise, do it normally */
	else
	{
#ifdef _WIN32
		LONG upperPos = offset >> 32;
		DWORD result;

		/* attempt to seek to the new location */
		result = SetFilePointer((HANDLE)file, (UINT32)offset, &upperPos, FILE_BEGIN);
		if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return 0;

		/* do the read */
		if (ReadFile((HANDLE)file, buffer, count, &result, NULL))
			return result;
		else
			return 0;
#else
		{
			int rv;

			fseek((FILE *)file, offset, SEEK_SET);
			rv = fread(buffer, 1, count, (FILE *)file);

			return rv;
		}
#endif
	}
}



/*-------------------------------------------------
	chdman_write - write to an offset
-------------------------------------------------*/

static UINT32 chdman_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	/* if it's the special chd handle, this is bad */
	if (file == (struct chd_interface_file *)special_chd)
	{
		printf("Error: chdman write to CHD image = bad!\n");
		return 0;
	}

	/* otherwise, do it normally */
	else
	{
#ifdef _WIN32
		LONG upperPos = offset >> 32;
		DWORD result;

		/* attempt to seek to the new location */
		result = SetFilePointer((HANDLE)file, (UINT32)offset, &upperPos, FILE_BEGIN);
		if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return 0;

		/* do the read */
		if (WriteFile((HANDLE)file, buffer, count, &result, NULL))
			return result;
		else
			return 0;
#else
		fseek((FILE *)file, offset, SEEK_SET);
		return fwrite(buffer, 1, count, (FILE *)file);
#endif
	}
}



/*-------------------------------------------------
	chdman_length - return the current EOF
-------------------------------------------------*/

static UINT64 chdman_length(struct chd_interface_file *file)
{
	/* if it's the special chd handle, this is bad */
	if (file == (struct chd_interface_file *)special_chd)
		return special_logicalbytes;

	/* otherwise, do it normally */
	else
	{
#ifdef _WIN32
		DWORD highSize = 0, lowSize;
		UINT64 filesize;

		/* get the file size */
		lowSize = GetFileSize((HANDLE)file, &highSize);
		filesize = lowSize | ((UINT64)highSize << 32);
		if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
			filesize = 0;
		return filesize;
#else
		size_t oldpos = ftell((FILE *)file);
		size_t filesize;

		/* get the size */
		fseek((FILE *)file, 0, SEEK_END);
		filesize = ftell((FILE *)file);
		fseek((FILE *)file, oldpos, SEEK_SET);

		return filesize;
#endif
	}
}

// return the CHD handle for a disk
struct chd_file *rom_get_disk_handle(int disk)
{
	return disks[disk];
}
