#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

/**
 * @brief In this project,I will finish a tiny C Interpreter
 * Comparing with the PKU Programming Practice Course, I will not use Flex to generate the lexical analysis
 * and Bison to generate the grammar analysis
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
+---------------------------------------------------------+ Low address
*/
// So we need to create a virtual machine to maintain a heap for memory \
// allocation
//----------------------------------------------------------------

// Pointer
int *text;      // Code segment
int *old_text;  // Dump text segment
int *stack;     // Stack segment
char *data;     // Data segment

// Storing the pc statement in the register
// PC,SP,BP,AX        Virtual machine registers
int *pc, *sp, *bp, ax, cycle;

// Instruction set basing on the x86 instruction set
enum { 
    LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT 
};









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
    int op, *tmp;

    while(1)
    {
        // Getting the next operation code
        op = *pc++;

        if( op == IMM )       {ax = *pc++;}                       // Loading the immediate value to ax
        else if( op == LC )   {ax = *(char *)ax;}                 // Loading the character value to ax and storing into ax
        else if( op == LI )   {ax = *(int *)ax;}                  // Loading Integer value to ax and storing into ax
        else if( op == SC )   {ax = *(char *) *sp++ = ax;}        // Saving the data as the character value into the address and Storing in the stack

        else if( op == SI )   {*(int *)*sp++ = ax;}               // Saving the data as the integer value into the address and Storing in the stack
        else if( op == PUSH)  { *--sp = ax; }                     // Pushing the value of ax into the stack

        // jump <addr>
        else if( op == JMP )  { pc = (int *)pc; }                 // Jumping to the next address

        // "If" judgement
        else if( op == JZ )   { pc == ax ? pc + 1 : (int *)pc; }  // Jumping to the next address if the ax is zero
        else if( op == JNZ )  { pc == ax ? (int *)pc : pc + 1; }  // Jumping to the next address if the ax isn't zero

        // *!* Callback subroutine
        // ----------------------------------------------------------------
        // We should not use the "JMP", because when the function are returning from the subroutine we need to
        // store the address of the "JMP" address
        // So we need to store the address of the pc
        else if( op == CALL ) { *--sp = (int)(pc + 1); pc = (int *)*pc; } // Calling back the subroutine
    //  else if( op == RET )  { pc = (int *)*sp++; }              // Returning from the subroutine. Replacing by the "LEV"

        // ENT <size>, make new call frame
        else if( op == ENT )  { *--sp = (int) bp; bp = sp; sp = sp - *pc++;}

        // ADJ <size>, remove arguments from frame
        else if( op == ADJ )  { sp = sp + *pc++; }               // Adding esp, <size>
        else if( op == LEV )  { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }  // Restoring call frame and pc

        // LEA <offset>
        else if( op == LEA )  { ax = (int) bp + *pc++; }        // Loading address for arguments

        // ----------------------------------------------------------------


        // *!* Operator
        // ----------------------------------------------------------------
        // (the first argument in the top of the stack) op (ax)
        // After calculation, the parameters at the top of the stack will be pushed back, \
        // and the results will be stored in register ax

        else if( op == OR )  { ax = *sp++ | ax; }             
        else if( op == XOR ) { ax = *sp++ ^ ax; }            
        else if( op == AND ) { ax = *sp++ & ax; }             
        else if( op == EQ )  { ax = *sp++ == ax; }             
        else if( op == NE )  { ax = *sp++!= ax; }  

        else if( op == LT )  { ax = *sp++ < ax; }   
        else if( op == LE )  { ax = *sp++ <= ax; }
        else if( op == GT )  { ax = *sp++ > ax; }
        else if( op == GE )  { ax = *sp++ >= ax; }   
        else if( op == SHL ) { ax = *sp++ << ax; }
        else if( op == SHR ) { ax = *sp++ >> ax; }

        else if( op == ADD ) { ax = *sp++ + ax; }
        else if( op == SUB ) { ax = *sp++ - ax; }
        else if( op == MUL ) { ax = *sp++ * ax; }
        else if( op == DIV ) { ax = *sp++ / ax; }
        else if( op == MOD ) { ax = *sp++ % ax; }

        // ----------------------------------------------------------------


        // *!* Built-in functions
        // ----------------------------------------------------------------
        // Using C functions to bootstrap the program for supporting the virtual machine

        else if (op == EXIT) { printf("exit(%d)", *sp); return *sp;}
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }

        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}

        else
        {
            // Erro Judgment
            printf("Error: Unknown instruction: %d\n", op);
            return -1;
        }




    }



    return 0;
}

int main(int argc, char **argv)
{
    int i,fd;                // The file descriptor

    argc--;
    argv++;

    poolSize = 512 * 1024;   // Arbitrary memory size, 512KB
    line = 1;

    // ----------------------------------------------------------------
    // Reading the source code file

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

    // ---------------------------------------------------------------------
    // Creating the virtual machine

    // Allocating the memory for virtual machine
    if(!(text = old_text = malloc(poolSize)))
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for text area\n",poolSize);
        return -1;
    }

    if( !(data = malloc(poolSize)) )
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for data area\n",poolSize);
        return -1;
    }

    if(!(stack = malloc(poolSize)) )
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for stack area\n",poolSize);
        return -1;
    }

    memset(text,0,poolSize);
    memset(data,0,poolSize);
    memset(stack,0,poolSize);

    // Instruction set
    bp = sp = (int *)((int) stack + poolSize);
    ax = 0;

    // *!*  Test 1: Testing the instructions set
    //----------------------------------------------------------------
    /**
    i = 0;

    text[i++] = IMM;
    text[i++] = 10;
    text[i++] = PUSH;
    text[i++] = IMM;
    text[i++] = 20;
    text[i++] = ADD;
    text[i++] = PUSH;
    text[i++] = EXIT;

    pc = text;
    **/
    // ----------------------------------------------------------------


    














    program();

    return eval();
}


