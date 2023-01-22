#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

/**
 * @brief In this project,I will finish a tiny C Interpreter
*/

int token;              // Current token
char *src, *old_src;    // The pointer of the code source
int poolSize;           // The default memory size of the text/data/stack in the programming
int line;                // The current number of lines

//----------------------------------------------------------------
/**
The process will be divided into the following parts in the memory

+---------------------------------------------------------+ High address
|    Stack: Processing data related to function calls     |
|                      ..............                     |     
|                             |                           |
|                             V                           |
|                                                         |
|                                                         |
|    Heap: Dynamically allocate memory for programs       |
+---------------------------------------------------------+
|    Bss segment: The uninitialized data segment          |
+---------------------------------------------------------+
|    Data segment: The initial stack segment              |
+---------------------------------------------------------+
|    Text segment: The code                               |
+---------------------------------------------------------+









* 
*/
//----------------------------------------------------------------






// Using for the lexical analysis and Getting the next token in the source code
// This function will ignore the spaces
void next()
{
    token = *src++;
    return ;
}

// Analysis of the source code
// This function will be took as the entrance of the grammar analysis
void program()
{
    next();                       // Getting the next token
    while( token > 0 )
    {
        printf("The toke is: %c\n", token);
        next();
    }
}

// Analysis of the code expression
void expression(int level)
{
    // TODO: Implement
}

// Interpreting the code and Entering the virtual machine
int eval()
{
    // TODO: Implement
    return 0;
}

int main(int argc, char **argv)
{
    int i,fd;                // The file descriptor

    argc--;
    argv++;

    poolSize = 512 * 1024;   // Arbitrary memory size, 512KB
    line = 1;

    // Opening the file
    if((fd = open(*argv,0)) < 0)
    {
        // Error opening
        printf("The Tiny-C-Interpreter could not open (%s)\n", *argv);
        return -1;
    }

    // Allocating the memory
    if( !(src = old_src = malloc(poolSize)) )
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for source area\n", poolSize);
        return -1;
    }

    // Now begin to read the source code and interpreter the code
    if( (i = read(fd,src,poolSize - 1)) <= 0)
    {
        // Error reading
        printf("The read() return %d\n", i);
        return -1;
    }

    // Adding the EOF character
    src[i] = 0;
    close(fd);

    // TODO: Implement

    program();

    return eval();
}


