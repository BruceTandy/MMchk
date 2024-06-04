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
    // Input file
    char*            filename;                       // Filename to analyse
    char             baseName[256];                  // Filename without path
    char             dirName[256];                   // Directory name
    char             outputName[256];                // Filename to output errors or problems
    FILE*            fp;                             // Open file pointer
    // Parameters
    char             pegs;                           // Number of pegs in code
    char             colours;                        // Number of colours in code
    int              codes;                          // Expected number of codes
    int              actualCodes;                    // Actual number of codes in solution file
    int              guesses;                        // Max number of guesses
    // Correctness flags
    bool             pegsOK;                         // Do we have a consistent view of the numbers of pegs?
    bool             coloursOK;                      // Do we have a consistent view of the numbers of colours?
    bool             codesOK;                        // Did we get the expected number of codes?
    // Sub structures
    struct CodeDef*  codeDefs;                       // Static information about each code
    char**           marking;                        // Two dimensional array holding marks
    struct Solution* data;                           // All data held in file being analysed (except headers)
    struct Absent*   missing;                        // List of missing codes
} Repo;

// A turn consists of a guess and a mark
typedef struct Turn
{   
    // Parameters
    int  guess;                                 // Numeric representation of the guess
    int  mark;                                  // Numeric representation of the mark
    // Correctness flags
    bool guessOK;                               // Is the guess a well formatted guess?
    bool markOK;                                // Is the mark what was expected?
} Turn;

// A Solution consists the code to be guessed, an array of turns and the number of turns taken to resolve
// This is a representation of a file from the solution file
typedef struct Solution
{   
    // Parameters
    int          line;                          // Line of the file that contains this solution (not counting the header)
    int          code;                          // The code solved on this line
    int          noTurns;                       // Number of turns we are told it takes to solve this code
    int          actualNoTurns;                 // Number of turns it actually took (clearly should be the same)
    // Correctness flags
    bool         codeOK;                        // Is the numeric code the same of the alpha representation?
    bool         codeRepeated;                  // Has this code been solved previously?
    bool         turnsOK;                       // Does the number of turns output match the actual number of turns shown?
    bool         resolved;                      // Do the guesses / marks end with all-black?
    bool         marksOK;                       // Are all the given marks accurate?
    bool         guessesOK;                     // Is the format of the guesses ok - ie Guess+Mark, Guess+Mark...
    bool         guessConsistant;               // Is the same guess made for every code after the same mark?
    struct Turn* turns;
} Solution;

// A Solution consists the code to be guessed, an array of turns and the number of turns taken to resolve
typedef struct Absent
{
    int          code;                          // Store details of any missing codes
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
int sgetc( FILE* fp );

#endif   /* MMCHK_H */
