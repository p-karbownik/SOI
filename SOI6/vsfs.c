
#include "vsfs.h"

int createDisk()
{
    FILE* ptr_to_file = fopen(DISK_NAME, "w");

    if(ptr_to_file == NULL)
        return DISK_OPEN_ERROR;

    char char_buffer[] = { 0x00u };
    short short_buffer[5];

    for(int i = 0; i < BLOCK_SIZE * BLOCKS_AMOUNT; i++)
        fwrite(char_buffer, sizeof(char), 1, ptr_to_file);

    ptr_to_superBlock = malloc(sizeof(SuperBlock));

    if(ptr_to_superBlock == NULL)
    {
        fclose(ptr_to_file);
        return SUPER_BLOCK_ERROR;
    }
    strcpy(ptr_to_superBlock->diskName, DISK_NAME);
    short_buffer[0] = ptr_to_superBlock->superBlock_Offset = 0;
    short_buffer[1] = ptr_to_superBlock->inodeBitMap_Offset = 1;
    short_buffer[2] = ptr_to_superBlock->dataBlocksBitMap_Offset = 2;
    short_buffer[3] = ptr_to_superBlock->inodeTable_Offset = 3;
    short_buffer[4] = ptr_to_superBlock->dataBlocks_Offset = 8;

    fseek(ptr_to_file, 0, SEEK_SET);

    fwrite(ptr_to_superBlock->diskName, sizeof(char), strlen(DISK_NAME) + 1, ptr_to_file);
    fwrite(short_buffer, sizeof(short), 5, ptr_to_file);

    fclose(ptr_to_file);

    return 0;
}

int addFile(char fileName[])
{
    int result = 0;
    unsigned long fileSize = getFileSize(fileName, &result);

    unsigned short neededBlocks = (fileSize / BLOCK_SIZE) + (fileSize % 8);

    FILE* ptr_to_file;
    FILE* ptr_to_disk;

    if(result == -1)
        return FILE_OPEN_ERROR; //CANNOT OPEN FILE TO COPY

    result = findInodeOfFileOnVSFS(fileName);

    if(result == -2)
        return DISK_OPEN_ERROR; //CANNOT OPEN DISK FILE

    if(result != -1)
        return NOT_UNIQUE_FILE_NAME; //FILE NAME IS NOT UNIQUE

    int blockID = getEmptyBlockID(neededBlocks);

    if(blockID == -1)
        return NOT_ENOUGH_SPACE; //NOT ENOUGH SPACE

    if(blockID == -2)
        return FILE_OPEN_ERROR;

    result = createNewInode(fileName, (unsigned short) blockID, neededBlocks, fileSize);

    if(result == -1)
        return NEW_INODE_CREATION_ERROR; // CANNOT ADD NEW INODE

    result = setOccupiedBitsInBlocksBitMap((unsigned short) blockID, neededBlocks);

    if(result == -1)
        return NOT_ENOUGH_SPACE;

    ptr_to_file = fopen(fileName, "rb");
    ptr_to_disk = fopen(DISK_NAME, "r+b");

    if(ptr_to_file == NULL)
        return FILE_OPEN_ERROR;

    if(ptr_to_disk == NULL)
        return DISK_OPEN_ERROR;

    char buffer[1];

    fseek(ptr_to_disk, (ptr_to_superBlock->dataBlocks_Offset + blockID) * BLOCK_SIZE, SEEK_SET);

    while(ftell(ptr_to_file) < fileSize)
    {
        fread(buffer, sizeof(char), 1, ptr_to_file);
        fwrite(buffer, sizeof(char), 1, ptr_to_disk);
    }

    fclose(ptr_to_file);
    fclose(ptr_to_disk);

    return 0;
}

int removeDisk()
{
    int result = remove(DISK_NAME);

    if(result == 0)
    {
        if(ptr_to_superBlock != NULL)
        {free(ptr_to_superBlock); ptr_to_superBlock = NULL;}
        return result;
    }
    else
        return result;
}

int getOccupiedInodesIds(int tab[])
{
    FILE* ptr_to_disk = fopen(DISK_NAME, "r+b)");
    unsigned int bufferSize = INODE_AMOUNT / 8 + ((INODE_AMOUNT % 8) ? 1 : 0);
    unsigned char bitsArray[] = {0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u};

    unsigned char buffer[bufferSize];
    tab[0] = 0;

    if(ptr_to_disk == NULL)
        return -1;

    fseek(ptr_to_disk, ptr_to_superBlock->inodeBitMap_Offset * BLOCK_SIZE, SEEK_SET);
    fread(buffer, sizeof(unsigned char), bufferSize, ptr_to_disk);

    for(int i = 0; i < bufferSize; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            if(buffer[i] & bitsArray[j])
            {
                tab[0] = tab[0] + 1;
                tab[tab[0]] = i * 8 + j;
            }
        }
    }

    fclose(ptr_to_disk);

    return 0;
}

int findInodeOfFileOnVSFS(char fileName[])
{
    FILE* ptr_to_disk;
    int inodeNumber = -1;
    int inodesToCheck[INODE_AMOUNT + 1];
    char buffer[MAX_FILE_NAME];

    if(getOccupiedInodesIds(inodesToCheck) == -1)
        return -2;

    ptr_to_disk = fopen(DISK_NAME, "rb");

    if(ptr_to_disk == NULL)
        return -2;

    for(int i = 1; i <= inodesToCheck[0]; i++)
    {
        fseek(ptr_to_disk, ptr_to_superBlock->inodeTable_Offset * BLOCK_SIZE + inodesToCheck[i] * INODE_LENGTH, SEEK_SET);

        fread(buffer, sizeof(char), MAX_FILE_NAME, ptr_to_disk);

        if(strcmp(fileName, buffer) == 0)
        {
            inodeNumber = inodesToCheck[i];
            break;
        }
    }

    fclose(ptr_to_disk);

    return inodeNumber;
}

int getEmptyBlockID(int neededBlocks)
{
    int emptyBlocks = 0;
    int maxFreeSpaceLength = 0;
    int firstEmptyBlockOffSet = -1;
    unsigned short bufferSize = (BLOCKS_AMOUNT / 8) + (BLOCKS_AMOUNT % 8) ;
    char buffer[bufferSize];
    unsigned char bitsArray[] = {0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u};

    FILE* ptr_to_disk = fopen(DISK_NAME, "r+b");

    if (ptr_to_disk == NULL)
        return -2;

    fseek(ptr_to_disk, ptr_to_superBlock->dataBlocksBitMap_Offset * BLOCK_SIZE, SEEK_SET);

    fread(buffer, sizeof(char), bufferSize, ptr_to_disk);

    for(int i = 0; i < bufferSize; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            if((buffer[i] & bitsArray[j]) == bitsArray[j])
            {
                firstEmptyBlockOffSet = -1;
                maxFreeSpaceLength = 0;
            }

            else
            {
                if(firstEmptyBlockOffSet == -1)
                    firstEmptyBlockOffSet = 8 * i + j;

                emptyBlocks++;
                maxFreeSpaceLength++;

                if(maxFreeSpaceLength >= neededBlocks)
                    break;
            }
        }
    }

    fclose(ptr_to_disk);

    return firstEmptyBlockOffSet;
}

unsigned long getFileSize(char fileName[], int* result)
{
    FILE* ptr_to_file = fopen(fileName, "r+b");
    unsigned long length = 0;

    if(ptr_to_file == NULL)
    {
        *result = -1;
        return length;
    }

    fseek(ptr_to_file, 0, SEEK_END);

    length = ftell(ptr_to_file);
    *result = 0;

    return length;
}

int createNewInode(char fileName[], unsigned short firstBlock, unsigned short lengthInBlocks, unsigned long fileSize)
{
    FILE* ptr_to_disk = fopen(DISK_NAME, "r+b");

    if(ptr_to_disk == NULL)
        return -1;

    int emptyInode = -1;
    int emptyInodeFounded = 0;

    unsigned int charBufferSize = (INODE_AMOUNT / 8) + ((INODE_AMOUNT % 8) ? 1 : 0 );
    unsigned short u_shortBuffer[2];
    unsigned long u_longBuffer[1];
    char charBuffer[charBufferSize];
    unsigned char bitsArray[] = {0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u};

    u_shortBuffer[0] = firstBlock;
    u_shortBuffer[1] = lengthInBlocks;
    u_longBuffer[0] = fileSize;

    fseek(ptr_to_disk, ptr_to_superBlock->inodeBitMap_Offset * BLOCK_SIZE, SEEK_SET);
    fread(charBuffer, sizeof(char), charBufferSize, ptr_to_disk);

    for(int i = 0; i < charBufferSize; i++)
    {
        int j = 0;

        for(j = 0; j < 8; j++)
        {
            if((charBuffer[i] & bitsArray[j]) != bitsArray[j])
            {
                emptyInode = 8 * i + j;

                emptyInodeFounded = 1;

                break;
            }
        }

        if(emptyInodeFounded)
        {
            fseek(ptr_to_disk, ptr_to_superBlock->inodeBitMap_Offset * BLOCK_SIZE + (i / 8), SEEK_SET);
            fread(charBuffer, sizeof(char), 1, ptr_to_disk);
            fseek(ptr_to_disk, -1, SEEK_CUR);
            charBuffer[0] = charBuffer[0] | bitsArray[j];
            fwrite(charBuffer, sizeof(char), 1, ptr_to_disk);
            break;
        }
    }

    if(!emptyInodeFounded)
    {
        fclose(ptr_to_disk);
        return -2;
    }

    fseek(ptr_to_disk, ptr_to_superBlock->inodeTable_Offset * BLOCK_SIZE + emptyInode * INODE_LENGTH, SEEK_SET);

    fwrite(fileName, sizeof(char), MAX_FILE_NAME, ptr_to_disk);
    fwrite(u_shortBuffer, sizeof(unsigned short), 2, ptr_to_disk);
    fwrite(u_longBuffer, sizeof(unsigned long), 1, ptr_to_disk);

    fclose(ptr_to_disk);
    return 0;
}

int setOccupiedBitsInBlocksBitMap(unsigned short blockID, unsigned short neededBlocks)
{
    FILE* ptr_to_disk = fopen(DISK_NAME, "r+b");

    unsigned char bitsArray[] = {0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u};
    int k = 0;
    int j = blockID % 8;
    char buffer[1];

    if(ptr_to_disk == NULL)
        return -1;

    fseek(ptr_to_disk, ptr_to_superBlock->dataBlocksBitMap_Offset * BLOCK_SIZE + (blockID / 8), SEEK_SET);

    while(k <= neededBlocks)
    {
        for(; j < 8; j++)
        {
            fread(buffer, sizeof(char), 1, ptr_to_disk);
            buffer[0] = buffer[0] | bitsArray[j];

            fseek(ptr_to_disk, -1, SEEK_CUR);
            fwrite(buffer, sizeof(char), 1, ptr_to_disk);
            if(j < 7)
                fseek(ptr_to_disk, -1, SEEK_CUR);
            k++;
        }
        j = 0;
    }

    fclose(ptr_to_disk);

    return 0;
}

int viewCatalogue()
{
    int inodesToCheck[INODE_AMOUNT + 1];
    FILE* ptr_to_disk;
    char fileNames[INODE_AMOUNT][MAX_FILE_NAME];
    if(getOccupiedInodesIds(inodesToCheck) == -1)
        return DISK_OPEN_ERROR;

    ptr_to_disk = fopen(DISK_NAME, "rb");

    if(ptr_to_disk == NULL)
        return DISK_OPEN_ERROR;

    int j = 0;

    for(int i = 1; i <= inodesToCheck[0]; i++)
    {
        fseek(ptr_to_disk, ptr_to_superBlock->inodeTable_Offset * BLOCK_SIZE + inodesToCheck[i] * INODE_LENGTH, SEEK_SET);
        fread(fileNames[j], sizeof(char), MAX_FILE_NAME, ptr_to_disk);

        j++;
    }

    fclose(ptr_to_disk);

    printf("Files in catalogue:\n");

    for(int i = 0; i < j; i++)
        printf("%d -> %s\n", i, fileNames[i]);

    return 0;
}

int deleteFile(char fileName[])
{
    int inodeNumber = findInodeOfFileOnVSFS(fileName);
    if(inodeNumber < 0)
        return CANNOT_FIND_FILE;
    FILE* ptr_to_disk = fopen(DISK_NAME, "r+b");

    char charBuffer[MAX_FILE_NAME];
    unsigned u_shortBuffer[2];
    unsigned long u_longBugger[1];
    char zero[] = {0x00u};
    unsigned char bitsArray[] = {0x7fu, 0xbfu, 0xdfu, 0xefu, 0xf7u, 0xfbu, 0xfdu, 0xfeu}; 

    if(ptr_to_disk == NULL)
        return DISK_OPEN_ERROR;

    fseek(ptr_to_disk, (ptr_to_superBlock->inodeTable_Offset * BLOCK_SIZE) + (inodeNumber * INODE_LENGTH), SEEK_SET);

    fread(charBuffer, sizeof(char), MAX_FILE_NAME, ptr_to_disk);
    fread(u_shortBuffer, sizeof(char), 2, ptr_to_disk);
    fread(u_longBugger, sizeof(unsigned long), 1, ptr_to_disk);

    fseek(ptr_to_disk, -1 * (MAX_FILE_NAME * sizeof(char) + sizeof(unsigned short) * 2 + sizeof(unsigned long)), SEEK_CUR);

    for(int i = 0; i < (MAX_FILE_NAME * sizeof(char) + sizeof(unsigned short) * 2 + sizeof(unsigned long)); i++)
        fwrite(zero, sizeof(char), 1, ptr_to_disk);

    fseek(ptr_to_disk, ptr_to_superBlock->inodeBitMap_Offset * BLOCK_SIZE + inodeNumber / 8, SEEK_SET);

    fread(charBuffer, sizeof(char), 1, ptr_to_disk);
    charBuffer[0] = charBuffer[0] & bitsArray[inodeNumber % 8];

    fseek(ptr_to_disk, -1, SEEK_CUR);
    fwrite(charBuffer, sizeof(char), 1, ptr_to_disk);

    fseek(ptr_to_disk, ptr_to_superBlock->dataBlocksBitMap_Offset * BLOCK_SIZE + u_shortBuffer[0] / 8, SEEK_SET);
    int j = u_shortBuffer[0] % 8;

    for(int i = 0; i < u_shortBuffer[1]; )
    {
        for( ; j < 8; j++)
        {
            fread(charBuffer, sizeof(char), 1, ptr_to_disk);

            charBuffer[0] = charBuffer[0] & bitsArray[j];
            fseek(ptr_to_disk, -1, SEEK_CUR);
            fwrite(charBuffer, sizeof(char), 1, ptr_to_disk);		            	
            fseek(ptr_to_disk, -1, SEEK_CUR);

            i++;
        }
        j = 0;
    }

    fseek(ptr_to_disk, (ptr_to_superBlock->dataBlocks_Offset + u_shortBuffer[0]) * BLOCK_SIZE, SEEK_SET);

    for(int i = 0; i < u_shortBuffer[1]; i++)
        for(j = 0; j < BLOCK_SIZE; j++)
            fwrite(zero, sizeof(char), 1, ptr_to_disk);

    fclose(ptr_to_disk);

    return 0;
}

int getFile(char fileName[])
{
    int inodeNumber = findInodeOfFileOnVSFS(fileName);
    FILE* ptr_to_disk;
    FILE* ptr_to_file;
    char charBuffer[MAX_FILE_NAME];
    unsigned short u_shortBuffer[2];
    unsigned long u_longBuffer[1];
    char newFileName[MAX_FILE_NAME + 5];

    if(inodeNumber < 0)
        return CANNOT_FIND_FILE;

    ptr_to_disk = fopen(DISK_NAME, "r+b");

    if(ptr_to_disk == NULL)
        return DISK_OPEN_ERROR;

    fseek(ptr_to_disk, ptr_to_superBlock->inodeTable_Offset * BLOCK_SIZE + inodeNumber * INODE_LENGTH, SEEK_SET);

    fread(charBuffer, sizeof(char), MAX_FILE_NAME, ptr_to_disk);
    fread(u_shortBuffer, sizeof(unsigned short), 2, ptr_to_disk);
    fread(u_longBuffer, sizeof(unsigned long), 1, ptr_to_disk);

    strcpy(newFileName, "copy");
    strcat(newFileName, fileName);
    ptr_to_file = fopen(newFileName, "wb");

    if(ptr_to_file == NULL)
    {
        fclose(ptr_to_disk);
        return FILE_OPEN_ERROR;
    }

    fseek(ptr_to_disk, (ptr_to_superBlock->dataBlocks_Offset + u_shortBuffer[0]) * BLOCK_SIZE, SEEK_SET);
    for(int i = 0; i < u_longBuffer[0]; i++)
    {
        fread(charBuffer, sizeof(char), 1, ptr_to_disk);
        fwrite(charBuffer, sizeof(char), 1, ptr_to_file);
    }

    fclose(ptr_to_disk);
    fclose(ptr_to_file);

    return 0;
}

void viewDiskMap()
{
    printf("Disk Map\n");
    printf("Block Nr -> Usage [%%] -> Block Type\n");
    for(int i = 0; i < BLOCKS_AMOUNT; i++)
    {
        printf("%d -> %d%% -> %s\n", i, getBlockUsagePercentage(i), getBlockTypeName(i));
    }
}

int getBlockUsagePercentage(int blockID)
{
    FILE* ptr_to_disk = fopen(DISK_NAME, "rb");
    int counter = 0;
    unsigned char buffer[1];
    double temp;
    if(ptr_to_disk == NULL)
        return -1;

    fseek(ptr_to_disk, blockID * BLOCK_SIZE, SEEK_SET);

    for(int i = 0; i < BLOCK_SIZE; i++)
    {
        fread(buffer, sizeof(unsigned char), 1, ptr_to_disk);
        if(buffer[0] != 0x00u)
            counter++;
    }

    fclose(ptr_to_disk);

    temp = counter * 100;
    temp /= BLOCK_SIZE;
    return ((int) temp + ((temp > ((int) temp)) ? 1 : 0));
}

char* getBlockTypeName(int blockID)
{
    ptr_to_superBlock->superBlock_Offset = 0;
    if(blockID == 0)
    {
        return "Super block";
    }
    else if(blockID == ptr_to_superBlock->inodeBitMap_Offset)
    {
        return "Inode BitMap";
    }
    else if(blockID == ptr_to_superBlock->dataBlocksBitMap_Offset)
    {
        return "Data Blocks BitMap";
    }
    else if (blockID >= ptr_to_superBlock->inodeTable_Offset && blockID <= ptr_to_superBlock->dataBlocks_Offset)
    {
        return "Inode Table Offset";
    } else if (blockID >= ptr_to_superBlock->dataBlocks_Offset)
    {
        return "Data Block";
    }
}

int loadDiskSB()
{
    FILE* ptr_to_file = fopen(DISK_NAME, "r");

    if(ptr_to_file == NULL)
        return DISK_OPEN_ERROR;

    char char_buffer[DISK_NAME_LENGTH];
    short short_buffer[5];

    ptr_to_superBlock = malloc(sizeof(SuperBlock));

    if(ptr_to_superBlock == NULL)
    {
        fclose(ptr_to_file);
        return SUPER_BLOCK_ERROR;
    }

    fread(char_buffer, sizeof(char), DISK_NAME_LENGTH - 1, ptr_to_file);

    if(strcmp(DISK_NAME, char_buffer) != 0)
    {
        fclose(ptr_to_file);
        free(ptr_to_superBlock);
        return DISK_OPEN_ERROR;
    }
    fread(short_buffer, sizeof(short), 5, ptr_to_file);

    strcpy(ptr_to_superBlock->diskName, DISK_NAME);
    ptr_to_superBlock->superBlock_Offset = short_buffer[0];
    ptr_to_superBlock->inodeBitMap_Offset = short_buffer[1];
    ptr_to_superBlock->dataBlocksBitMap_Offset = short_buffer[2];
    ptr_to_superBlock->inodeTable_Offset = short_buffer[3];
    ptr_to_superBlock->dataBlocks_Offset = short_buffer[4];

    fseek(ptr_to_file, 0, SEEK_SET);

    fclose(ptr_to_file);

    return 0;
}
