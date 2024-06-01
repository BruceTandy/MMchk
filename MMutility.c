/******************************************************************************************************************/
//  This is part of a program to find optimal or near optimal solutions to Mastermind games of varying complexity
//  The specific puzzle to be solved and method employed may be configured using a series of parameters
//  For details about the parameters please run:   MMopt -h
//  
//  The author of this code is myself  Bruce Tandy
//  My contact details are bruce.tandy@btinternet.com
//
//  I would be very interested to hear your feedback about this program and results you have obtained from it
/******************************************************************************************************************/
//
// Miscellaneous utility functions
//
#include "MMutility.h"
#include "MMchk.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Convert a string to an integer
// Only positive integers are allowed (including zero)
// Maximum value allowable is 999,999,999
// Handle error situations and return -1 in the case of an error
int stringToInt( char* str )
{
    int   i   = 0;
    int   len = 0;
    long  val = 0;
    bool  err = false;

    len = strlen( str );
    if( len > 0 && len < 10 )
    {
        for( i = 0; i < len && !err; i++ )
        {
            if( str[i] >= '0' && str[i] <= '9' )
            {
                val = (val * 10)+str[i]-'0';
            }
            else
            {
                err = true;
            }
        }
    }
    else
    {
        err = true;
    }

    if( !err )
    {
        if( val > 999999999 )
        {
            err = true;
        }
    }

    if( err )
    {
        val = -1;
    }

    return (int)val;
}

// Construct a string that represents a specified code
// If the second parameter is false (to show that the code is not feasible), then mark the code in perenthesis
// NOTE - There MUST be Pegs+3 bytes space available in the string - or it will crash
char* printCode( Repo* pRepo, unsigned short code, bool feasible, char* pcBuf )
{
    int posn = 0;
    int i    = 0;

    if( pcBuf != NULL)
    {
        if( !feasible ) pcBuf[posn++] = '(';

        for( i = pRepo->pegs; i > 0; i-- )
        {
            pcBuf[posn++] = 'A'+pRepo->codeDefs[code].peg[i-1];    // Display as letters, but can easily change to numbers
        }
        if( !feasible ) pcBuf[posn++] = ')';
        pcBuf[posn] = '\0';
    }

    return pcBuf;
}

// Return the mark awarded for the given guess and solution
// Note, this is functionalised so that the larger parameter can be passed first
// This allows the marking array to be effectively halved in size
char marking( Repo* pRepo, unsigned short guess, unsigned short solution )
{
    return guess > solution ? pRepo->marking[guess][solution] : pRepo->marking[solution][guess];
}

// Determine the code value for the string provided
// The string may be upper or lower case (or a mix)
// If the string does not match up with a valid code then return STOP
// Note that any additional characters after the expected number will be ignored
// (Therefore "ABCD" will be seen as the same as "ABCDE" if only 4 pegs are expected)
unsigned short getCode( Repo* pRepo, char* codeString )
{
    short          i      = 0;
    int            colour = 0;
    unsigned short code   = 0;

    if( strlen( codeString ) < pRepo->pegs ) return STOP;
    for( i = 0; i < pRepo->pegs; i++ )
    {
        if( codeString[i] >= 'A' && codeString[i] <= 'Z' ) colour = codeString[i] - 'A';
        else if( codeString[i] >= 'a' && codeString[i] <= 'z' ) colour = codeString[i] - 'a';
        else return STOP;

        if( colour >= pRepo->colours ) return STOP;

        code *= pRepo->colours;
        code += colour;
    }
    return code;
}

// Determine the mark represented by the string provided
// The string must only contain the characters b, w or - (upper or lower case)
// (If the string contains '-', it must be the only character)
// If the string does not match up with a valid mark then return -1
//
// Unfortunately, this function takes a 'blunderbuss' approach to matching the mark string with the mark
// The actual matk strings may not have been set up when this is called, so a 'cludge' approach was taken for expediency
int getMark( Repo* pRepo, char* markString )
{
    char  cleanMark[ pRepo->pegs+1 ];
    int   i       = 0;
    int   markLen = 0;
    short black   = 0;
    short white   = 0;

    markLen = strlen( markString );
    if( markLen > pRepo->pegs ) return -1;

    for( i = 0; i < markLen; i++ )
    {
        if( markString[i] == 'b' || markString[i] == 'B' ) black += 1;
        else if( markString[i] == 'w' || markString[i] == 'W' ) white += 1;
        else if( markString[i] != '-' || i != 0 ) return -1;                // Error if '-' is in string and not in first position
        else if( markLen > 1 ) return -1;                                   // Error if '-' is followed by other characters
    }

    for( i = 0; i < black; i++ )           cleanMark[i]   = 'b';
    for( i = black; i < black+white; i++ ) cleanMark[i]   = 'w';
    if( black+white > 0 )
        cleanMark[black+white] = '\0';
    else
        return 0;           // Must be a no score mark - represented by 0;

    if( strcmp(cleanMark, "b"          ) == 0 ) return 1;
    if( strcmp(cleanMark, "w"          ) == 0 ) return 2;
    if( strcmp(cleanMark, "ww"         ) == 0 ) return 3;
    if( strcmp(cleanMark, "bb"         ) == 0 ) return 4;
    if( strcmp(cleanMark, "www"        ) == 0 ) return 5;
    if( strcmp(cleanMark, "bw"         ) == 0 ) return 6;
    if( strcmp(cleanMark, "bww"        ) == 0 ) return 7;
    if( strcmp(cleanMark, "bbb"        ) == 0 ) return 8;
    if( strcmp(cleanMark, "wwww"       ) == 0 ) return 9;
    if( strcmp(cleanMark, "bwww"       ) == 0 ) return 10;
    if( strcmp(cleanMark, "bbw"        ) == 0 ) return 11;
    if( strcmp(cleanMark, "bbww"       ) == 0 ) return 12;
    if( strcmp(cleanMark, "bbbb"       ) == 0 ) return 13;      
    if( strcmp(cleanMark, "wwwww"      ) == 0 ) return 14;     
    if( strcmp(cleanMark, "bwwww"      ) == 0 ) return 15;     
    if( strcmp(cleanMark, "bbwww"      ) == 0 ) return 16;     
    if( strcmp(cleanMark, "bbbw"       ) == 0 ) return 17;      
    if( strcmp(cleanMark, "bbbww"      ) == 0 ) return 18;     
    if( strcmp(cleanMark, "bbbbb"      ) == 0 ) return 19;     
    if( strcmp(cleanMark, "wwwwww"     ) == 0 ) return 20;    
    if( strcmp(cleanMark, "bwwwww"     ) == 0 ) return 21;    
    if( strcmp(cleanMark, "bbwwww"     ) == 0 ) return 22;    
    if( strcmp(cleanMark, "bbbwww"     ) == 0 ) return 23;    
    if( strcmp(cleanMark, "bbbbw"      ) == 0 ) return 24;    
    if( strcmp(cleanMark, "bbbbww"     ) == 0 ) return 25;    
    if( strcmp(cleanMark, "bbbbbb"     ) == 0 ) return 26;    
    if( strcmp(cleanMark, "wwwwwww"    ) == 0 ) return 27;   
    if( strcmp(cleanMark, "bwwwwww"    ) == 0 ) return 28;   
    if( strcmp(cleanMark, "bbwwwww"    ) == 0 ) return 29;   
    if( strcmp(cleanMark, "bbbwwww"    ) == 0 ) return 30;   
    if( strcmp(cleanMark, "bbbbwww"    ) == 0 ) return 31;   
    if( strcmp(cleanMark, "bbbbbw"     ) == 0 ) return 32;    
    if( strcmp(cleanMark, "bbbbbww"    ) == 0 ) return 33;   
    if( strcmp(cleanMark, "bbbbbbb"    ) == 0 ) return 34;   
    if( strcmp(cleanMark, "wwwwwwww"   ) == 0 ) return 35;  
    if( strcmp(cleanMark, "bwwwwwww"   ) == 0 ) return 36;  
    if( strcmp(cleanMark, "bbwwwwww"   ) == 0 ) return 37;  
    if( strcmp(cleanMark, "bbbwwwww"   ) == 0 ) return 38;  
    if( strcmp(cleanMark, "bbbbwwww"   ) == 0 ) return 39;  
    if( strcmp(cleanMark, "bbbbbwww"   ) == 0 ) return 40;  
    if( strcmp(cleanMark, "bbbbbbw"    ) == 0 ) return 41;   
    if( strcmp(cleanMark, "bbbbbbww"   ) == 0 ) return 42;  
    if( strcmp(cleanMark, "bbbbbbbb"   ) == 0 ) return 43;  
    if( strcmp(cleanMark, "wwwwwwwww"  ) == 0 ) return 44; 
    if( strcmp(cleanMark, "bwwwwwwww"  ) == 0 ) return 45; 
    if( strcmp(cleanMark, "bbwwwwwww"  ) == 0 ) return 46; 
    if( strcmp(cleanMark, "bbbwwwwww"  ) == 0 ) return 47; 
    if( strcmp(cleanMark, "bbbbwwwww"  ) == 0 ) return 48; 
    if( strcmp(cleanMark, "bbbbbwwww"  ) == 0 ) return 49; 
    if( strcmp(cleanMark, "bbbbbbwww"  ) == 0 ) return 50; 
    if( strcmp(cleanMark, "bbbbbbbw"   ) == 0 ) return 51;  
    if( strcmp(cleanMark, "bbbbbbbww"  ) == 0 ) return 52; 
    if( strcmp(cleanMark, "bbbbbbbbb"  ) == 0 ) return 53; 
    if( strcmp(cleanMark, "wwwwwwwwww" ) == 0 ) return 54;
    if( strcmp(cleanMark, "bwwwwwwwww" ) == 0 ) return 55;
    if( strcmp(cleanMark, "bbwwwwwwww" ) == 0 ) return 56;
    if( strcmp(cleanMark, "bbbwwwwwww" ) == 0 ) return 57;
    if( strcmp(cleanMark, "bbbbwwwwww" ) == 0 ) return 58;
    if( strcmp(cleanMark, "bbbbbwwwww" ) == 0 ) return 59;
    if( strcmp(cleanMark, "bbbbbbwwww" ) == 0 ) return 60;
    if( strcmp(cleanMark, "bbbbbbbwww" ) == 0 ) return 61;
    if( strcmp(cleanMark, "bbbbbbbbw"  ) == 0 ) return 62; 
    if( strcmp(cleanMark, "bbbbbbbbww" ) == 0 ) return 63;
    if( strcmp(cleanMark, "bbbbbbbbbb" ) == 0 ) return 64;

    return -1;
}