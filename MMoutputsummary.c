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
// Add summary information to a standard file
// Note that a named semaphore is used to protect simultaneous access to the file by more than one program
//
#include "MMoutputsummary.h"
#include "MMopt.h"
#include "MMsimilarSolver.h"
#include "MMsymmetries.h"
#include "MMutility.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

// Prepare for all file output
// This includes creating directories if necessary and changing to default to that directory
int outputSetup( Puzzle* pPuzzle )
{
    // All output will be sent to a common directory.
    // Check that this directory exists, and create it if not
    // Then a single summary file will be created if not already there so output can be added to it
    char           szDir[MAX_DIR];
    char           szFile[MAX_DIR];
    struct stat    fileInfo;
    FILE*          fp  = NULL;  
    struct passwd* pwd = NULL;
    int            rc  = 0;

    // Find the home directory, then check and create the subdirectories required, and finally change to the subdirectory
    pwd = getpwuid(getuid());
    if( pwd == NULL ) { fprintf(stderr, "Can't get home directory\n"); return -1; }
    sprintf( szDir, "%s%s", pwd->pw_dir, BASE_DIR1 );
    if( stat( szDir, &fileInfo) != 0 )
        if( mkdir( szDir, 0755 ) != 0 ) { fprintf(stderr, "Can't make directory %s\n", szDir); return -1; }
    strcat( szDir, BASE_DIR2 );
    if( stat( szDir, &fileInfo) != 0 )
        if( mkdir( szDir, 0755 ) != 0 ) { fprintf(stderr, "Can't make directory %s\n", szDir); return -1; }
    strcat( szDir, BASE_DIR3 );
    if( stat( szDir, &fileInfo) != 0 )
        if( mkdir( szDir, 0755 ) != 0 ) { fprintf(stderr, "Can't make directory %s\n", szDir); return -1; }
    if( chdir( szDir ) != 0 )           { fprintf(stderr, "Can't make directory %s\n", szDir); return -1; }

    // Open (and create if necessary) a named, binary semaphore that will be used to stop two processes accessing the Summary file at the same time
    pPuzzle->summaryLock = sem_open( SUMMARYLOCKNAME, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, 1 );
    if( pPuzzle->summaryLock == SEM_FAILED ) { fprintf( stderr, "Failed to create/open Semaphore to protect Summary file\n" ); return -1; }

    // Find out if the summary file already exists - if not, create it
    strcpy(szFile, SUMMARY);
    strcat(szFile, EXTENSION);
    if( stat( szFile, &fileInfo) != 0 )
    {
        // Note that once the semaphore is successfully opened, it must be closed
        if( sem_wait( pPuzzle->summaryLock ) != 0 ) { fprintf( stderr, "ERROR locking access to Summary file\n" ); return -1; }
        
        fp = fopen( szFile, "w");
        if( fp != NULL ) 
        {
            fprintf(fp, "Date/Time,Version,Puzzle,Algorithm,Params,Order,TTTS,MTTS,Average,Distribution,Std Dev,1st Guess,Elapsed Time,CPU Time,Considered,Processed,Evaluated,Evaluations/CPU sec, Solution File\n");
            if( fclose(fp) != 0 ) fprintf( stderr, "ERROR closing file in outputSetup\n" );
            fp = NULL;
        }
        else
        {
            fprintf( stderr, "ERROR opening file in outputSetup (%s)\n", szFile );
            rc = -1;
        }

        if( sem_post( pPuzzle->summaryLock ) != 0 ) { fprintf( stderr, "ERROR unlocking access to Summary file\n" ); return -1; }
    }

    // Also setup other output files if required
    pPuzzle->szSimOutputFile[0] = '\0';
    if( pPuzzle->simOutput > DEF_SIMOUTPUT ) setupSimoutput( pPuzzle );

    pPuzzle->szSymmetriesOutputFile[0] = '\0';
    if( pPuzzle->verbose == 2 ) setupSymmetriesOutput( pPuzzle );

    return rc;
}

// Organise for the analysis of a guess structure and add to the summary file
char* outputFileSummary( Puzzle* pPuzzle, char* distribution, int argc, char **argv  )
{
    char           szFile[MAX_DIR];
    char           pcBuf[pPuzzle->pegs+3];
    FILE*          fp  = NULL;  
    int            scores[pPuzzle->maxTurnsToSolve + 1];
    float          avg = 0.0;
    double         stdDev = 0.0;
    int            sum = 0;
    int            i   = 0;
    struct tm*     ts;
    char           tads[64];
    Guess*         pGuess = pPuzzle->pProblem->pGuessList;

    for( i = 0; i <= pPuzzle->maxTurnsToSolve; i++ ) scores[i] = 0;

    assessSummary( pPuzzle, pGuess, scores, 1 );
    avg = (float)pPuzzle->totalTurnsToSolve / (float)pPuzzle->solnSize;

    sprintf(distribution, "%d", scores[1]);
    stdDev = (1 - avg) * (1 - avg) * scores[1];
    for( i = 2; i <= pPuzzle->maxTurnsToSolve; i++ )
    {
        sprintf(distribution+strlen(distribution), "\\%d", scores[i]);
        stdDev += (i - avg) * (i - avg) * scores[i];
    }
    stdDev /= pPuzzle->solnSize;
    stdDev = sqrt( stdDev );

    ts = localtime( &pPuzzle->baseClockTime );
    strftime( tads, sizeof(tads), "%d/%m/%y %H:%M", ts );
    
    strcpy(szFile, SUMMARY);
    strcat(szFile,EXTENSION);

    // Protect access to the Summary file with a semaphore
    if( sem_wait( pPuzzle->summaryLock ) == 0 )
    {
        fp = fopen( szFile, "a");
        if( fp != NULL )
        {
            fprintf( fp, "%s", tads );                                                                   // Time and Date Stamp
            fprintf( fp, ",%d.%03d.%03d", MAYOR_VERSION, MINOR_VERSION, FIX_VERSION );                   // Version
            fprintf( fp, ",\"%s\"", pPuzzle->szGameName );                                               // Puzzle
            fprintf( fp, ",%s", pPuzzle->szAlgorithm );                                                  // Algorithm
            fprintf( fp, "," );
            for( i = 1; i < argc; i++ )                                                                  // Params
                fprintf( fp, " %s", argv[i] );
            if( pPuzzle->ordered ) fprintf( fp, ",Yes" ); else fprintf( fp, ",No" );                     // Order Matters flag
            fprintf( fp, ",%d", pPuzzle->totalTurnsToSolve );                                            // TTTS
            fprintf( fp, ",%d", pPuzzle->maxTurnsToSolve );                                              // MTTS
            fprintf( fp, ",%f", avg );                                                                   // Average
            fprintf( fp, ",%s", distribution );                                                          // Distribution
            fprintf( fp, ",%f", stdDev );                                                                // Standard Deviation
            fprintf( fp, ",%s", printCode(pPuzzle, pGuess->code, true, pcBuf) );                         // Initial Guess
            fprintf( fp, ",%0ld.%02ld", pPuzzle->elapsedTime / 100, pPuzzle->elapsedTime % 100);         // Elapsed Time
            fprintf( fp, ",%0ld.%02ld", pPuzzle->elapsedCpuTime / 100, pPuzzle->elapsedCpuTime % 100 );  // CPU Time
            fprintf( fp, ",%ld", pPuzzle->considered );                                                  // Guesses considered
            fprintf( fp, ",%ld", pPuzzle->processed );                                                   // Guesses processed
            fprintf( fp, ",%ld", pPuzzle->evaluated );                                                   // Guesses evaluated
            if( pPuzzle->elapsedCpuCalcTime > 0 )
                fprintf( fp, ",%ld", pPuzzle->evaluated*100/pPuzzle->elapsedCpuCalcTime );               // Guesses per CPU second
            else
                fprintf( fp, ",n/a" );                                                                   // Problem with zero elapsed time
            fprintf( fp, ",\"%s\"", pPuzzle->szSolnFile );                                               // Solution File
            fprintf( fp, "\n" );
            if( fclose(fp) != 0 ) fprintf( stderr, "ERROR closing file in outputSummary\n" );
            fp = NULL;
        }
        else
        {
            fprintf(stderr, "Can't open Summary file\n");
        }
        if( sem_post( pPuzzle->summaryLock ) != 0 )
        {
            fprintf( stderr, "ERROR unlocking access to Summary file\n" );
        }
    }
    else
    {
        fprintf( stderr, "ERROR locking access to Summary file\n" );
    }
    return distribution;
}

// Recursive function to extract information from the guess structure about how many turns it took to resolve each possible puzzle set
// Return the maximum number of turns taken to solve any puzzle
// (Note this is an alternative and repeated way of doing what maxTurnsToSolve does.  However, very useful for similarGuessesSolver)
int assessSummary( Puzzle* pPuzzle, Guess* pGuess, int* scores, int turns )
{
    int i           = 0;
    int maxTurns    = 0;
    int subMaxTurns = 0;

    for( i = 0; i < pPuzzle->marks; i++ )
    {
        if( pGuess->problem[i].size > 0 )
        {
            if( i != pPuzzle->allBlack )
            {
                switch( pGuess->problem[i].size )
                {
                    case 1:         // One more guess required
                        scores[turns+1]++;
                        if( maxTurns < turns+1 ) maxTurns = turns+1;
                        break;
                    case 2:         // One more guess + two more guesses
                        scores[turns+1]++;
                        scores[turns+2]++;
                        if( maxTurns < turns+2 ) maxTurns = turns+2;
                        break;
                    case 3:         // Depends on a couple of factors...
                        if( pGuess->problem[i].totalTurnsToSolve == 5 )
                        {
                            // This is the best case, one guess is a hit and splits the other two
                            scores[turns+1]++;
                            scores[turns+2]++;
                            scores[turns+2]++;
                            if( maxTurns < turns+2 ) maxTurns = turns+2;
                        }
                        else
                        {
                            if( pGuess->problem[i].pCommon->partitionList[pGuess->problem[i].lateStageGuess] != i )
                            {
                                // Second best case, there is a guess not in the code list that splits the remaining 3 solutions
                                scores[turns+2]++;
                                scores[turns+2]++;
                                scores[turns+2]++;
                                if( maxTurns < turns+2 ) maxTurns = turns+2;
                            }
                            else
                            {
                                // Worst case, we hit one, but don't split the other two 
                                scores[turns+1]++;
                                scores[turns+2]++;
                                scores[turns+3]++;
                                if( maxTurns < turns+3 ) maxTurns = turns+3;
                            }
                        }
                        break;
                    default:        // More than 3 solutions remain, we may have a split or we may need to continue to walk the tree
                        if( pGuess->problem[i].lateStageGuess != STOP )
                        {
                            // We have a split, so 1 case of one more guess and size-1 for 2 more guesses
                            scores[turns+1]++;
                            scores[turns+2] += pGuess->problem[i].size - 1;
                            if( maxTurns < turns+2 ) maxTurns = turns+2;
                        }
                        else
                            subMaxTurns = assessSummary( pPuzzle, pGuess->problem[i].pGuessList, scores, turns + 1 );
                            if( maxTurns < subMaxTurns ) maxTurns = subMaxTurns;
                        break;

                }
            }
            else
            {
                // This is a solution, so update the scores array
                scores[turns]++;
                if( maxTurns < turns ) maxTurns = turns;
            }
        }
    }
    return maxTurns;
}

void outputScreenSummary( Puzzle* pPuzzle, char* distribution )
{
    char pcBuf[pPuzzle->pegs+3];

    // Output info directly to stdout
    fprintf( stdout, "\n%s evaluated successfully\n", pPuzzle->szGameName );
    fprintf( stdout, "Total Turns to Solve:     %d\n", pPuzzle->totalTurnsToSolve );
    fprintf( stdout, "Maximum Turns to Solve:   %d\n", pPuzzle->maxTurnsToSolve );
    fprintf( stdout, "Elapsed Time:             %0ld.%02ld\n", pPuzzle->elapsedTime / 100, pPuzzle->elapsedTime % 100 );
    fprintf( stdout, "CPU Time:                 %0ld.%02ld\n", pPuzzle->elapsedCpuTime / 100, pPuzzle->elapsedCpuTime % 100 );
    fprintf( stdout, "Guesses considered:       %ld\n", pPuzzle->considered );
    fprintf( stdout, "Guesses processed:        %ld\n", pPuzzle->processed );
    fprintf( stdout, "Guesses fully evaluated:  %ld\n", pPuzzle->evaluated );
    fprintf( stdout, "Algorithm used:           %s", pPuzzle->szAlgorithm );
    if( pPuzzle->initSitGuess != STOP || pPuzzle->firstGuess != STOP )
    {
        fprintf( stdout, " " );
        if( pPuzzle->initSitGuess != STOP )
            fprintf( stdout, "-i%s:%s"
                , printCode( pPuzzle, pPuzzle->initSitGuess, true, pcBuf )
                , pPuzzle->markString[pPuzzle->initSitMark]
                );
        if( pPuzzle->initSitGuess != STOP && pPuzzle->firstGuess != STOP )
            fprintf( stdout, " " );
        if( pPuzzle->firstGuess != STOP )
            fprintf( stdout, "-f%s"
                , printCode( pPuzzle, pPuzzle->firstGuess, true, pcBuf )
                );
    }
    fprintf( stdout, "\n" );
    fprintf( stdout, "Distribution:             %s\n", distribution );
    fprintf( stdout, "Initial guess:            %s\n", printCode( pPuzzle, pPuzzle->pProblem->pGuessList->code, true, pcBuf ) );
    if( pPuzzle->verbose == 1 )
        fprintf( stdout, "Repeating Structures      %d\n", pPuzzle->countGuess );
    fprintf( stdout, "\n" );
    if( pPuzzle->simOutput > 0 )
    {
        fprintf( stdout, "Similar Guesses Analysis\n" );
        fprintf( stdout, "Level:                    %d\n", pPuzzle->simOutput );
        fprintf( stdout, "Distinct Solutions:       %d\n", pPuzzle->pSimilarSolnIssues->distinctResults );
        fprintf( stdout, "Total Sets:               %d\n", pPuzzle->pSimilarSolnIssues->totalSets );
        fprintf( stdout, "Non-Symetrical Guesses:   %d\n", pPuzzle->pSimilarSolnIssues->totalNonSym );
        if( pPuzzle->pSimilarSolnIssues->pSetIssue == NULL )
        {
            fprintf( stdout, "No Issues found\n" );
        }
        else
        {
            int count = 0;
            while( pPuzzle->pSimilarSolnIssues->pSetIssue != NULL )
            {
                count += 1;
                fprintf( stdout, "Issue:  " );
                fprintf( stdout, "%s:", printCode( pPuzzle, pPuzzle->pSimilarSolnIssues->pSetIssue->prevGuess, true, pcBuf ) );
                fprintf( stdout, "%-6s ", pPuzzle->markString[pPuzzle->pSimilarSolnIssues->pSetIssue->prevMark] );
                fprintf( stdout, "%8d:", pPuzzle->pSimilarSolnIssues->pSetIssue->totalTurnsToSolve );
                fprintf( stdout, "%-20s", pPuzzle->pSimilarSolnIssues->pSetIssue->distribution );
                fprintf( stdout, "  (Best %d)", pPuzzle->pSimilarSolnIssues->pSetIssue->bestTTTS );
                if( pPuzzle->pSimilarSolnIssues->pSetIssue->totalTurnsToSolve <= pPuzzle->pSimilarSolnIssues->pSetIssue->bestTTTS )
                    fprintf( stdout, " ***" );
                fprintf( stdout, "\n" );

                pPuzzle->pSimilarSolnIssues->pSetIssue = pPuzzle->pSimilarSolnIssues->pSetIssue->pNextSetIssue;
            }
            fprintf( stdout, "Total number of issues:   %d\n", count );
        }
        fprintf( stdout, "\n" );
    }
    return;
}
