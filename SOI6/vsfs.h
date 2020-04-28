
#ifndef VERY_SIMPLE_FILE_SYSTEM_VSFS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"

#define VERY_SIMPLE_FILE_SYSTEM_VSFS_H
#define BLOCK_SIZE 2048
#define BLOCKS_AMOUNT 64
#define MAX_FILE_NAME 64
#define DISK_NAME "Disk.vsfs"
#define DISK_NAME_LENGTH 11
#define INODE_LENGTH 128
#define INODE_AMOUNT 80

typedef struct SuperBlock
{
    char diskName[DISK_NAME_LENGTH];

    short superBlock_Offset;
    short inodeBitMap_Offset;
    short dataBlocksBitMap_Offset;
    short inodeTable_Offset;
    short dataBlocks_Offset;

} SuperBlock;

typedef struct Inode
{
    char fileName[MAX_FILE_NAME];
    unsigned short firstBlock;
    unsigned short lengthInBlocks;
    unsigned long lengthInBytes;
} Inode;

SuperBlock* ptr_to_superBlock;

int createDisk();
int addFile(char fileName[]);
int removeDisk();
int getOccupiedInodesIds(int tab[]);
int findInodeOfFileOnVSFS(char fileName[]);
int getEmptyBlockID(int neededBlocks);
unsigned long getFileSize(char fileName[], int* result);
int createNewInode(char fileName[], unsigned short firstBlock, unsigned short lengthInBlocks, unsigned long fileSize);
int setOccupiedBitsInBlocksBitMap(unsigned short blockID, unsigned short neededBlocks);
int viewCatalogue();
int deleteFile(char fileName[]);
int getFile(char fileName[]);
void viewDiskMap();
int getBlockUsagePercentage(int blockID);
char* getBlockTypeName(int blockID);
int loadDiskSB();
#endif //VERY_SIMPLE_FILE_SYSTEM_VSFS_H
