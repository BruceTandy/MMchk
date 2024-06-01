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
// Output details of the solution found to file
//
#include "MMoutputsolution.h"
#include "MMopt.h"
#include "MMutility.h"
#include "MMsolver.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// Set up the header for the solution output file
int outputSolutionHeader( Puzzle* pPuzzle )
{
    // Open a file and add the header information as well as the initial guess
    struct stat    fileInfo;
    FILE*          fp  = NULL;  
    struct passwd* pwd = NULL;
    int            i   = 0;
    int            d   = 0;
    int            rc  = 0;
    struct tm*     timeInfo = NULL;

    d = sprintf( pPuzzle->szSolnFile, "%s%d,%d)_", SOLN, pPuzzle->pegs, pPuzzle->colours );
    if( pPuzzle->algorithm != NULL )
    {   // Copy algorithm parameter - but edit out any '.' or '/' characters as they can be problematic with file names
        for( i = 0; pPuzzle->algorithm[i] != '\0'; i++ )
            if( pPuzzle->algorithm[i] != '/' && pPuzzle->algorithm[i] != '.' )
                pPuzzle->szSolnFile[d++] = pPuzzle->algorithm[i];
        pPuzzle->szSolnFile[d] = '\0';
    }
    else
    {
        strcat( pPuzzle->szSolnFile, "full" );
    }

    // Add the time taken when the program started as a uniqueness factor in filename
    sprintf( pPuzzle->szSolnFile+strlen(pPuzzle->szSolnFile), "_%ld%09ld%s", pPuzzle->baseRealTime.tv_sec, pPuzzle->baseRealTime.tv_nsec, EXTENSION );

    fp = fopen( pPuzzle->szSolnFile, "w");
    if( fp != NULL )
    {
        fprintf(fp, "#,Solution,Turns");
        for( i = 0; i < pPuzzle->maxTurnsToSolve; i++ )
        {
            fprintf(fp, ",Guess %d,Mark %d", i+1, i+1 );
        }
        fprintf(fp, "\n");
        if( fclose(fp) != 0 ) fprintf( stderr, "ERROR closing file in outputSolutionHeader\n" );
        fp = NULL;
    }
    else
    {
        fprintf(stderr, "Can't open Solution file (Writing header)\n");
        rc = 2;
    }

    return rc;
}

// Recursive function to walk the Guess structure and output all of the results.
// Only use one string to output details of guesses and marks for all games.
// To better facilitate this the start points of various points in the string are marked so that the string may be truncated
// back to these points to make changes for the next case
// This routine is additionally complicated by the treatment of problems with 3 or less remaining possible solutions
// Such problems are left only partially resolved, specifically with no further data structures.
// This is done to save both processing time and memory - both of which become increasingly critical as problem complexity rises
// For example it is worth noting that in solving MM(4,6) ie classic Mastermind, 168 instances of the principle data structures
// are required if this 3 or less approach is taken.
// However, if all solutions are expressed in the data structures then 1339 instances are required - which is a 797% increase!
// With 3 or less solutions remaining it is very easy to work out how many more guesses are required.
// Also, enough data is stored to quickly calculate the final moves - which need to be done here, hence the additional complexity
//
// Consideration was given to breaking this routine into a series of smaller routines.
// However, the code consists of a 'shopping' list of situations so there is a natural structure that is fairly easy to read.
// In addition, a common code block to write out a guess and its mark have been moved to a function - improving overall readability
// Therefore it was felt that this provided a readable and maintainable function dispite its size
//
// NB - a convenient way to locate the end of a string has been much used...
// This requires strchr to seach the string for the first occurance of a null charater
// - returning a pointer to this place in the string.  Very useful!
int outputSolution( Puzzle* pPuzzle, Guess* pGuess, char* pDetails, int turns )
{
    char*  pStartPoint = NULL;
    char*  pMarkPoint  = NULL;
    char*  pGuessPoint = NULL;
    int    i           = 0;
    int    c           = 0;
    int    code[3];
    bool   inCodeList  = true;
    int    rc          = 0;

    pStartPoint = strchr( pDetails, '\0' );
    strcat( pDetails, "," );
    printCode( pPuzzle, pGuess->code, pGuess->inCodeList, strchr( pStartPoint, '\0' ) );
    strcat( pStartPoint, "," );
    pMarkPoint = strchr( pStartPoint, '\0' );
    turns += 1;

    for( i = 0; i < pPuzzle->marks; i++ )
    {
        if( pGuess->problem[i].size > 0 )
        {
            *pMarkPoint = '\0';                                 // Ensure we cut off any additional text at this point
            strcat( pMarkPoint, pPuzzle->markString[i] );       // We have a mark for pGuess - so output it
            if( i != pPuzzle->allBlack )
            {
                // If the mark is not All Black, we will need to add to it, so mark this point in the string
                // NB, must be before comma, as ouputSolution always adds the comma, so recursive calls cannot start with a comma in place
                pGuessPoint = strchr( pStartPoint, '\0' );

                switch( pGuess->problem[i].size )
                {
                    case 1:         // One more guess required, add it plus mark and then output
                        code[0] = pGuess->problem[i].firstCode;
                        outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, pPuzzle->allBlack );
                        rc = outputSolutionLine( pPuzzle, code[0], pDetails, turns + 1 );
                        *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                        break;
                    case 2:         // Two solutions remain, output solutions for both
                        code[0] = pGuess->problem[i].firstCode;
                        code[1] = pGuess->problem[i].pCommon->codeList[code[0]];

                        // First take the case where code[1] is the solution and we guess code[0] then code[1]
                        outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, marking( pPuzzle, code[0], code[1] ) );
                        outputGuessAndMark( pPuzzle, pGuessPoint, code[1], true, pPuzzle->allBlack );
                        rc = outputSolutionLine( pPuzzle, code[1], pDetails, turns + 2 );
                        *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark

                        // Then take the case where code[0] is the solution and we guess code[0]
                        outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, pPuzzle->allBlack );
                        rc = outputSolutionLine( pPuzzle, code[0], pDetails, turns + 1 );
                        *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                        break;
                    case 3:         // Three solutions remain
                        // In the case of 3 solutions remaining we will have specifically looked for a non-code list split (minor split)
                        // And set the lateStageGuess if one is found (this is done late, in maxTurnsToSolve)
                        // We do this because in the case of 3 remaining solutions we can work out how many turns remain (5 or 6)
                        // ...without doing further work on the solution other than searching for a major split
                        code[0] = pGuess->problem[i].firstCode;
                        code[1] = pGuess->problem[i].pCommon->codeList[code[0]];
                        code[2] = pGuess->problem[i].pCommon->codeList[code[1]];

                        // Work out if lateStageGuess is in the code list, and if it is, ensure it is code[0] - swapping as required
                        if( pGuess->problem[i].lateStageGuess == code[0] )
                        {
                            inCodeList = true;
                        }
                        else
                        {
                            if( pGuess->problem[i].lateStageGuess == code[1] )
                            {
                                inCodeList = true;
                                code[1] = code[0];
                                code[0] = pGuess->problem[i].lateStageGuess;
                            }
                            else
                            {
                                if( pGuess->problem[i].lateStageGuess == code[2] )
                                {
                                    inCodeList = true;
                                    code[2] = code[1];
                                    code[1] = code[0];
                                    code[0] = pGuess->problem[i].lateStageGuess;
                                }
                                else
                                {
                                    inCodeList = false;
                                }
                            }
                        }

                        if( pGuess->problem[i].lateStageGuess != STOP && inCodeList )
                        {   
                            // This is the best case, one guess is a hit and splits the other two
                            
                            // Step through cases where code[1] then code[2] is the solution.  We guess: code[0], then solution
                            for( c = 1; c < 3; c++ )
                            {
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, marking( pPuzzle, code[0], code[c] ) );
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[c], true, pPuzzle->allBlack );
                                rc = outputSolutionLine( pPuzzle, code[c], pDetails, turns + 2 );
                                *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                            }

                            // Finish with the case where code[0] is the solution and we guess code[0]
                            outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, pPuzzle->allBlack );
                            rc = outputSolutionLine( pPuzzle, code[0], pDetails, turns + 1 );
                            *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                        }
                        else
                        {
                            if( pGuess->problem[i].lateStageGuess != STOP && !inCodeList )
                            {
                                // Second best case, there is a guess not in the code list that splits the remaining 3 solutions

                                // Step through cases where each code is the solution.  We guess: late stage guess, then solution
                                for( c = 0; c < 3; c++ )
                                {
                                    outputGuessAndMark( pPuzzle, pGuessPoint, pGuess->problem[i].lateStageGuess, false, marking( pPuzzle, pGuess->problem[i].lateStageGuess, code[c] ) );
                                    outputGuessAndMark( pPuzzle, pGuessPoint, code[c], true, pPuzzle->allBlack );
                                    rc = outputSolutionLine( pPuzzle, code[c], pDetails, turns + 2 );
                                    *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                                }
                            }
                            else
                            {
                                // Worst case, we hit one, but don't split the other two 

                                // First the case where code[2] is the solution and we guess: code[0], code[1] then code[2]
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, marking( pPuzzle, code[0], code[2] ) );
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[1], true, marking( pPuzzle, code[1], code[2] ) );
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[2], true, pPuzzle->allBlack );
                                rc = outputSolutionLine( pPuzzle, code[2], pDetails, turns + 3 );
                                *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark

                                // Now take the case where code[1] is the solution but we still initially guess code[0]
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, marking( pPuzzle, code[0], code[1] ) );
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[1], true, pPuzzle->allBlack );
                                rc = outputSolutionLine( pPuzzle, code[1], pDetails, turns + 2 );
                                *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark

                                // Finally take the case where code[0] is the solution and we guess code[0]
                                outputGuessAndMark( pPuzzle, pGuessPoint, code[0], true, pPuzzle->allBlack );
                                rc = outputSolutionLine( pPuzzle, code[0], pDetails, turns + 1 );
                                *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                            }
                        }
                        break;
                    default:        // More than 3 solutions remain, we may have a split or we may need to continue to walk the tree
                        if( pGuess->problem[i].lateStageGuess != STOP )
                        {
                            // This (late stage) guess will differentiate all other solutions
                            // So for every remaining solution, we need to output the late stage guess and mark
                            // The late stage guess may either be one of the remaining solutions or not
                            // If it is one of the remaining solutions then we need to output that as well
                            for( c = pGuess->problem[i].firstCode; c != STOP; c = pGuess->problem[i].pCommon->codeList[c] )
                            {
                                if( c != pGuess->problem[i].lateStageGuess )
                                {
                                    outputGuessAndMark( pPuzzle, pGuessPoint, pGuess->problem[i].lateStageGuess, true, marking( pPuzzle, pGuess->problem[i].lateStageGuess, c ) );
                                    outputGuessAndMark( pPuzzle, pGuessPoint, c, true, pPuzzle->allBlack );
                                    rc = outputSolutionLine( pPuzzle, c, pDetails, turns + 2 );
                                    *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                                }
                            }
                            // Finally take the case where the late stage guess is the solution
                            if( pGuess->problem[i].pCommon->partitionList[pGuess->problem[i].lateStageGuess] == pGuess->problem[i].mark )
                            {
                                outputGuessAndMark( pPuzzle, pGuessPoint, pGuess->problem[i].lateStageGuess, true, pPuzzle->allBlack );
                                rc = outputSolutionLine( pPuzzle, pGuess->problem[i].lateStageGuess, pDetails, turns + 1 );
                                *pGuessPoint = '\0';       // Truncate the constructed string ready for the next mark
                            }
                        }
                        else
                            outputSolution( pPuzzle, pGuess->problem[i].pGuessList, pDetails, turns );
                        break;
                }
            }
            else
            {
                // This is a solution, so output the details
                rc = outputSolutionLine( pPuzzle, pGuess->code, pDetails, turns );
            }
        }
    }
   *pStartPoint = '\0';    // Finally truncate back to the way we found things
   return 0;
}

// Convenience routine to output a guess and a mark to the end of the given string
int outputGuessAndMark( Puzzle* pPuzzle, char* pBuffer, int code, bool inCodeList, char mark )
{
    strcat( pBuffer, "," );
    printCode( pPuzzle, code, inCodeList, strchr( pBuffer, '\0' ) );  // Add new guess
    strcat(pBuffer, ",");
    strcat( pBuffer, pPuzzle->markString[mark] );                     // And its mark
    return 0;
}

// Output the details for one solution
int outputSolutionLine( Puzzle* pPuzzle, int code, char* pDetails, int turns )
{
    char           pcBuf[pPuzzle->pegs+3];
    FILE*          fp   = NULL;  
    int            rc   = 0;

    fp = fopen( pPuzzle->szSolnFile, "a");
    if( fp != NULL )
    {
        fprintf( fp, "%d,%s,%d%s\n", code, printCode(pPuzzle, code, true, pcBuf), turns, pDetails );
        if( fclose(fp) != 0 ) fprintf( stderr, "ERROR closing file in outputSolution\n" );
        fp = NULL;
    }
    else
    {
        fprintf(stderr, "Can't open Solution file\n");
        rc = 2;
    }

    return rc;
}