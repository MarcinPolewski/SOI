#include "operator.h"

int main(int argc, char **argv)
{
    int operationResult, discSize;

    if (argc == 1)
    {
        printf("No instruction provided\n");
        return 1;
    }

    if (strcmp(argv[1], "ls") == 0)
    {
        if (argc == 3)
        {
            operationResult = performLsCommand(argv[2]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else if (argc == 4 && strcmp(argv[2], "-a") == 0)
        {
            operationResult = performLsAllCommand(argv[3]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "create_disc") == 0)
    {
        if (argc == 4)
        {
            discSize = atoi(argv[3]);
            operationResult = createDisc(argv[2], discSize);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "delete_disc") == 0)
    {
        if (argc == 3)
        {
            operationResult = deleteDisc(argv[2]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "rm") == 0)
    {
        if (argc == 4)
        {
            operationResult = rmFile(argv[2], argv[3]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "cp_to_disc") == 0)
    {
        if (argc == 4)
        {
            operationResult = cpToDisc(argv[2], argv[3]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "cp_from_disc") == 0)
    {
        if (argc == 4)
        {
            operationResult = cpFromDisc(argv[2], argv[3]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "print_taken_space") == 0)
    {
        if (argc == 3)
        {
            operationResult = printTakenSpace(argv[2]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else if (strcmp(argv[1], "print_memory_map") == 0)
    {
        if (argc == 3)
        {
            operationResult = printMemoryMap(argv[2]);
            if (operationResult == 0)
            {
                printf("Operation completed successfully\n");
            }
            else
            {
                printf("Operation failed\n");
            }
        }
        else
        {
            printf("Invalid number of arguments\n");
        }
    }
    else
    {
        printf("Invalid instruction\n");
    }
}
