#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libgen.h>

#define BLOCK_SIZE 4096
#define MAX_FILE_NAME 255

#define FREE 0
#define TAKEN 1

struct DiscBasic
{
    unsigned int discSize;
    unsigned int blockCount;
    unsigned int firstBlockAddress;
    unsigned int memoryMapAddress;

} discBasic;

struct FileDescriptor
{
    char fileName[MAX_FILE_NAME];
    unsigned int descriptorState;
    unsigned int firstBlockAddress;
    unsigned int nextFileDescriptorAddress;
    unsigned int fileDescriptorAddress;
} fileDescriptor;

struct Block
{
    unsigned int state;
    unsigned int nextBlockAddress;
    char data[BLOCK_SIZE];
} block;

int createDisc(char *discName, int discSize)
{
    int blockCount, fileDescriptorsSize, memoryMapSize, basicInfoSize, descriptorTableSize, blocksSize;
    int firstBlockAddress, memoryMapAddress, mapElement, i;
    FILE *drive;

    blockCount = (discSize / BLOCK_SIZE) + 1;

    // fileDescriptorsSize = blockCount * sizeof(int); /* max number of files is equal to number of blocks*/
    // memoryMapSize = blockCount * sizeof(int);
    // descriptorTableSize = blockCount * sizeof(struct FileDescriptor);
    // blocksSize = blockCount * sizeof(struct Block);
    // basicInfoSize = sizeof(struct DiscBasic);
    memoryMapAddress = sizeof(struct DiscBasic);
    firstBlockAddress = memoryMapAddress + blockCount * sizeof(int);

    struct DiscBasic db = {
        discSize,
        blockCount,
        firstBlockAddress,
        memoryMapAddress};

    struct FileDescriptor emptyDescriptor = {
        "",
        FREE,
        0,
        0,
        0};

    struct Block emptyBlock = {
        FREE,
        0,
        ""};

    drive = fopen(discName, "wb");
    if (drive == NULL)
    {
        printf("Error creating disc\n");
        return 1;
    }

    /* save basic*/
    if (fwrite(&db, sizeof(struct DiscBasic), 1, drive))
    {
        printf("Written discbasic succesfully\n");
    }
    else
    {
        printf("Failed to write superblock\n");
        fclose(drive);
        remove(discName);
        return 1;
    }

    /* save memory map */
    mapElement = FREE;
    for (i = 0; i < blockCount; i++)
    {
        if (fwrite(&mapElement, sizeof(int), 1, drive))
        {
            printf("Written map element succesfully\n");
        }
        else
        {
            printf("Failed to write block\n");
            fclose(drive);
            remove(discName);
            return 1;
        }
    }

    /* save file descriptors */
    for (i = 0; i < blockCount; i++)
    {
        if (fwrite(&fileDescriptor, sizeof(struct FileDescriptor), 1, drive))
        {
            printf("Written descriptor succesfully\n");
        }
        else
        {
            printf("Failed to write block\n");
            fclose(drive);
            remove(discName);
            return 1;
        }
    }

    /* save blocks */
    for (i = 0; i < blockCount; i++)
    {
        if (fwrite(&emptyBlock, sizeof(struct Block), 1, drive))
        {
            printf("Written block succesfully\n");
        }
        else
        {
            printf("Failed to write block\n");
            fclose(drive);
            remove(discName);
            return 1;
        }
    }
}

int deleteDisc(char *discName)
{
    return remove(discName);
}

struct DiscBasic loadDiscBasic(char *discName)
{
    FILE *drive;
    struct DiscBasic db;

    drive = fopen(discName, "rb");
    if (drive == NULL)
    {
        printf("Error opening disc\n");
        exit(1);
    }

    if (!fread(&db, sizeof(struct DiscBasic), 1, drive))
    {
        printf("Failed to read superblock\n");
        fclose(drive);
        exit(1);
    }

    fclose(drive);
    return db;
}

int isBlockTaken(char *discName, int blockAddress)
{
    struct DiscBasic db = loadDiscBasic(discName);
    FILE *drive;
    int mapElement;

    drive = fopen(discName, "rb");
    if (drive == NULL)
    {
        printf("Error opening disc\n");
        exit(1);
    }

    fseek(drive, db.memoryMapAddress + (blockAddress - 1) * sizeof(int), SEEK_SET);
    if (fread(&mapElement, sizeof(int), 1, drive))
    {
        printf("Read map element succesfully\n");
        printf("taken: %d\n", mapElement);
    }
    else
    {
        printf("Failed to read map element\n");
        fclose(drive);
        exit(1);
    }

    fclose(drive);
    return mapElement;
}

struct FileDescriptor getFileDescriptor(char *fileName, char *discName)
{
    struct DiscBasic db = loadDiscBasic(discName);
    FILE *drive = fopen(discName, "rb");
    if (!drive)
        exit(-1);

    fseek(drive, db.memoryMapAddress + db.blockCount * sizeof(int), SEEK_SET);

    for (int i = 0; i < db.blockCount; i++)
    {
        struct FileDescriptor fd;
        if (fread(&fd, sizeof(fd), 1, drive) != 1)
            break;
        if (!strcmp(fd.fileName, fileName) && fd.descriptorState == TAKEN)
        {
            fclose(drive);
            return fd;
        }
    }

    fclose(drive);
    printf("file not found\n");
    exit(-1);
}

int performLsCommand(char *discName)
{
    struct DiscBasic db = loadDiscBasic(discName);
    FILE *drive = fopen(discName, "rb");
    if (!drive)
        exit(-1);

    fseek(drive, db.memoryMapAddress + db.blockCount * sizeof(int), SEEK_SET);

    for (int i = 0; i < db.blockCount; i++)
    {
        struct FileDescriptor fd;
        if (fread(&fd, sizeof(fd), 1, drive) != 1)
            break;
        if (fd.descriptorState == TAKEN)
        {
            printf("name: %s first block address: %u\n", fd.fileName, fd.firstBlockAddress);
        }
    }

    fclose(drive);
    return 0;
}

int rmFile(char *fileName, char *discName)
{
    struct FileDescriptor fd = getFileDescriptor(fileName, discName);
    struct DiscBasic db = loadDiscBasic(discName);

    FILE *f = fopen(discName, "rb+");
    if (!f)
        return -1;

    /* change all blocks nad memory map to free */
    unsigned currentBlock = fd.firstBlockAddress;
    while (currentBlock)
    {
        fseek(f, db.memoryMapAddress + (currentBlock - 1) * sizeof(int), SEEK_SET);
        int freeVal = FREE;
        fwrite(&freeVal, sizeof(int), 1, f);

        fseek(f, db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block), SEEK_SET);
        struct Block blk;
        fread(&blk, sizeof(blk), 1, f);
        currentBlock = blk.nextBlockAddress;
    }
    fclose(f);

    /* change file descriptor to free */
    f = fopen(discName, "rb+");
    fseek(f, db.memoryMapAddress + db.blockCount * sizeof(int) + fd.fileDescriptorAddress * sizeof(struct FileDescriptor), SEEK_SET);
    fd.descriptorState = FREE;
    fwrite(&fd, sizeof(fd), 1, f);
    fclose(f);
    return 0;
}

int cpToDisc(char *fileName, char *discName)

{
    /* open local file to read*/
    FILE *src = fopen(fileName, "rb");
    if (!src)
        return -1;

    struct DiscBasic db = loadDiscBasic(discName);
    /* open disc */
    FILE *disc = fopen(discName, "rb+");
    if (!disc)
    {
        fclose(src);
        return -1;
    }

    /* check if file does not exist on disc*/
    for (int i = 0; i < (int)db.blockCount; i++)
    {
        struct FileDescriptor fd;
        fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int) + i * sizeof(struct FileDescriptor), SEEK_SET);
        if (fread(&fd, sizeof(fd), 1, disc) != 1)
            break;
        if (!strcmp(fd.fileName, fileName) && fd.descriptorState == TAKEN)
        {
            printf("File already exists on disc\n");
            fclose(disc);
            fclose(src);
            return -1;
        }
    }

    /*Find free descriptor*/
    fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int), SEEK_SET);
    int descriptorIndex = -1;
    for (int i = 0; i < (int)db.blockCount; i++)
    {
        struct FileDescriptor tmp;
        long pos = ftell(disc);
        if (fread(&tmp, sizeof(tmp), 1, disc) != 1)
            break;
        if (tmp.descriptorState == FREE)
        {
            descriptorIndex = i;
            fseek(disc, pos, SEEK_SET);
            memset(&tmp, 0, sizeof(tmp));
            strncpy(tmp.fileName, fileName, MAX_FILE_NAME - 1);
            tmp.descriptorState = TAKEN;
            tmp.fileDescriptorAddress = i;
            fwrite(&tmp, sizeof(tmp), 1, disc);
            break;
        }
    }
    if (descriptorIndex < 0) /* failed to find free descriptor*/
    {
        printf("failed to find free descriptor");
        fclose(disc);
        fclose(src);
        return -1;
    }

    /* at this point free descriptor was found */

    /*Fetch descriptor and set up linking*/
    fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int) + descriptorIndex * sizeof(struct FileDescriptor), SEEK_SET);
    struct FileDescriptor fd;
    fread(&fd, sizeof(fd), 1, disc);
    fd.firstBlockAddress = 0;

    unsigned prevBlock = 0, currentBlock = 0;
    size_t bytesRead;
    char buffer[BLOCK_SIZE];

    /*Read local file in chunks and store them*/
    while ((bytesRead = fread(buffer, 1, BLOCK_SIZE, src)) > 0)
    {
        // Find free block
        int freeBlock = -1;
        fseek(disc, db.memoryMapAddress, SEEK_SET);
        for (int i = 0; i < (int)db.blockCount; i++)
        {
            int status;
            long mapPos = ftell(disc);
            if (fread(&status, sizeof(status), 1, disc) != 1)
                break;
            if (status == FREE)
            {
                freeBlock = i + 1;
                fseek(disc, mapPos, SEEK_SET);
                status = TAKEN;
                fwrite(&status, sizeof(status), 1, disc);
                break;
            }
        }
        if (freeBlock < 0)
            break;
        // Update the file descriptor's first block if needed
        if (fd.firstBlockAddress == 0)
        {
            fd.firstBlockAddress = freeBlock;
        }

        // Link blocks
        if (prevBlock != 0)
        {
            fseek(disc, db.firstBlockAddress + prevBlock * sizeof(struct Block), SEEK_SET);
            struct Block pb;
            fread(&pb, sizeof(pb), 1, disc);
            pb.nextBlockAddress = freeBlock;
            fseek(disc, -((long)sizeof(pb)), SEEK_CUR);
            fwrite(&pb, sizeof(pb), 1, disc);
        }
        prevBlock = freeBlock;
        currentBlock = freeBlock;

        // Write data into the block
        struct Block blk;
        memset(&blk, 0, sizeof(blk));
        blk.state = TAKEN;
        blk.nextBlockAddress = 0;
        memcpy(blk.data, buffer, bytesRead);
        fseek(disc, db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block), SEEK_SET);
        fwrite(&blk, sizeof(blk), 1, disc);

        int address = db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block);
        printf("block of data written at: %d \n", address);
        printf("first block address: %d\n", db.firstBlockAddress);
        printf("current block address %d\n", currentBlock);
    }

    // Update descriptor
    fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int) + descriptorIndex * sizeof(struct FileDescriptor), SEEK_SET);
    fwrite(&fd, sizeof(fd), 1, disc);

    fclose(disc);
    fclose(src);
    return 0;
}

int cpFromDisc(char *fileName, char *discName)
{

    struct FileDescriptor fd = getFileDescriptor(fileName, discName);
    struct DiscBasic db = loadDiscBasic(discName);

    FILE *outFile = fopen(fileName, "wb");
    if (!outFile)
        return -1;
    FILE *disc = fopen(discName, "rb");
    if (!disc)
    {
        fclose(outFile);
        return -1;
    }

    unsigned currentBlock = fd.firstBlockAddress;
    while (currentBlock)
    {
        fseek(disc, db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block), SEEK_SET);
        struct Block blk;
        if (fread(&blk, sizeof(blk), 1, disc) != 1)
            break;
        fwrite(blk.data, 1, strnlen(blk.data, BLOCK_SIZE), outFile);
        currentBlock = blk.nextBlockAddress;
    }

    fclose(disc);
    fclose(outFile);
    return 0;
}

int printTakenSpace(char *discName)
{
    struct DiscBasic db = loadDiscBasic(discName);
    FILE *disc = fopen(discName, "rb");
    if (!disc)
    {
        return -1;
    }
    int takenCount = 0, mapVal;
    fseek(disc, db.memoryMapAddress, SEEK_SET);
    for (int i = 0; i < db.blockCount; i++)
    {
        if (fread(&mapVal, sizeof(mapVal), 1, disc) != 1)
        {
            fclose(disc);
            return -1;
        }
        if (mapVal == TAKEN)
        {
            takenCount++;
        }
    }
    fclose(disc);
    printf("%d\n", takenCount * BLOCK_SIZE);
    return 0;
}

int printMemoryMap(char *discName)
{
    struct DiscBasic db = loadDiscBasic(discName);
    FILE *drive = fopen(discName, "rb");
    if (!drive)
    {
        printf("Error opening disc\n");
        return -1;
    }

    printf("Memory Map:\n");
    for (int i = 0; i < db.blockCount; i++)
    {
        int status;
        fseek(drive, db.memoryMapAddress + i * sizeof(int), SEEK_SET);
        if (fread(&status, sizeof(status), 1, drive) != 1)
        {
            printf("Failed to read map element at block %d\n", i);
            fclose(drive);
            return -1;
        }
        printf("Block %d: %s\n", i + 1, status == TAKEN ? "TAKEN" : "FREE");
    }

    fclose(drive);
    return 0;
}