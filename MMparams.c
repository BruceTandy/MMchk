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
// Process the parameters and set up the puzzle to solve
//
#include "MMparams.h"
#include "MMchk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

// Set up processing from parameters that may be passed when invoking the program
// See help text for details  (run MMopt -h)
int setup( Repo* pRepo, int argc, char **argv )
{
    FILE* fp           = NULL;
    char  baseName[256];
    char  dirName[256];
    int   p            = 0;
    int   c            = 0;
    int   i            = 0;
    int   rc           = 0;

    // Initialise repository
    pRepo->filename    = NULL;
    pRepo->fp          = NULL;
    pRepo->pegs        = 0;
    pRepo->pegsOK      = true;
    pRepo->colours     = 0;
    pRepo->coloursOK   = true;
    pRepo->codes       = 0;
    pRepo->actualCodes = 0;
    pRepo->codesOK     = false;
    pRepo->guesses     = 8;
    pRepo->headerOK    = false;
    pRepo->codeDefs    = NULL;
    pRepo->marking     = NULL;
    pRepo->data        = NULL;
    pRepo->missing     = NULL;

    // Expecting one parameter, which should be a filename
    if( argc == 2 && strlen( argv[1] ) > 0 )
        pRepo->filename = argv[1];
    else
        pRepo->filename = "/Users/brucetandy/Documents/Mastermind/Results/SolnMM(2,2)_full_951211835042434.csv";  // DEBUG

    if( pRepo->filename != NULL )
    {
        pRepo->fp = fopen( pRepo->filename, "r" );
        if( pRepo->fp != NULL )
        {
            for( p = 0; p < 256; p++ )
            {
                baseName[p] = '\0';   // fully clear the baseName
                dirName[p]  = '\0';   // and dirName
            }
            p = strlen( pRepo->filename ) - 1;
            while( p >= 0 && pRepo->filename[p] != '/' ) p--;
            if( p >= 0 && pRepo->filename[p] == '/' )
            {
                strcpy( baseName, &pRepo->filename[p+1] );
                strcpy( dirName,  pRepo->filename );
                dirName[p] = '\0';
            }
            else
            {
                strcpy( baseName, pRepo->filename );
                dirName[0] = '\0';
            }

            // Parse the number of pegs and colours from the file name
            // However, it's not a fatal error if the file has been renamed
            if( baseName[6] == '(' )
            {
                if( baseName[7] >= '0' && baseName[7] <= '9' )
                {
                    if( baseName[8] == ',' )
                    {
                        pRepo->pegs = baseName[7] - '0';
                        p = 9;
                    }
                    else if( baseName[8] >= '0' && baseName[8] <= '9' && baseName[9] == ',' )
                    {
                        pRepo->pegs = ( baseName[7] - '0' ) * 10 + baseName[8] - '0';
                        p = 10;
                    }
                    else
                    {
                        fprintf( stderr, "Filename does not have the expected format: %s\n", baseName );
                        fprintf( stderr, "%52s\n", "^" );  // 44 + 8
                    }

                    if( baseName[p] >= '0' && baseName[p] <= '9' )
                    {
                        if( baseName[p+1] == ')' )
                        {
                            pRepo->colours = baseName[p] - '0';
                        }
                        else if( baseName[p+1] >= '0' && baseName[p+1] <= '9' && baseName[p+2] == ')' )
                        {
                            pRepo->colours = ( baseName[p] - '0' ) * 10 + baseName[p+1] - '0';
                        }
                        else
                        {
                            fprintf( stderr, "Filename does not have the expected format: %s\n", baseName );
                            fprintf( stderr, "%50s\n", "^" );  // 44 + 6
                        }
                    }
                    else
                    {
                        fprintf( stderr, "Filename does not have the expected format: %s\n", baseName );
                        fprintf( stderr, "%50s\n", "^" );  // 44 + 6
                    }
                }
                else
                {
                    fprintf( stderr, "Filename does not have the expected format: %s\n", baseName );
                    fprintf( stderr, "%50s\n", "^" );  // 44 + 6
                }
            }
            else
            {
                fprintf( stderr, "Filename does not have the expected format: %s\n", baseName );
                fprintf( stderr, "%50s\n", "^" );  // 44 + 6
            }
        }
        else
        {
            fprintf( stderr, "Filename \"%s\"is invalid", argv[1] );
            return -1;
        }
    }
    else
    {
        helpText( pRepo );
        return -1;
    }
    return 0;
}

// Setup all of the possible codes including useful information about each - such as the colours in that code
int setupCodeDefs( Repo* pRepo )
{
    unsigned char colour[pRepo->pegs];
    unsigned int  sel      = 0;
    unsigned char p        = 0;
    unsigned char c        = 0;

    // Set up a reference array containing all possible codes
    // Note, we do this in a way that will work irrespective of the number of pegs

    // Create the array
    pRepo->codeDefs = (CodeDef*)malloc(sizeof(CodeDef)*pRepo->codes);
    if( pRepo->codeDefs == NULL )
    {
        fprintf( stderr, "Failed to allocate the codeDefs array\n" );
        return 1;
    }

    // Initialise the colours used in the first code
    for( p=0; p<pRepo->pegs; p++ ) colour[p] = 0;

    // Now set up the code and then find the next one
    for( sel = 0; sel < pRepo->codes; sel++)
    {
        // Set up current code and colour frequencies
        for( c=0; c<pRepo->colours; c++ ) pRepo->codeDefs[sel].colourFrequency[c] = 0;
        for( p=0; p<pRepo->pegs; p++ )
        {
            pRepo->codeDefs[sel].peg[p] = colour[p];
            pRepo->codeDefs[sel].colourFrequency[colour[p]] += 1;
        }

        // Find the next code
        colour[0] += 1;
        for( p=0; p<pRepo->pegs; p++ )
        {
            if( colour[p] >= pRepo->colours )
            {
                colour[p] = 0;
                if( p+1<pRepo->pegs ) colour[p+1] += 1;
            }
        }
    }
    return 0;
}

/**********************************************************************************************************************
Work out every mark up front.
This function sets up a global, 2 dimensional array.
The 2 dimensions represent the guess and the code, the array contains the marking.
(Marks are symetrical - so A vs B has the same marking as B vs A)

Note that each mark is given an integer value according to the markTranslation matrix
This is a slightly odd ordering in order to achieve the following objectives:
  1/ Consistency beween puzzles with different numbers of pegs
  2/ Contiguous range for each number of pegs
  3/ AllBlack is always the highest index in the range for a set of pegs (Marks-1)

 Errors can result in a non-zero return
**********************************************************************************************************************/
int setupMarks( Repo* pRepo )
{
    unsigned int  guess    = 0;
    unsigned int  solution = 0;
    unsigned char black    = 0;
    unsigned char white    = 0;
    unsigned char total    = 0;
    unsigned int  i        = 0;
    int           rc       = 0;
    unsigned char guessColours[pRepo->colours];
    unsigned char solutionColours[pRepo->colours];

                                                                        // Use a numbering scheme so that for each number of pegs, there is a contiguous range of marks
    unsigned char markTranslation[MAX_PEGS+1][MAX_PEGS+1] = {           // Use  marking[black, white]
                                                            {  0,  2,  3,  5,  9, 14, 20, 27, 35, 44, 54 }   // 0 black pegs; 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 and 10 white pegs
                                                          , {  1,  6,  7, 10, 15, 21, 28, 36, 45, 55, XX }   // 1 black peg;  0, 1, 2, 3, 4, 5, 6, 7, 8 and 9 white pegs
                                                          , {  4, 11, 12, 16, 22, 29, 37, 46, 56, XX, XX }   // 2 black pegs; 0, 1, 2, 3, 4, 5, 6, 7 and 8 white pegs
                                                          , {  8, 17, 18, 23, 30, 38, 47, 57, XX, XX, XX }   // 3 black pegs; 0, 1, 2, 3, 4, 5, 6 and 7 white pegs
                                                          , { 13, 24, 25, 31, 39, 48, 58, XX, XX, XX, XX }   // 4 black pegs; 0, 1, 2, 3, 4, 5 and 6 white pegs
                                                          , { 19, 32, 33, 40, 49, 59, XX, XX, XX, XX, XX }   // 5 black pegs; 0, 1, 2, 3, 4 and 5 white pegs
                                                          , { 26, 41, 42, 50, 60, XX, XX, XX, XX, XX, XX }   // 6 black pegs; 0, 1, 2, 3 and 4 white pegs
                                                          , { 34, 51, 52, 61, XX, XX, XX, XX, XX, XX, XX }   // 7 black pegs; 0, 1, 2 and 3 white pegs
                                                          , { 43, 62, 63, XX, XX, XX, XX, XX, XX, XX, XX }   // 8 black pegs; 0, 1 and 2 white pegs
                                                          , { 53, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX }   // 9 black pegs; No white pags (can't have 9 black, 1 white)
                                                          , { 64, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX }   // 10 black pegs
                                                          };
    // Setup global array indication the mark obtained when submitting each guess to each solution
    pRepo->marking = (char**)malloc(sizeof(char*) * pRepo->codes);
    if( pRepo->marking != NULL )
    {
        for( i = 0; i < pRepo->codes; i++ )
        {
            pRepo->marking[i] = malloc( sizeof(char) * ( i + 1 ) );
            if( pRepo->marking[i] == NULL )
            {
                fprintf(stderr, "Failure whilst malloc'ing parts of glabal 'marking' array\n");
                return 1;
            }
        }
    }
    else
    {
        fprintf(stderr, "Failure whilst malloc'ing glabal 'marking' array\n");
        return 1;
    }

    for( guess = 0; guess < pRepo->codes; guess++ )
    {
        for( solution = guess; solution < pRepo->codes; solution++ )
        {
            // Clear counts from last time
            black = white = 0;

            // Special case, if the guess and the solution are the same, we have an exact match, ie all black
            if( guess == solution )
            {
                black = pRepo->pegs;
            }
            else
            {
                // We will count the number of times each colour appears in both the guess and the solution
                // Here we initialise the arrays of counts
                for(i=0; i<pRepo->colours; i++)
                {
                    guessColours[i] = 0;
                    solutionColours[i] = 0;
                }

                // Find the number of exact matches (ie black marking pegs)
                // Also count how many of each colour in both the guess and the solution
                for(i=0; i<pRepo->pegs; i++)
                {
                    if(pRepo->codeDefs[guess].peg[i] == pRepo->codeDefs[solution].peg[i])
                        black += 1;
                    guessColours[ pRepo->codeDefs[guess].peg[i] ]++;
                    solutionColours[ pRepo->codeDefs[solution].peg[i] ]++;
                }

                // Work out the overlap of colours - irrespective of placement
                // Then the number of white marker pegs must be the overlap less the black marker pegs
                total = 0;
                for(i=0; i<pRepo->colours; i++)
                {
                    total += solutionColours[i] < guessColours[i] ? solutionColours[i] : guessColours[i];
                }
                white = total - black;
            }

            // Set up marks in both dimensions of the array
            pRepo->marking[solution][guess] = markTranslation[black][white];

            if(pRepo->marking[solution][guess] == XX)
            {
                // This represents an error, print out the guess and solution that caused the problem and incrament the return code
                fprintf(stderr, "Error in function mark() guess = %d, solution = %d\n", guess, solution);
                return 1;
            }
        }
    }
    return rc;
}

void helpText( Repo* pRepo )
{
    printf( "Program to check the validity of a Mastermind solution\n" );
    printf( "One parameter is manditory, that is the name (and path if necessary) of a solution file, as written by MMopt\n" );
    printf( " ( For example SolnMM(4,6)_full_282970100085955.csv )\n" );
    printf( " ( Or /Users/brucetandy/Documents/Mastermind/Results/SolnMM(4,6)_full_282970100085955.csv )\n" );
    printf( "\n" );
    printf( "This program will produce a short report to stdout giving details of number of pegs, number of colours..\n" );
    printf( "..as well as the completeness and validity of the solution.\n" );
    printf( "\n" );
    printf( "If the solution is not satisfactory, a list of codes not resolved and a list of erroneous resolutions will be reported.\n" );
    printf( "\n" );
    printf( "This program makes no statement or claim about whether a solution is optimal or not\n" );
    printf( "\n" );

    return;
}