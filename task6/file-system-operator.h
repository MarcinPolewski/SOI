#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define BLOCK_SIZE 4096
#define MAX_FILE_NAME 255

#define FREE 0
#define TAKEN 1

/* Workaround for missing strnlen in older C standards */
static size_t my_strnlen(const char *s, size_t maxlen)
{
    size_t i;
    for (i = 0; i < maxlen && s[i] != '\0'; i++)
        ;
    return i;
}

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
    int blockCount, memoryMapAddress, firstBlockAddress, mapElement, i;
    FILE *drive;
    struct DiscBasic db;
    struct FileDescriptor emptyDescriptor;
    struct Block emptyBlock;

    blockCount = (discSize / BLOCK_SIZE) + 1;

    memoryMapAddress = sizeof(struct DiscBasic);
    firstBlockAddress =
        memoryMapAddress + blockCount * (sizeof(int) + sizeof(struct FileDescriptor));

    db.discSize = discSize;
    db.blockCount = blockCount;
    db.firstBlockAddress = firstBlockAddress;
    db.memoryMapAddress = memoryMapAddress;

    strcpy(emptyDescriptor.fileName, "");
    emptyDescriptor.descriptorState = FREE;
    emptyDescriptor.firstBlockAddress = 0;
    emptyDescriptor.nextFileDescriptorAddress = 0;
    emptyDescriptor.fileDescriptorAddress = 0;

    emptyBlock.state = FREE;
    emptyBlock.nextBlockAddress = 0;
    memset(emptyBlock.data, 0, BLOCK_SIZE);

    drive = fopen(discName, "wb");
    if (drive == NULL)
    {
        printf("Error creating disc\n");
        return 1;
    }

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

    fclose(drive);
    return 0;
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
    struct DiscBasic db;
    FILE *drive;
    int mapElement;

    db = loadDiscBasic(discName);
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
    struct DiscBasic db;
    FILE *drive;
    int i;

    db = loadDiscBasic(discName);
    drive = fopen(discName, "rb");
    if (!drive)
        exit(-1);

    fseek(drive, db.memoryMapAddress + db.blockCount * sizeof(int), SEEK_SET);

    for (i = 0; i < (int)db.blockCount; i++)
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
    struct DiscBasic db;
    FILE *drive;
    int i;

    db = loadDiscBasic(discName);
    drive = fopen(discName, "rb");
    if (!drive)
        exit(-1);

    fseek(drive, db.memoryMapAddress + db.blockCount * sizeof(int), SEEK_SET);

    for (i = 0; i < (int)db.blockCount; i++)
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
    struct FileDescriptor fd;
    struct DiscBasic db;
    FILE *f;
    unsigned currentBlock;

    fd = getFileDescriptor(fileName, discName);
    db = loadDiscBasic(discName);

    f = fopen(discName, "rb+");
    if (!f)
        return -1;

    currentBlock = fd.firstBlockAddress;
    while (currentBlock)
    {
        struct Block blk;
        int freeVal = FREE;
        fseek(f, db.memoryMapAddress + (currentBlock - 1) * sizeof(int), SEEK_SET);
        fwrite(&freeVal, sizeof(int), 1, f);

        fseek(f, db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block), SEEK_SET);
        fread(&blk, sizeof(blk), 1, f);
        currentBlock = blk.nextBlockAddress;
        printf("remove , current block: %d\n", currentBlock);
    }
    fclose(f);

    f = fopen(discName, "rb+");
    fseek(f, db.memoryMapAddress + db.blockCount * sizeof(int) + fd.fileDescriptorAddress * sizeof(struct FileDescriptor),
          SEEK_SET);
    fd.descriptorState = FREE;
    fwrite(&fd, sizeof(fd), 1, f);
    fclose(f);
    return 0;
}

int cpToDisc(char *fileName, char *discName)
{
    FILE *src;
    struct DiscBasic db;
    FILE *disc;
    int i, descriptorIndex;
    struct FileDescriptor fd;
    unsigned prevBlock, currentBlock;

    src = fopen(fileName, "rb");
    if (!src)
        return -1;

    db = loadDiscBasic(discName);
    disc = fopen(discName, "rb+");
    if (!disc)
    {
        fclose(src);
        return -1;
    }

    for (i = 0; i < (int)db.blockCount; i++)
    {
        struct FileDescriptor checkFd;
        fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int) + i * sizeof(struct FileDescriptor),
              SEEK_SET);
        if (fread(&checkFd, sizeof(checkFd), 1, disc) != 1)
            break;
        if (!strcmp(checkFd.fileName, fileName) && checkFd.descriptorState == TAKEN)
        {
            printf("File already exists on disc\n");
            fclose(disc);
            fclose(src);
            return -1;
        }
    }

    fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int), SEEK_SET);
    descriptorIndex = -1;
    for (i = 0; i < (int)db.blockCount; i++)
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
    if (descriptorIndex < 0)
    {
        printf("failed to find free descriptor");
        fclose(disc);
        fclose(src);
        return -1;
    }

    fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int) + descriptorIndex * sizeof(struct FileDescriptor),
          SEEK_SET);
    fread(&fd, sizeof(fd), 1, disc);
    fd.firstBlockAddress = 0;

    prevBlock = 0;
    currentBlock = 0;
    {
        size_t bytesRead;
        char buffer[BLOCK_SIZE];
        while ((bytesRead = fread(buffer, 1, BLOCK_SIZE, src)) > 0)
        {
            int freeBlock = -1;
            fseek(disc, db.memoryMapAddress, SEEK_SET);
            for (i = 0; i < (int)db.blockCount; i++)
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

            if (fd.firstBlockAddress == 0)
            {
                fd.firstBlockAddress = freeBlock;
            }

            if (prevBlock != 0)
            {
                struct Block pb;
                fseek(disc, db.firstBlockAddress + (prevBlock - 1) * sizeof(struct Block),
                      SEEK_SET);
                fread(&pb, sizeof(pb), 1, disc);
                pb.nextBlockAddress = freeBlock;
                fseek(disc, -((long)sizeof(pb)), SEEK_CUR);
                fwrite(&pb, sizeof(pb), 1, disc);
            }
            prevBlock = freeBlock;
            currentBlock = freeBlock;

            {
                struct Block blk;
                memset(&blk, 0, sizeof(blk));
                blk.state = TAKEN;
                blk.nextBlockAddress = 0;
                memcpy(blk.data, buffer, bytesRead);
                fseek(disc, db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block),
                      SEEK_SET);
                fwrite(&blk, sizeof(blk), 1, disc);

                printf("block of data written at: %d \n",
                       db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block));
                printf("first block address: %d\n", db.firstBlockAddress);
                printf("current block address %d\n", currentBlock);
            }
        }
    }

    fseek(disc, db.memoryMapAddress + db.blockCount * sizeof(int) + descriptorIndex * sizeof(struct FileDescriptor),
          SEEK_SET);
    fwrite(&fd, sizeof(fd), 1, disc);

    fclose(disc);
    fclose(src);
    return 0;
}

int cpFromDisc(char *fileName, char *discName)
{
    struct FileDescriptor fd;
    struct DiscBasic db;
    FILE *outFile;
    FILE *disc;
    unsigned currentBlock;

    fd = getFileDescriptor(fileName, discName);
    db = loadDiscBasic(discName);

    outFile = fopen(fileName, "wb");
    if (!outFile)
        return -1;

    disc = fopen(discName, "rb");
    if (!disc)
    {
        fclose(outFile);
        return -1;
    }

    currentBlock = fd.firstBlockAddress;
    while (currentBlock)
    {
        struct Block blk;
        fseek(disc, db.firstBlockAddress + (currentBlock - 1) * sizeof(struct Block), SEEK_SET);
        if (fread(&blk, sizeof(blk), 1, disc) != 1)
            break;
        fwrite(blk.data, 1, my_strnlen(blk.data, BLOCK_SIZE), outFile);
        currentBlock = blk.nextBlockAddress;
    }

    fclose(disc);
    fclose(outFile);
    return 0;
}

int printTakenSpace(char *discName)
{
    struct DiscBasic db;
    FILE *disc;
    int takenCount = 0, mapVal, i;

    db = loadDiscBasic(discName);
    disc = fopen(discName, "rb");
    if (!disc)
    {
        return -1;
    }
    fseek(disc, db.memoryMapAddress, SEEK_SET);
    for (i = 0; i < (int)db.blockCount; i++)
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
    struct DiscBasic db;
    FILE *drive;
    int i;

    db = loadDiscBasic(discName);
    drive = fopen(discName, "rb");
    if (!drive)
    {
        printf("Error opening disc\n");
        return -1;
    }

    printf("Memory Map:\n");
    for (i = 0; i < (int)db.blockCount; i++)
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