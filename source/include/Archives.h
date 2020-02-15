/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ----------------------------------------------------------------------------------------#
#------- _  -------------------  ______   _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \ | |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____    _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/ | | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |      | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|      |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/  -------------------------------------------------------------#
#------------------------   ______   _   -------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  -------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   ------------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  ------------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  ------------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   ------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License --------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <cusunanni@hotmail.it> --------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@eternalongju2.com> -----------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Official Forum : http://rinnegatamante.eu/luaplayerplus/forum.php ---------------------------------------------------#
#- For help using LuaPlayerPlus, code help, and other please visit : http://rinnegatamante.eu/luaplayerplus/forum.php --#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Homemister for LPHM sourcecode --------------------------------------------------------------------------------------#
#- Zack & Shine for LP Euphoria sourcecode -----------------------------------------------------------------------------#
#- ab5000 for support on psp-ita.com -----------------------------------------------------------------------------------#
#- valantin for sceIoMvdir and sceIoCpdir improved functions------------------------------------------------------------#
#- Dark_AleX for usbdevice ---------------------------------------------------------------------------------------------#
#- VirtuosFlame & ColdBird for iso drivers and kuBridge ----------------------------------------------------------------#
#- sakya for Media Engine and OslibMod ---------------------------------------------------------------------------------#
#- Booster & silverspring for EEPROM write/read functions --------------------------------------------------------------#
#- Akind for RemoteJoyLite ---------------------------------------------------------------------------------------------#
#- cooleyes for mpeg4 lib ----------------------------------------------------------------------------------------------#
#- Arshia001 for PSPAALib ----------------------------------------------------------------------------------------------#
#- InsertWittyName & MK2k for PGE sourcecode ---------------------------------------------------------------------------#
#- Youresam for LUA BMPLib ---------------------------------------------------------------------------------------------#
#- Raphael for vram manager code ---------------------------------------------------------------------------------------#
#- Dynodzzo for LSD concepts -------------------------------------------------------------------------------------------#
#- ab_portugal for Image.negative function -----------------------------------------------------------------------------#
#- JiC� for drawCircle function ----------------------------------------------------------------------------------------#
#- Rapper_skull & DarkGiovy for testing LuaPlayer Plus and coming up with some neat ideas for it. ----------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#ifndef __ARCHIVES_H_
#define __ARCHIVES_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ** UnZip *********************************************************************
 *******************************************************************************/

/** @defgroup Zip Zip Library
 *  @{
 */

#include "zlib.h"
#include <stdio.h>


typedef struct
{
	unsigned long version;
	unsigned long versionneeded;
	unsigned long flag;
	unsigned long compressionmethod;
	unsigned long dosdate;
	unsigned long crc;
	unsigned long compressedsize;
	unsigned long uncompressedsize;
	unsigned long filenamesize;
	unsigned long fileextrasize;
	unsigned long filecommentsize;
	unsigned long disknumstart;
	unsigned long internalfileattr;
	unsigned long externalfileattr;

} zipFileInfo;

typedef struct
{
	unsigned long currentfileoffset;

} zipFileInternalInfo;

typedef struct
{
	char *buffer;
	z_stream stream;
	unsigned long posinzip;
	unsigned long streaminitialised;
	unsigned long localextrafieldoffset;
	unsigned int  localextrafieldsize;
	unsigned long localextrafieldpos;
	unsigned long crc32;
	unsigned long crc32wait;
	unsigned long restreadcompressed;
	unsigned long restreaduncompressed;
	FILE* file;
	unsigned long compressionmethod;
	unsigned long bytebeforezip;

} zipFileInfoInternal;

typedef struct
{
	unsigned long countentries;
	unsigned long commentsize;

} zipGlobalInfo;

typedef struct
{
	FILE* file;
	zipGlobalInfo gi;
	unsigned long bytebeforezip;
	unsigned long numfile;
	unsigned long posincentraldir;
	unsigned long currentfileok;
	unsigned long centralpos;
	unsigned long centraldirsize;
	unsigned long centraldiroffset;
	zipFileInfo currentfileinfo;
	zipFileInternalInfo currentfileinfointernal;
	zipFileInfoInternal* currentzipfileinfo;
	int encrypted;
	unsigned long keys[3];
	const unsigned long* crc32tab;

} _zip;



/**
 * A zip
 */
typedef void Zip;

typedef struct
{
	char* filename;
	unsigned long size;
	void *next;
} ZipFileList;

/**
 * A file within a zip
 */
typedef struct
{
	unsigned char *data;	/**<  The file data */
	int size;				/**<  Size of the data */

} ZipFile;

/**
 * Open a Zip file
 *
 * @param filename - Path of the zip to load.
 *
 * @returns A pointer to a ::Zip struct or NULL on error.
 */
Zip* ZipOpen(const char *filename);

/**
 * Close a Zip file
 *
 * @param zip - A valid (previously opened) ::Zip
 *
 * @returns 1 on success, 0 on error
 */
int ZipClose(Zip *zip);

/**
 * Read a file from a zip
 *
 * @param zip - A valid (previously opened) ::Zip
 *
 * @param filename - The file to read within the zip
 *
 * @param password - The password of the file (pass NULL if no password)
 *
 * @returns A ::ZipFile struct containing the file
 */
ZipFile* ZipFileRead(Zip *zip, const char *filename, const char *password);

int ZitCurrentFileInfo(Zip* file, zipFileInfo *pfileinfo, char *filename, unsigned long filenamebuffersize, void *extrafield, unsigned long extrafieldbuffersize, char *comment, unsigned long commentbuffersize);

int ZitGlobalInfo(Zip* file, zipGlobalInfo *zipinfo);

int ZipGotoNextFile(Zip* file);

/**
 * Extract all files from a zip
 *
 * @param zip - A valid (previously opened) ::Zip file
 *
 * @param password - The password of the file (pass NULL if no password)
 *
 * @param path - Path where to extract files
 *
 * @returns 1 on success, 0 on error.
 */
int ZipExtract(Zip *zip, const char *password, const char* path);

/**
 * Free the file data previously loaded from a zip
 *
 * @param file - A valid (previously read) ::ZipFile
 */
void ZipFileFree(ZipFile *file);

/**
 * Get list all files from a zip
 *
 * @param zip - A valid (previously opened) ::Zip file
 * 
 * @param fileList - Linked list of pathes for files inside zip file
 * 
 * @returns 1 on success, 0 on error.
 */
int ZipList(Zip *zip, ZipFileList *fileList);

#ifdef __cplusplus
}
#endif

#endif /* __ARCHIVES_H_ */

/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------*/
