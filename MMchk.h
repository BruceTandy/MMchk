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
// Key Include file defining all the main structures and constants
// All code files include this
//
#ifndef MMCHK_H
#define MMCHK_H

#include <stdio.h>
#include <stdbool.h>

// Program identification information
#define MAYOR_VERSION          0
#define MINOR_VERSION          1
#define FIX_VERSION            1
#define RELEASE_DAY            28
#define RELEASE_MONTH          05
#define RELEASE_YEAR           2024
#define PROGRAM_NAME           "Mastermind Solution Checker"

#define XX                     -1                      // Rogue value for marking scheme (char)
#define STOP                   65535                   // Rogue value for codes (unsigned short)
#define MAX_PEGS               10
#define MAX_COLOURS            10

// Structure pre-declarations
struct Repo;
struct CodeDef;
struct Turn;
struct Solution;
struct Absent;
struct CodeDef;

// Root structure used to hold all of the puzzle parameters and to point to structures used in finding the best solution
typedef struct Repo
{
    char*            filename;                       // Filename to analyse
    FILE*            fp;                             // Open file pointer
    char             pegs;                           // Number of pegs in code
    bool             pegsOK;                         // Do we have a consistent view of the numbers of pegs?
    char             colours;                        // Number of colours in code
    bool             coloursOK;                      // Do we have a consistent view of the numbers of colours?
    int              codes;                          // Expected number of codes
    int              actualCodes;                    // Actual number of codes in solution file
    bool             codesOK;                        // Did we get the expected number of codes?
    int              guesses;                        // Max number of guesses
    bool             headerOK;                       // Is the header there and as expected
    struct CodeDef*  codeDefs;                       // Static information about each code
    char**           marking;                        // Two dimensional array holding marks
    struct Solution* data;                           // All data held in file being analysed (except headers)
    struct Absent*   missing;                        // List of missing codes
} Repo;

// A turn consists of a guess and a mark
typedef struct Turn
{   
    int  guess;
    bool guessOK;
    bool guessIllegal;
    int  mark;
    bool markOK;
    bool used;
} Turn;

// A Solution consists the code to be guessed, an array of turns and the number of turns taken to resolve
typedef struct Solution
{   
    int          code;
    bool         codeOK;
    bool         codeRepeated;
    int          noTurns;
    bool         turnsOK;
    int          actualNoTurns;
    bool         resolved;
    bool         marksOK;
    bool         guessesOK;
    bool         guessConsistant;
    struct Turn* turns;
} Solution;

// A Solution consists the code to be guessed, an array of turns and the number of turns taken to resolve
typedef struct Absent
{
    int          code;
    bool         codeMissing;
} Absent;

// Definition of each possible code
typedef struct CodeDef
{
    char peg[MAX_PEGS];                                // Code array of coloured pegs
    char colourFrequency[MAX_COLOURS];                 // How many times each colour is used in this code 
} CodeDef;

int main( int argc, char **argv );
int parseHeader( Repo* pRepo );
int countPegs( Repo* pRepo );
int countCodes( Repo* pRepo );
int parseFile( Repo* pRepo );
int checkCodes( Repo* pRepo );
int checkCounts( Repo* pRepo );
int checkGuesses( Repo* pRepo );
int checkMarks( Repo* pRepo );
int report( Repo* pRepo );
int parseCode( Repo* pRepo, char* szCode );
int getField( FILE* fp, char* field, int maxLen );
int nextField( char* string, char* field, int maxLen );
int getLine( FILE* fp, char* line, int maxLen );

#endif   /* MMCHK_H */
