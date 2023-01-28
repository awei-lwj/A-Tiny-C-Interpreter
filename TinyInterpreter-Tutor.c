#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

// Not supporting the marco/struct

// Frame
// -- source code file ---> lexer ---> token stream ---> parser ---> assembly            


int token;           // Current token
char *src, *old_src; // The pointer of the code source
int poolSize;        // The default memory size of the text/data/stack in the programming
int line;            // The current number of lines

//----------------------------------------------------------------
// The process will be divided into the following parts in the memory
// 
// +---------------------------------------------------------+ High address
// |    Stack: Processing data related to function calls     |
// |                      ..............                     |
// |                             |                           |
// |                             V                           |
// |                                                         |
// |                                                         |
// |    Heap: Dynamically allocate memory for programs       |
// +---------------------------------------------------------+
// |    Bss segment: The uninitialized data segment          |
// +---------------------------------------------------------+
// |    Data segment: The initial stack segment              |
// +---------------------------------------------------------+
// |    Text segment: The code                               |
// +---------------------------------------------------------+ Low address
// So we need to create a virtual machine to maintain a heap for memory allocation
//----------------------------------------------------------------

// Pointer
int *text;     // Code segment
int *old_text; // Dump text segment
int *stack;    // Stack segment
char *data;    // Data segment

// Storing the pc statement in the register
// PC,SP,BP,AX        Virtual machine registers
int *pc, *sp, *bp, ax, cycle;

// Instruction set based on the x86 instruction set
enum
{
    LEA,IMM,JMP,CALL,JZ,JNZ,ENT,ADJ,LEV,LI,LC,SI,SC,PUSH,
    OR,XOR,AND,EQ,NE,LT,GT,LE,GE,SHL,SHR,ADD,SUB,MUL,DIV,MOD,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT
};

// ----------------------------------------------------------------

// Lexical analysis
// ----------------------------------------------------------------
// Tokens and classes (operators last and in precedence order)
enum
{
    Num = 128,Fun,Sys,Glo,Loc,Id,
    Char,Else,Enum,If,Int,Return,Sizeof,While,
    Assign,Cond,Lor,Lan,Or,Xor,And,Eq,Ne,Lt,Gt,Le,Ge,Shl,Shr,Add,Sub,Mul,Div,Mod,Inc,Dec,Brak
};

// Identifier: The ID of the variable
struct identifier
{
    int token; // The token returned by the identifier
    int hash;

    // Local variables
    char *name; // The name of the local variable
    int class;  // The class of the local variable
    int type;   // The type of the local variable
    int value;  // The value of the local variable

    // Global variables
    int Bclass;
    int Btype;
    int Bvalue;
};

int token_val;   // The value of the current token(mainly of number)
int *current_id; // The current parsed ID
int *symbols;    // The symbol table

// fields of identifier
enum
{
    Token,Hash,Name,Type,Class,Value,BType,BClass,BValue,IdSize
};

// Types of variables/functions
enum
{
    CHAR, INT, PTR
};

int *idmain;         // the main function

// Grammar analysis
int basetype;         // the type of a declaration, make it global for convenience
int expr_type;       // the type of an expression

int index_of_bp;     // the index of the pointer on stack using for parsing the function

// Using for the lexical analysis and Getting the next token in the source code
// This function will ignore the spaces
void next()
{
    char *last_pos;
    int hash;

    while (token = *src)
    {
        ++src;

        // Parsing the token stream
        if (token == '\n')
        {
            line++;
        }
        else if (token == '#') // Not supporting the macro
        {
            while (*src != 0 && *src != '\n')
            {
                src++;
            }
        }
        // ----------------------------------------------------------------
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) // character
        {
            // Parsing the identifier
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') ||
                   (*src >= '0' && *src <= '9') || (*src == '_'))
            {
                hash = hash * 147 + *src;
                src++;
            }

            // Searching for the existing identifier(linear search)
            current_id = symbols;

            while (current_id[Token])
            {
                if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos))
                {
                    // The token exits
                    token = current_id[Token];
                    return;
                }

                current_id = current_id + IdSize;
            }

            // Storing the new ID
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;

            token = current_id[Token] = Id;

            return;
        }
        // ----------------------------------------------------------------
        else if (token >= '0' && token <= '9') // Parsing the number
        {
            token_val = token - '0';
            if (token_val > 0)
            {
                // Decimal
                while (*src >= '0' && *src <= '9')
                {
                    token_val = token_val * 10 + *src++ -'0';
                }
            }
            else
            {
                // Starting with number 0
                if (*src == 'x' || *src == 'X')
                {
                    // Hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || 
                           (token >= 'A' && token <= 'F'))
                    {
                        token_val = token_val * +(token & 15) + (token >= 'A' ? 9 : 0);
                        token = *src++;
                    }
                }
                else
                {
                    // Oct
                    while (*src >= '0' && *src <= '7')
                    {
                        token_val = token_val * 8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }
        // ----------------------------------------------------------------
        else if (token == '"' || token == '\'') // String
        {
            // Parsing string literal, currently, there are only supporting escape
            // characters is '\n', store the string literal into data
            last_pos = data;
            while (*src != 0 && *src != token)
            {
                token_val = *src++;
                if (token_val == '\\')
                {
                    // Escaping character
                    token_val = *src++;
                    if (token_val == 'n')
                    {
                        token_val = '\n';
                    }
                }

                if (token == '"')
                {
                    *data++ = token_val;
                }
            }

            src++;

            // A single character
            if (token == '"')
            {
                token_val = (int)last_pos;
            }
            else
            {
                token = Num;
            }

            return;
        }
        // ----------------------------------------------------------------
        // look ahead
        else if (token == '/') // comment of //
        {
            if (*src == '/')
            {
                // Skipping comments
                while (*src != 0 && *src != '\n')
                {
                    ++src;
                }
            }
            else
            {
                // Divide Operator
                token = Div;
                return;
            }
        }
        // ----------------------------------------------------------------
        else if (token == '=')
        {
            // parse '==' and '='
            if (*src == '=')
            {
                src++;
                token = Eq;
            }
            else
            {
                token = Assign;
            }

            return;
        }
        else if (token == '+')
        {
            // parse '+' and '++'
            if (*src == '+')
            {
                src++;
                token = Inc;
            }
            else
            {
                token = Add;
            }

            return;
        }
        else if (token == '-')
        {
            // parse '-' and '--'
            if (*src == '-')
            {
                src++;
                token = Dec;
            }
            else
            {
                token = Sub;
            }

            return;
        }
        else if (token == '!')
        {
            // parse '!='
            if (*src == '=')
            {
                src++;
                token = Ne;
            }

            return;
        }
        else if (token == '<')
        {
            // parse '<=', '<<' or '<'
            if (*src == '=')
            {
                src++;
                token = Le;
            }
            else if (*src == '<')
            {
                src++;
                token = Shl;
            }
            else
            {
                token = Lt;
            }

            return;
        }
        else if (token == '>')
        {
            // parse '>=', '>>' or '>'
            if (*src == '=')
            {
                src++;
                token = Ge;
            }
            else if (*src == '>')
            {
                src++;
                token = Shr;
            }
            else
            {
                token = Gt;
            }
            return;
        }
        else if (token == '|')
        {
            // parse '|' or '||'
            if (*src == '|')
            {
                src++;
                token = Lor;
            }
            else
            {
                token = Or;
            }
            return;
        }
        else if (token == '&')
        {
            // parse '&' and '&&'
            if (*src == '&')
            {
                src++;
                token = Lan;
            }
            else
            {
                token = And;
            }
            return;
        }
        else if (token == '^')
        {
            token = Xor;
            return;
        }
        else if (token == '%')
        {
            token = Mod;
            return;
        }
        else if (token == '*')
        {
            token = Mul;
            return;
        }
        else if (token == '[')
        {
            token = Brak;
            return;
        }
        else if (token == '?')
        {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':')
        {
            // directly return the character as token;
            return;
        } // end if

    } // end while

    return;
}

// Grammar analysis
// ----------------------------------------------------------------

// This function will be took as the entrance of the grammar analysis
void program()
{
    next(); // Getting the next token
    
    while (token > 0)
    {
        global_declaration();
    }
}

// global variables
void global_declaration()
{
    // EBNF
    // global_declaration ::= enum_decl | variable_decl | function_decl
    //
    // enum_decl ::= 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num'} '}'
    //
    // variable_decl ::= type {'*'} id { ',' {'*'} id } ';'
    //
    // function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'

    int type;       // type of the variable
    int i;

    basetype = INT; 

    // Parsing the enum
    if( token == Enum )
    {
        // enum [id] { awei = 985,school = 211 }
        match(Enum);
        if( token != '{' )
        {
            match(Id);   // Skipping the [id] part;         
        }
        if( token == '{' )
        {
            // Parsing the assignment part
            match('{');
            enum_declaration();
            match('}');
        }

        match(';');
        return ;
    }

    // Parsing type information
    if( token == Int )
    {
        match(Int);
    }
    else if( token == Char )
    {
        match(Char);
        basetype = CHAR;
    }

    // Parsing the common seperator variables declaration
    while( token != ';' && token != '}' )
    {
        type = basetype;

        while( token == MUL )
        {
            match(MUL);
            type = type + PTR;
        }

        if( token != Id )
        {
            // Invalid declaration
            printf("%d: Bad declaration\n", line);
            exit(-1);
        }
        
        if( current_id[Class])
        {
            // Identifier exits
            printf("%d: Duplicate global declaration\n", line);
            exit(-1);
        }

        match(Id);
        current_id[Type] = type;

        if( token == '(' )
        {
            current_id[Class] = Fun;
            current_id[Value] = (int) (text + 1);   // The address of the function
            function_declaration();
        }
        else
        {
            // Variable declaration
            current_id[Class] = Glo;     // Global variable
            current_id[Value] = (int) data;  // Assignment memory address
            data = data + sizeof(int); 
        }

        if( token == ',' )
        {
            match(',');
        }

    } // end while

    next();

}

void match(int tk)
{
    if( token == tk )
    {
        next();
    }
    else
    {
        printf("%d: expected token: %d\n", line, tk);
        exit(-1);
    }
}

void enum_declaration()
{
    int i = 0;

    while( token != '}' )
    {
        if( token != Id )
        {
            printf("%d: Bad enum identifier %d\n", line, token);
            exit(-1);
        }

        next();

        if( token == Assign )
        {
            next();

            if( token != Num )
            {
                printf("%d: Bad enum initializer\n",line);  
                exit(-1);      
            }

            i = token_val;
            next();
        }

        current_id[Class] = Num;
        current_id[Type]  = INT;
        current_id[Value] = i++;

        if( token == ',' )
        {
            next();
        }

    }

}

// Parsing the function
// ----------------------------------------------------------------

void function_declaration()
{
    // Parameter
    match('(');
    function_parameter();
    match(')');

    // Body
    match('{');
    function_body();
  // match('}');

    // Reason
    // variable_decl 与 function_decl 是放在一起解析的，而 variable_decl 是以字符 ; 结束的。
    // 而 function_decl 是以字符 } 结束的，若在此通过 match 消耗了 ‘;’ 字符，那么global_declaration 
    // 外层的 while 循环就没法准确地知道函数定义已经结束。所以我将结束符的解析放在了global_declaration外层的 while 循环中。
    
    current_id = symbols;

    // Unwind local variable declarations for all local variables.
    while(current_id[Token])
    {
        if( current_id[Class] == Loc )
        {
            current_id[Class] = current_id[BClass];
            current_id[Type]  = current_id[BType];
            current_id[Value] = current_id[BValue];
        }
        current_id = current_id + IdSize;

    }



}

void function_parameter()
{
    int type;
    int params = 0;

    while( token != ')' )
    {
        type = INT;

        // Type matches
        if( token == INT )
        {
            match(Int);
        }
        else if( token == Char )
        {
            type = CHAR;
            match(Char);
        }

        // Pointer
        while( token == Mul )
        {
            match(Mul);
            type = type + PTR;
        }

        // Parameter name
        if( token != Id )
        {
            printf("%d: Bad parameter declaration\n",line);
            exit(-1);
        }

        if( current_id[Class] == Loc )
        {
            printf("%d: Duplicate parameter declaration\n",line);
            exit(-1);
        }

        match(Id);

        // Storing the local variable
        current_id[BClass] = current_id[Class]; current_id[Class] = Loc;
        current_id[BType]  = current_id[Type];  current_id[Type]  = type;
        current_id[BValue] = current_id[Value]; current_id[Value] = params++; // index of current parameter

        if( token == ',' )
        {
            match(',');
        }
    }

    index_of_bp = params + 1;

}

void function_body()
{
    //{
    //   1. local declarations 2. statement   
    //}

    int pos_local;     // the position of the local variables on the stack
    int type;
    pos_local = index_of_bp;

    while( token == Int || token == Char )
    {
        basetype = (token == Int ) ? INT : CHAR;
        match(token);

        // local variables
        while( token != ';' )
        {
            // Same as the global variable
            type = basetype;

            while( token == Mul )
            {
                match(Mul);
                type = type + PTR;
            }

            if( token != Id )
            {
                // Invalid declaration
                printf("%d: Bad local declaration\n",line);
                exit(-1);
            }

            if( current_id[Class] == Loc )
            {
                // Identifier exits
                printf("%d: Duplicate local declaration\n",line);
                exit(-1);
            }

            match(Id);

            // Storing the local variable
            current_id[BClass] = current_id[Class]; current_id[Class] = Loc;
            current_id[BType]  = current_id[Type];  current_id[Type]  = type;
            current_id[BValue] = current_id[Value]; current_id[Value] = ++pos_local; // the index of the current parameter

            if( token == ',' )
            {
                match(',');
            }

        }

        match(';');

    }

    // Saving the stack size for local variables
    *++text = ENT;
    *++text = pos_local - index_of_bp;

    // Statement
    while( token != '}' )
    {
        statement();
    }

    // Leaving the sub function
    *++text = LEV;

}

void statement()
{
    // In my Tiny-C-Compiler, there are 6 statements recognized
    // 1. if(...) <statement> [else <statement>]
    // 2. while(...) <statement>
    // 3. { <statement> }
    // 4. return xxx;
    // 5. <empty statement>;
    // 6. expression;

    int *a, *b;    // Branch control

    // ----------------------------------------------------------------

    //    <cond>
    //    JZ a
    //    <true statement>
    //   JMP b
    // a:
    //   <false statement>
    // b:

    if( token == If )
    {
        match(If);
        match('(');
        expression(Assign); // Condition
        match(')');

        *++text = JZ;
        b = ++text;

        statement();       // Parsing statement

        if( token == Else )
        {
            // Parsing Else
            match(Else);
            
            // Emit code for JMP B
            *b = (int) (text + 3);
            *++text = JMP;
            b = ++text;

            statement();

        }

        *b = (int)(text + 1);

    }
    // ----------------------------------------------------------------
    // a:  
    //    <cond>
    //    JZ b
    //    <statement>
    //    JMP a
    // b:
    else if( token == While )
    {
        match(While);

        a = text + 1;

        match('(');
        expression(Assign);
        match(')');

        *++text = JZ;
        b = ++text;

        statement();

        *++text = JMP;
        *++text = (int)a;
        *b = (int)(text + 1);
    }
    // ----------------------------------------------------------------
    else if( token == Return )
    {
        // return [expression]
        match(Return);

        if( token != ';' )
        {
            expression(Assign);
        }

        match(';');

        // Emit code for return
        *++text = LEV;
    }
    else if( token == '{' )
    {
        // { <statement> }
        match('{');

        while( token != '}' )
        {
            statement();
        }

        match('}');
    }
    else if( token == ';' )
    {
        // ; <empty statement>
        match(';');
    }
    else
    {
        // a = b; or function_call();
        expression(Assign);
        match(';');
    }

}

// ----------------------------------------------------------------

// Analysis of the code expression
void expression(int level)
{
    // 1. unity_unary ::= unit | unit unary_op | unary_op unit
    // 2. expr        ::= unit_unary_op (bin_op unit_unary ...)

    int *id;
    int tmp;
    int *addr;

    {
        if( !token )
        {
            printf("%d: Unexpected token EOF of expression\n", line);
            exit(-1);
        }

        // Unary expression
        // ----------------------------------------------------------------

        // Constant expression
        // ----------------------------------------------------------------
        if( token == Num )     // INT
        {
            match(Num);

            // Emitting code
            *++text = IMM;
            *++text = token_val;
            expr_type = INT;
        }
        else if( token == '"' )    // CHAR
        {
            // p = "first line "      -->     p = "first line second line"
            //     "second line"

            // Emitting code
            *++text = IMM;
            *++text = token_val;

            match('"');

            // Storing the rest of the string
            while( token = '"' )
            {
                match('"');
            }

            // Appending the end of the string character '\0'
            // All the data are default to 0, so we just move data one position forward
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
            expr_type = PTR;
        }
        // ----------------------------------------------------------------

        // Sizeof
        // ----------------------------------------------------------------
        else if( token == Sizeof )
        {
            // Now only supporting "sizeof(int)" "sizeof(char)" "sizeof(*....)(pointer)"

            match(Sizeof);
            match('(');
            expr_type = INT;

            if( token == INT )
            {
                match(Int);
            }
            else if( token == Char )
            {
                match(Char);
                expr_type = CHAR;
            }

            while( token == Mul )
            {
                match(Mul);
                expr_type = PTR + expr_type;
            }

            match(')');
        
            // Emitting code
            *++text = IMM;
            *++text = (expr_type == INT) ? sizeof(int) : sizeof(char);

            expr_type = INT;
        }
        // ----------------------------------------------------------------

        // Variable and function
        // ----------------------------------------------------------------
        else if( token == Id )
        {
            // Not only support for
            // 1. function_call
            // 2. enum variable
            // 3. global/local variable

            match(Id);

            id = current_id;

            if( token == '(' )
            {
                // function call
                match('(');

                // Parsing the parameters
                // Comparing with the standard C,we put parameter on the stack in order
                tmp = 0; // the number of arguments
                while( token != ')' )
                {
                    expression(Assign);
                    *++text = PUSH;

                    tmp++;

                    if( token == ',' )
                    {
                        match(',');
                    }
                }

                match(')');

                // Parsing the function type declaration
                // Emitting code
                if( id[Class] == Sys )
                {
                    *++text = id[Value];  // System function
                }
                else if( id[Class] == Fun )
                {
                    *++text = CALL; *++text = id[Value]; // Fun function
                }
                else
                {
                    printf("%d: Bad function call\n", line);
                    exit(-1);
                }

                // Clearing the arguments for the stack
                if( tmp > 0 )
                {
                    *++text = ADJ;
                    *++text = tmp;
                }
                expr_type = id[Type];

            } // end if

            else if( id[Class] == Num )
            {
                // Enum variables
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            }
            else
            {
                // Global/Local variables
                if( id[Class] == Loc )
                {
                    *++text = LEA;
                    *++text = index_of_bp - id[Value];
                }
                else if( id[Class] == Glo )
                {
                    *++text = IMM;
                    *++text = id[Value];
                }
                else
                {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }

                // Emitting code
                // Loading the value(default) of this address in the ax
                expr_type = id[Type];
                *++text = (expr_type == Char ) ? LC : LI; 

            }

        } // end if( variable and function )
        // ----------------------------------------------------------------

        // Forced conversion
        else if( token == '(' )
        {
            match('(');

            if( token == Int || token == Char )
            {
                // Cast type
                tmp = (token == Char) ? CHAR : INT;

                match(token);

                while(token == Mul)
                {
                    match(Mul);
                    tmp = tmp + PTR;
                }

                match(')');

                // Cast has precedence as Inc
                expression(Inc);

                expr_type = tmp;
            }
            else
            {
                // Normal expression
                expression(Assign);
                match(')');
            }

        }

        // Pointer
        else if( token == Mul )
        {
            // Deference *<addr>
            match(Mul);
            expression(Inc);

            if( expr_type >= PTR )
            {
                expr_type = expr_type - PTR;
            }
            else
            {
                printf("%d: Bad deference\n",line);
                exit(-1);
            }

            *++text = (expr_type == CHAR) ? LC : LI;
        }
        else if( token == And )
        {
            // Get the address
            match(And);
            expression(Inc);

            if(*text == LC || *text == LI)
            {
                text--;
            }
            else
            {
                printf("%d: Bad address\n",line);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        }
        else if( token == '|' )
        {
            match('|');
            expression(Inc);

            // Emitting code
            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            expr_type = INT;
        }
        else if( token == '~' )
        {
            match('~');
            expression(Inc);

            // Emitting code, use <expr> XOR -1
            *++text = PUSH;
            *++text = IMM;
            *++text = -1;
            *++text = XOR;

            expr_type = INT;
        }
        else if( token == Add )
        {
            match(Add);
            expression(Inc);

            expr_type = INT;
        }
        else if( token == Sub )
        {
            match(Sub);

            if( token == Num )
            {
                *++text = IMM;
                *++text = -token_val;
                match(Num);
            }
            else
            {
                *++text = IMM;
                *++text = -1;
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
            }

            expr_type = INT;
        }
        else if( token == Inc || token == Dec )
        {
            tmp = token;
            match(token);
            expression(Inc);

            if( *text == LC )
            {
                *text = PUSH;    // Duplicate the address
                *++text = LC;
            }
            else if( *text == LI )
            {
                *text = PUSH;
                *++text = LI;
            }
            else
            {
                printf("%d: Bad lvalue of pre-increment\n",line);
                exit(-1);
            }

            *++text = PUSH;
            *++text = IMM;

            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (tmp == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        }

        // ----------------------------------------------------------------
    }

     // binary operator and postfix operators. 
    {
        while( token >= level )
        {
            tmp = expr_type;

            // a = (expression)->IMM <addr> LC/LI
            if( token == Assign )
            {
                match(Assign);
                if( *text == LC || *text == LI )
                {
                    // Storing the lvalue's pointer
                    *text = PUSH;
                }
                else
                {
                    printf("%d: Bad lvalue in assignment\n",line);
                    exit(-1);
                }

                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR ) ? SC : SI;
            }
            else if( token == Cond )   // expression ? a : b
            {
                match(Cond);
                *++text = JZ;

                addr = ++text;

                expression(Assign);

                if(token == ':')
                {
                    match(':');
                }
                else
                {
                    printf("%d: Missing colon in expression\n", line);
                    exit(-1);
                }

                *addr = (int)(text + 3);
                *text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
            }
            else if( token == Lor )   // '||'
            {
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if( token == Lan )   // '&&'
            {
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);
                expr_type = INT;
            }

            // Mathematical operators: |, ^, &, ==, != <=, >=, <, >, <<, >>, +, -, *, /, %
            // ----------------------------------------------------------------
            else if ( token == Or )       // '|'
            {
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;
                expr_type = INT;
            }
            else if ( token == Xor )       // '^'
            {
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;
                expr_type = INT;
            }
            else if ( token == And )       // '&'
            {
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                expr_type = INT;
            }
            else if ( token == Eq )       // ==
            {
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;
                expr_type = INT;
            }
            else if ( token == Ne )       // '!='
            {
                match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;
                expr_type = INT;
            }
            else if ( token == Lt )       // '<'
            {
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                expr_type = INT;
            }
            else if ( token == Gt )        // '>'
            {
                match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;
                expr_type = INT;
            }
            else if ( token == Le )          // <=
            {
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;
                expr_type = INT;
            }
            else if ( token == Ge )          // >=
            {
                match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;
                expr_type = INT;
            }
            else if ( token == Shl )          // <<
            {
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                expr_type = INT;
            }
            else if ( token == Shr )          // >>
            {
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                expr_type = INT;
            }
            else if ( token == Add ) 
            {
                // add
                match(Add);
                *++text = PUSH;
                expression(Mul);

                expr_type = tmp;
                if (expr_type > PTR) 
                {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }

                *++text = ADD;
            }
            else if (token == Sub) 
            {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);

                if (tmp > PTR && tmp == expr_type) 
                {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } 
                else if (tmp > PTR) 
                {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;
                    expr_type = tmp;
                } 
                else 
                {
                    // numeral subtraction
                    *++text = SUB;
                    expr_type = tmp;
                }

            }
            else if ( token == Mul )      // a * b
            {
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
                expr_type = tmp;
            }
            else if ( token == Div )       // a / b
            {
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;
                expr_type = tmp;
            }
            else if (token == Mod)         // a % b
            {
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;
                expr_type = tmp;
            }
            // ----------------------------------------------------------------

            else if( token == Inc || token == Dec )
            {
                // Getting its original value from the ax
                // Increasing/Decreasing the value to the variable

                if( *text == LI )
                {
                    *text = PUSH;
                    *++text = LI;
                }
                else if( *text == LC )
                {
                    *text = PUSH;
                    *++text = LC;
                }
                else
                {
                    printf("%d: Bad value in increment\n", line);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;

                match(token);

            }
            else if ( token == Brak )
            {
                // array access var[xx]
                match(Brak);
                *++text = PUSH;
                expression(Assign);
                match(']');

                if (tmp > PTR) 
                {
                    // pointer, `not char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                else if (tmp < PTR) 
                {
                    printf("%d: Pointer type expected\n", line);
                    exit(-1);
                }

                expr_type = tmp - PTR;
                *++text = ADD;
                *++text = (expr_type == CHAR) ? LC : LI;
            }
            else
            {
                printf("%d: Compiler error, The token = %d\n", line, token);
                exit(-1);
            }

        }

    }

}

// Interpreting the code and Entering the virtual machine
int eval()
{
    int op, *tmp;

    while (1)
    {
        // Getting the next operation code
        op = *pc++;

        if (op == IMM) { ax = *pc++; } // Loading the immediate value to ax

        else if (op == LC)  { ax = *(char *)ax; }         // Loading the character value to ax and storing into ax
        else if (op == LI)  { ax = *(int *)ax;  }         // Loading Integer value to ax and storing into ax
        else if (op == SC)  { ax = *(char *)*sp++ = ax; } // Saving the data as the character value into the address and Storing in the stack
        else if (op == SI)  { *(int *)*sp++ = ax; }       // Saving the data as the integer value into the address and Storing in the stack
        else if (op == PUSH){ *--sp = ax; }               // Pushing the value of ax into the stack

        // jump <addr>./a
        else if (op == JMP) { pc = (int *)pc; }           // Jumping to the next address

        // "If" judgement
        else if (op == JZ)  { pc = ax ? pc + 1 : (int *)pc; } // Jumping to the next address if the ax is zero
        else if (op == JNZ) { pc = ax ? (int *)pc : pc + 1; } // Jumping to the next address if the ax isn't zero

        // *!* Callback subroutine
        // ----------------------------------------------------------------
        // We should not use the "JMP", because when the function are returning from the subroutine we need to
        // store the address of the "JMP" address
        // So we need to store the address of the pc
        else if (op == CALL)
        {
            *--sp = (int)(pc + 1);
            pc = (int *)*pc;
        } 
        // Calling back the subroutine
    //  else if( op == RET )  { pc = (int *)*sp++; }              // Returning from the subroutine. Replacing by the "LEV"

        // ENT <size>, make new call frame
        else if (op == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }

        // ADJ <size>, remove arguments from frame
        else if (op == ADJ) { sp = sp + *pc++; } // Adding esp, <size>
        else if (op == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; } // Restoring call frame and pc

        // LEA <offset>
        else if (op == LEA) { ax = (int)bp + *pc++; } // Loading address for arguments

        // ----------------------------------------------------------------

        // *!* Operator
        // ----------------------------------------------------------------
        // (the first argument in the top of the stack) op (ax)
        // After calculation, the parameters at the top of the stack will be pushed back, \
        // and the results will be stored in register ax

        else if (op == OR)   { ax = *sp++ | ax; }
        else if (op == XOR)  { ax = *sp++ ^ ax; }
        else if (op == AND)  { ax = *sp++ & ax; }
        else if (op == EQ)   { ax = *sp++ == ax;}

        else if (op == NE)   { ax = *sp++ != ax;}
        else if (op == LT)   { ax = *sp++ < ax; }
        else if (op == LE)   { ax = *sp++ <= ax;}
        else if (op == GT)   { ax = *sp++ > ax; }
        else if (op == GE)   { ax = *sp++ >= ax;}

        else if (op == SHL)  { ax = *sp++ << ax;}
        else if (op == SHR)  { ax = *sp++ >> ax;}
        else if (op == ADD)  { ax = *sp++ + ax; }
        else if (op == SUB)  { ax = *sp++ - ax; }
        else if (op == MUL)  { ax = *sp++ * ax; }
        else if (op == DIV)  { ax = *sp++ / ax; }
        else if (op == MOD)  { ax = *sp++ % ax; }

        // ----------------------------------------------------------------

        // *!* Built-in functions
        // ----------------------------------------------------------------
        // Using C functions to bootstrap the program for supporting the virtual machine

        else if (op == EXIT) { printf("Exit(%d)", *sp); return *sp; }
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp); }
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp);}

        else if (op == PRTF)
        {
            tmp = sp + pc[1];
            ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
        }

        else if (op == MALC) { ax = (int)malloc(*sp); }
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp); }
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp); }
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
    int i, fd; // The file descriptor
    int *tmp;

    argc--;
    argv++;

    poolSize = 512 * 1024; // Arbitrary memory size, 512KB
    line = 1;


    // Opening the file
    if ((fd = open(*argv, 0)) < 0)
    {
        // Error opening
        printf("The Tiny-C-Interpreter could not open (%s)\n", *argv);
        return -1;
    }

    // ---------------------------------------------------------------------
    // Creating the virtual machine

    // Allocating the memory for virtual machine
    if (!(text = old_text = malloc(poolSize)))
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for text area\n", poolSize);
        return -1;
    }

    if (!(data = malloc(poolSize)))
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for data area\n", poolSize);
        return -1;
    }

    if (!(stack = malloc(poolSize)))
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for stack area\n", poolSize);
        return -1;
    }

    if (!(symbols = malloc(poolSize)))
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for symbol table\n", poolSize);
        return -1;
    }

    memset(text   , 0, poolSize);
    memset(data   , 0, poolSize);
    memset(stack  , 0, poolSize);
    memset(symbols, 0, poolSize);

    // Instruction set
    bp = sp = (int *)((int)stack + poolSize);
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

    // Adding the keyword/library to the symbol table before parsing

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";

    // Adding keyword
    i = Char;
    while( i <= While )
    {
        next(); current_id[Token] = i++;
    }

    // Adding library
    i = OPEN;
    while( i <= EXIT )
    {
        next();
        current_id[Class] = Sys;
        current_id[Type]  = INT;
        current_id[Value] = i++;   
    }

    next(); current_id[Token] = Char; // Handling void type
    next(); idmain = current_id; // Keeping track of main


    // ----------------------------------------------------------------
    // Reading the source code file

    // Opening the file
    if ((fd = open(*argv, 0)) < 0)
    {
        // Error opening
        printf("The Tiny-C-Interpreter could not open (%s)\n", *argv);
        return -1;
    }
   

    // Allocating the memory
    if (!(src = old_src = malloc(poolSize)))
    {
        // Error allocating memory
        printf("The Tiny-C-Interpreter could not allocate (%d) for source area\n", poolSize);
        return -1;
    }

    // Now begin to read the source code and interpreter the code
    if ((i = read(fd, src, poolSize - 1)) <= 0)
    {
        // Error reading
        printf("The read() return %d\n", i);
        return -1;
    }

    // Adding the EOF character
    src[i] = 0;
    close(fd);

    program();

    if( !(pc = (int *)idmain[Value]) )
    {
        printf("The main() function not defined\n");
        return -1;
    }

    // Setting stack
    sp = (int *)((int)stack + poolSize);
    *--sp = EXIT; // call exit if main returns
    *--sp = PUSH; tmp = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)tmp;

    return eval();

}
