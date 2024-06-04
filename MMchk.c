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
// Main function to setup,launch and report on the resolution of a Mastermind puzzle
//
#include "MMchk.h"
#include "MMparams.h"
#include "MMsortfns.h"
#include "MMutility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>


// Program entry point and high level orchestration of activities
int main( int argc, char **argv )
{
    Repo         repo;
    int          rc = 0;

    // Use the parameters passed (or defaults) to define the puzzle that is to be solved
    rc = setup( &repo, argc, argv ); if( rc ) return rc;    // Setup repository and access file for analysis
    rc = parseHeader( &repo );       if( rc ) return rc;    // Check header and find max number of guesses
    rc = countPegs( &repo );         if( rc ) return rc;    // Return the number of pegs in each code
    rc = countCodes( &repo );        if( rc ) return rc;    // Return the number of codes listed in the solution file
    rc = parseFile( &repo );         if( rc ) return rc;    // Read the whole file into data structures
    rc = setupCodeDefs( &repo );     if( rc ) return rc;    // Can only set up code defs after we know the number of codes, pegs and colours
    rc = setupMarks( &repo );        if( rc ) return rc;    // Can only set up the marks after we know the number of codes, pegs and colours

    rc = checkCodes( &repo );        if( rc ) return rc;    // Check all codes are there, and none repeated
    rc = checkCounts( &repo );       if( rc ) return rc;    // Check all solutions end in all-black and that the counts of turns to solve is correct
    rc = checkGuesses( &repo );      if( rc ) return rc;    // Check that only one guess is made per group of codes
    rc = checkMarks( &repo );        if( rc ) return rc;    // Check that all the marking is correct

    rc = report( &repo );            if( rc ) return rc;    // Output findings to stdout

    return 0;    
}

// Check the first line of the file
// If it does not look like it should, assume the file is corrupt and bail out
int parseHeader( Repo* pRepo )
{
    char line[256];
    char field[256];
    int  fields   = 0;
    int  fieldLen = 0;
    int  offset   = 0;
    int  i        = 0;
    int  guesses  = 0;
    int  rc       = 0;

    rc = fseek( pRepo->fp, 0, SEEK_SET );      // Go to the beginning of the file
    fields = getLine( pRepo->fp, line, 256 );
    if( line[0] != '\0' )
    {
        offset = 0;
        if( fields > 0 )
        {
            fieldLen = nextField( line + offset, field, 256 );
            offset += fieldLen + 1;
            if( strcmp( field, "#" ) != 0 )
            {
                fprintf( stderr, "Header line is incorrectly formatted - assuming file is corrupt\n" );
                return -1;
            }
        }
        if( fields > 1 )
        {
            fieldLen = nextField( line + offset, field, 256 );
            offset += fieldLen + 1;
            if( strcmp( field, "Solution" ) != 0 )
            {
                fprintf( stderr, "Header line is incorrectly formatted - assuming file is corrupt\n" );
                return -1;
            }
        }
        if( fields > 2 )
        {
            fieldLen = nextField( line + offset, field, 256 );
            offset += fieldLen + 1;
            if( strcmp( field, "Turns" ) != 0 )
            {
                fprintf( stderr, "Header line is incorrectly formatted - assuming file is corrupt\n" );
                return -1;
            }
        }
        if( fields == ( fields / 2 ) * 2 )      // Not expecting an even number of fields
        {
            fprintf( stderr, "Header line shows mismatched guesses and marks - assuming file is corrupt\n" );
            return -1;
        }

        if( fields > 3 + 10 * 2 )               // Not expection more than 10 guesses
        {
            fprintf( stderr, "Header line shows more guesses than expected - assuming file is corrupt\n" );
            return -1;
        }

        pRepo->guesses = ( fields - 3 ) / 2;
    }
    return 0;
}

// Count how many pegs there are in the first code
// It is assumed that every code has the same number of pegs, but this will be checked later
int countPegs( Repo* pRepo )
{
    char line[256];
    char field[256];
    int  len = 0;
    int  rc  = 0;

    rc = fseek( pRepo->fp, 0, SEEK_SET );      // Go to the beginning of the file

    getLine( pRepo->fp, line, 256 );
    len = getField( pRepo->fp, field, 256 );
    len = getField( pRepo->fp, field, 256 );
    if( len != EOF )
    {
        // Default to the number taken from the codes found in the solution file
        if( pRepo->pegs > 0 )
            pRepo->pegsOK = ( pRepo->pegs == len );
        pRepo->pegs = len;
    }

    return 0;
}

// Count how many codes were resolved
// This is taken to be the number of non-empty lines, less the header
// (First line will be checked to be the header)
// We will also try to work out the number of colours in this puzzle
int countCodes( Repo* pRepo )
{
    char   line[256];
    int    colours = 0;
    int    codes   = 0;
    int    rc      = 0;

    // Count the actual number of codes reported in the solution output
    rc = fseek( pRepo->fp, 0, SEEK_SET );      // Go to the beginning of the file
    if( rc != 0 )
    {
        fprintf( stderr, "Error running fseek in countCodes\n" );
        return -1;
    }
    while( getLine( pRepo->fp, line, 256 ) != EOF )  pRepo->actualCodes += 1;
    pRepo->actualCodes -= 1;    // Account for header line

    // Calculate the apparent number of colours and compare with what we may have read from the filename
    // If we did not get the number of colours from the filename, then have to go with calculated number
    colours = round( pow( pRepo->actualCodes, 1.0 / pRepo->pegs ) );
    if( pRepo->colours > 0 )
        pRepo->coloursOK = ( pRepo->colours == colours );
    else
        pRepo->colours = colours;

    // Now calculate the expected number of codes from the numbers of colours and pegs and check against actual
    pRepo->codes = round( pow( pRepo->colours, pRepo->pegs ) );
    pRepo->codesOK = ( pRepo->codes == pRepo->actualCodes );

    return 0;
}

// Read the output file into data structures
// We should know the max guesses from the header, but if not assume 8
// Note that the header file will not be stored
int parseFile( Repo* pRepo )
{
    char line[256];
    char field[256];
    int  fields   = 0;
    int  offset   = 0;
    int  code     = 0;
    int  guesses  = 0;
    int  fieldLen = 0;
    bool done     = false;
    int  allBlack = -1;
    int  i        = 0;
    int  j        = 0;
    int  rc       = 0;

    allBlack = ( pRepo->pegs * ( pRepo->pegs + 3 ) ) / 2 - 1;

    pRepo->data = malloc( sizeof(Solution) * pRepo->actualCodes );
    if( pRepo->data == NULL )
    {
        fprintf( stderr, "Failed to create Solution array\n" );
        return -1;
    }
    pRepo->missing = malloc( sizeof(Absent) * pRepo->codes );
    if( pRepo->missing == NULL )
    {
        fprintf( stderr, "Failed to create Absent array\n" );
        return -1;
    }

    for( i = 0; i < pRepo->codes; i++ )
    {
        pRepo->missing[i].code        = i;
        pRepo->missing[i].codeMissing = true;
    }

    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        // Parameters
        pRepo->data[i].line            = -1;
        pRepo->data[i].code            = -1;
        pRepo->data[i].noTurns         = -1;
        pRepo->data[i].actualNoTurns   = 99999;
        // Correctness flags
        pRepo->data[i].codeOK          = false;     // Set either way, so need to prove its good
        pRepo->data[i].codeRepeated    = true;      // Set either way, so need to prove its good
        pRepo->data[i].turnsOK         = false;     // Set either way, so need to prove its good
        pRepo->data[i].resolved        = false;     // Set if ok, so need to prove its good
        pRepo->data[i].marksOK         = true;      // Only change if there's a problem, so start optimistically
        pRepo->data[i].guessesOK       = false;     // Set either way, so need to prove its good
        pRepo->data[i].guessConsistant = true;      // Only change if there's a problem, so start optimistically

        pRepo->data[i].turns = malloc( sizeof(Turn) * pRepo->guesses );
        if( pRepo->data[i].turns != NULL )
        {
            for( j = 0; j < pRepo->guesses; j++ )
            {
                // Parameters
                pRepo->data[i].turns[j].guess        = -1;
                pRepo->data[i].turns[j].mark         = -1;
                // Correctness flags
                pRepo->data[i].turns[j].guessOK      = false;     // Set either way, so need to prove its good
                pRepo->data[i].turns[j].markOK       = false;     // Set if ok, so need to prove its good
            }
        }
        else
        {
            fprintf( stderr, "Failed to create one of the Turn arrays\n" );
            return -1;
        }
    }

    rc = fseek( pRepo->fp, 0, SEEK_SET );      // Go to the beginning of the file

    // Throw away header line
    getLine( pRepo->fp, line, 256 );

    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        fields = getLine( pRepo->fp, line, 256 );
        if( fields != EOF )
        {
            pRepo->data[i].line = i;
            offset = 0;
            if( fields >= 3 )
            {
                fieldLen = nextField( line + offset, field, 256 );
                offset += fieldLen + 1;
                pRepo->data[i].code = stringToInt( field );

                fieldLen = nextField( line + offset, field, 256 );
                offset += fieldLen + 1;
                code = parseCode( pRepo, field );
                pRepo->data[i].codeOK = ( pRepo->data[i].code == code );

                fieldLen = nextField( line + offset, field, 256 );
                offset += fieldLen + 1;
                pRepo->data[i].noTurns = stringToInt( field );
            }

            guesses = (fields - 3) / 2;
            pRepo->data[i].guessesOK = ( guesses * 2 + 3 == fields );       // Must have pairs of fields (Guess + Mark)

            if( guesses > pRepo->guesses )
            {
                fprintf( stderr, "More guesses than expected\n" );
                return -1;
            }

            done = false;
            for( j = 0; j < guesses && ! done; j++ )
            {
                fieldLen = nextField( line + offset, field, 256 );
                if( fieldLen > 0 )
                {
                    offset += fieldLen + 1;
                    pRepo->data[i].turns[j].guess = parseCode( pRepo, field );
                    pRepo->data[i].turns[j].guessOK = ( pRepo->data[i].turns[j].guess != -1 );

                    fieldLen = nextField( line + offset, field, 256 );
                    if( fieldLen > 0 )
                    {
                        offset += fieldLen + 1;
                        pRepo->data[i].turns[j].mark = getMark( pRepo, field );
                        if( pRepo->data[i].turns[j].mark == allBlack || pRepo->data[i].turns[j].mark == -1 ) done = true;
                    }
                    else
                    {
                        done = true;
                    }
                }
                else
                {
                    done = true;
                }

            }
        }
        else
        {
            fprintf( stderr, "Problem with inconsistent code counts in parseFile\n" );
            return -1;
        }
    }

    return 0;
}

// Check all codes are there, and none repeated
int checkCodes( Repo* pRepo )
{
    int priorCode = 0;
    int expectedCode = 0;
    int i        = 0;
    int j        = 0;

    // Sort into code order
    qsort( pRepo->data, pRepo->actualCodes, sizeof(Solution), cmpCodeOrder );
    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        if( pRepo->data[i].code == expectedCode )
        {
            pRepo->data[i].codeRepeated = false;
        }
        else if( pRepo->data[i].code == priorCode )
        {
            pRepo->data[i].codeRepeated = true;
        }
        else if( pRepo->data[i].code > expectedCode )
        {
            for( j = expectedCode; j < pRepo->data[i].code; j++ )
            {
                pRepo->missing[j].codeMissing = true;
                pRepo->data[j].codeRepeated = false;
            }
            pRepo->data[i].codeRepeated = false;
        }
        expectedCode = pRepo->data[i].code + 1;
        priorCode = pRepo->data[i].code;

        // Mark off each code we've seen
        if( priorCode < pRepo->codes )
            pRepo->missing[priorCode].codeMissing = false;

    }
    return 0;
}

// Check all solutions end in all-black and that the counts of turns to solve is correct
int checkCounts( Repo* pRepo )
{
    int  allBlack = 0;
    bool done     = false;
    int  i        = 0;
    int  g        = 0;

    // Calculate what mark represents sucess
    allBlack = ( pRepo->pegs * ( pRepo->pegs + 3 ) ) / 2 - 1;

    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        done = false;
        for( g = 0; g < pRepo->guesses && ! done; g++ )
        {
            if( pRepo->data[i].turns[g].mark == allBlack )
            {
                pRepo->data[i].turnsOK = ( pRepo->data[i].noTurns == g + 1 );
                pRepo->data[i].actualNoTurns = g + 1;
                pRepo->data[i].resolved = true;
                done = true;
            }
            else if( pRepo->data[i].turns[g].mark == -1 )
            {
                pRepo->data[i].turnsOK = ( pRepo->data[i].noTurns == g );
                pRepo->data[i].actualNoTurns = g;
                pRepo->data[i].resolved = false;
                done = true;
            }
        }
    }
    return 0;
}

// Check that only one guess is made per group of codes
int checkGuesses( Repo* pRepo )
{
    int  code      = 0;
    int  level     = 0;
    int  lastMark  = 0;
    int  lastGuess = 0;
    int  prevGuess = 0;
    int  allBlack  = 0;
    int  i         = 0;

    // Sort into mark order
    qsort( pRepo->data, pRepo->actualCodes, sizeof(Solution), cmpMarkOrder );

    // Calculate what mark represents sucess
    allBlack = ( pRepo->pegs * ( pRepo->pegs + 3 ) ) / 2 - 1;

    // Check that the same guess was made for every code at first level
    prevGuess = pRepo->data[0].turns[0].guess;
    for( i = 1; i < pRepo->actualCodes; i++ )
        if( pRepo->data[i].turns[0].guess != prevGuess ) pRepo->data[i].guessConsistant = false;

    // Now check eery guess at other levels
    for( i = 1; i < pRepo->actualCodes; i++ )
    {
        // Look at each level and if the previous guesses and marks were the same - this guess must be the same
        for( level = 1; level < pRepo->data[i].actualNoTurns; level++ )
        {
            lastMark  = pRepo->data[i-1].turns[level-1].mark;
            lastGuess = pRepo->data[i-1].turns[level-1].guess;
            if( pRepo->data[i].turns[level-1].mark == lastMark && pRepo->data[i].turns[level-1].guess == lastGuess )
            {
                prevGuess = pRepo->data[i-1].turns[level].guess;
                if( pRepo->data[i].turns[level].guess != prevGuess )
                    pRepo->data[i].guessConsistant = false;
            }

        }
    }
    return 0;
}

// Check that all the marking is correct
int checkMarks( Repo* pRepo )
{
    int  mark = 0;
    int  i    = 0;
    int  g    = 0;

    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        for( g = 0; g < pRepo->data[i].actualNoTurns; g++ )
        {
            mark = marking( pRepo, pRepo->data[i].code, pRepo->data[i].turns[g].guess );
            if( pRepo->data[i].turns[g].mark == mark )
                pRepo->data[i].turns[g].markOK = true;
        }
    }
    return 0;
}

int report( Repo* pRepo )
{
    bool  fileError     = false;
    bool  solutionError = false;
    bool* solnErrIndex  = NULL;
    bool  guessError    = false;
    char  buffer[15];
    char  line[256];
    FILE* fpo           = NULL;
    int   len           = 0;
    int   TTTS          = 0;
    int   i             = 0;
    int   j             = 0;

    solnErrIndex = (bool*)malloc( sizeof(bool) * pRepo->actualCodes );
    if( solnErrIndex == NULL )
    {
        fprintf( stderr, "Failed to allocate array in report function\n" );
        return -1;
    }

    // Put solutions back into original order
    qsort( pRepo->data, pRepo->actualCodes, sizeof(Solution), cmpLineOrder );
    qsort( pRepo->missing, pRepo->codes, sizeof(Absent), cmpAbsentOrder );

    // Write header for stdout status
    fprintf( stdout, "\nAnalysis of %s:   ", pRepo->baseName );

    // Work out if there are any top level problems
    if( ! pRepo->pegsOK || ! pRepo->coloursOK || ! pRepo->codesOK || pRepo->missing[0].codeMissing )
        fileError = true;

    // Now work out if there are any solution level problems
    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        solnErrIndex[i] = false;

        if(    ! pRepo->data[i].codeOK   ||   pRepo->data[i].codeRepeated || ! pRepo->data[i].turnsOK
            || ! pRepo->data[i].resolved || ! pRepo->data[i].marksOK      || ! pRepo->data[i].guessesOK || ! pRepo->data[i].guessConsistant
          )
        {
            solnErrIndex[i] = true;
            solutionError = true;
        }
        else
        {
            for( j = 0; j < pRepo->data[i].actualNoTurns; j++ )
                if( ! pRepo->data[i].turns[j].guessOK || ! pRepo->data[i].turns[j].markOK )
                {
                    solnErrIndex[i] = true;
                    solutionError = true;
                }
        }
    }

    // Hopefully no problems...
    if( ! fileError && ! solutionError )
    {
        for( i = 0; i < pRepo->actualCodes; i++ )
            TTTS += pRepo->data[i].noTurns;
        fprintf( stdout, "No errors found.  TTTS = %d\n\n", TTTS );
        return 0;
    }

    // If there are high level problems - write the details to stdout
    if( fileError )
    {
        fprintf( stdout, "\n" );
        if( ! pRepo->pegsOK || ! pRepo->coloursOK )
            fprintf( stdout, "Inconsistent numbers of Pegs/Colours between filename and solution (Ignoring filename)\n" );

        if( ! pRepo->codesOK )
        {
            fprintf( stdout, "Unexpected number of codes shown in solution\n" );
            fprintf( stdout, "Expecting %d codes, actually output %d codes\n", pRepo->codes, pRepo->actualCodes );
        }

        if( pRepo->missing[0].codeMissing )
        {
            fprintf( stdout, "The following code(s) were not shown in the solution file\n" );
            fprintf( stdout, "  %s", printCode( pRepo, pRepo->missing[0].code, true, buffer ) );
            for( i = 1; pRepo->missing[i].codeMissing; i++ )
                fprintf( stdout, ",%s", printCode( pRepo, pRepo->missing[i].code, true, buffer ) );
            fprintf( stdout, "\n" );
        }
    }

    if( solutionError )
    {
        // Change directory and also set up output filename
        if( chdir( pRepo->dirName ) != 0 )
        {
            fprintf( stderr, "Can't change to directory %s\n", pRepo->dirName );
            return -1;
        }

        strcpy( pRepo->outputName, pRepo->filename );
        len = strlen( pRepo->outputName );
        if( pRepo->outputName[len-4] == '.' && pRepo->outputName[len-3] == 'c' && pRepo->outputName[len-2] == 's' && pRepo->outputName[len-1] == 'v' )
        {
            strcpy( pRepo->outputName + len - 4, "_ERRORS.csv" );
        }
        else
        {
            fprintf( stderr, "Filename does not have the expected extension (.csv) - output to ERRORS.csv (may overwrite)\n" );
            strcpy( pRepo->outputName, "ERRORS.csv" );
        }
        fpo = fopen( pRepo->outputName, "w" );
        if( fpo == NULL )
        {
            fprintf( stderr, "Unable to open file: %s\n", pRepo->outputName );
            fprintf( stderr, "Solution errors - but unable to output details\n" );
            return -1;
        }

        // Tell stdout that there's an error file - and what it's called
        fprintf( stdout, "solution level errors - details in %s\n", pRepo->outputName );

        // Now merge the input file with errors found
        fseek( pRepo->fp, 0, SEEK_SET );      // Go to the beginning of the solution file

        getLine( pRepo->fp, line, 256 );
        fprintf( fpo, "Status,Issues,%s\n", line );  // Write header

        for( i = 0; i < pRepo->actualCodes; i++ )
        {
            getLine( pRepo->fp, line, 256 );
            if( ! solnErrIndex[i] )
            {
                fprintf( fpo, "OK,,%s\n", line );  // Say it's ok - then add original line
            }
            else
            {
                fprintf( fpo, "ERR," );            // Say there's an error, then add details
                if( ! pRepo->data[i].codeOK )           fprintf( fpo, "Code and Rep don't match " );
                if( pRepo->data[i].codeRepeated )       fprintf( fpo, "Repeated " );
                if( ! pRepo->data[i].turnsOK )          fprintf( fpo, "Turns incorrect " );
                if( ! pRepo->data[i].resolved )         fprintf( fpo, "Not resolved " );
                if( ! pRepo->data[i].marksOK )          fprintf( fpo, "Mark(s) wrong " );
                if( ! pRepo->data[i].guessesOK )        fprintf( fpo, "Guess/mark issue " );
                if( ! pRepo->data[i].guessConsistant )  fprintf( fpo, "Inconsistent guesses " );

                fprintf( fpo, ",%s\n", line );  // Finish with original line

                guessError = false;
                for( j = 0; j < pRepo->data[i].actualNoTurns; j++ )
                    if( ! pRepo->data[i].turns[j].guessOK || ! pRepo->data[i].turns[j].markOK )
                        guessError = true;
                if( guessError )
                {
                    fprintf( fpo, ",,,,," );  // Step over initial fields
                    for( j = 0; j < pRepo->data[i].actualNoTurns; j++ )
                    {
                        if( ! pRepo->data[i].turns[j].guessOK ) fprintf( fpo, "Prob," ); else fprintf( fpo, "," ); 
                        if( ! pRepo->data[i].turns[j].markOK )  fprintf( fpo, "Prob," ); else fprintf( fpo, "," );
                    }
                    fprintf( fpo, "\n" );
                }
            }
        }
        fclose( fpo );
    }
    fprintf( stdout, "\n" );
    free( solnErrIndex );
    solnErrIndex = NULL;

    return 0;
}

// Reverse calculate the numeric code from a text based representation
// Note that the codes are displayed with the post significant value to the right
// The code may also have backets around it (to show that it is not in the code list)
// These brackets must be discarded and ignored
int parseCode( Repo* pRepo, char* szCode )
{
    int offset = 0;
    int code   = 0;
    int i      = 0;

    if( szCode[0] == '(' )
        offset = 1;
    if( strlen( szCode ) != pRepo->pegs + offset * 2 )
    {
        fprintf( stderr, "Wrong number of pegs in code\n" );
        return -1;
    }
    for( i = 0; i < pRepo->pegs; i++ )
    {
        code *= pRepo->colours;
        code += szCode[i+offset] - 'A';
    }
    return code;
}

// Get the next field from the open file
// Field is delimited by a comma, new line or EOF
// The returned field does not contain the field delimiter
// The file pointer is left pointing to just after the delimiter (unless we are at EOF)
// If the file pointer is at the EOF, then EOF is returned
// Otherwise the field length is returned
// If the maxLen is too short, an empty field string is returned
// (But the file pointer is still moved on and the length of the field is still returned)
int getField( FILE* fp, char* field, int maxLen )
{
    int ch     = 0;
    int i      = 0;

    field[0] = '\0';
    ch = sgetc( fp );
    if( ch == EOF ) return EOF;

    while( ch != ',' && ch != '\n' && ch != EOF )
    {
        if( i < maxLen - 1 )
        {
            field[i]   = ch;
            field[i+1] = '\0';
        }
        else
        {
            field[0] = '0';
        }
        i += 1;
        ch = sgetc( fp );
    }

    return i;
}

// Get the next field from the string passed
// Field is delimited by a comma or null delimiter
// The returned field does not contain the delimiter
// If the passed string is empty, the returned string will also be empty
// The field length is returned
// If the maxLen is too short, an empty field string is returned
// (But the length of the field is still returned)
int nextField( char* string, char* field, int maxLen )
{
    int ch     = 0;
    int i      = 0;

    field[0] = '\0';

    while( string[i] != ',' && string[i] != '\n' && string[i] != '\0' )
    {
        if( i < maxLen - 1 )
        {
            field[i]   = string[i];
            field[i+1] = '\0';
        }
        else
        {
            field[0] = '0';
        }
        i += 1;
    }

    return i;
}

// Get the next line from the open file
// Line is delimited by a new line or EOF
// The returned field does not contain the line delimiter
// The file pointer is left pointing to just after the delimiter (unless we are at EOF)
// Blank lines are ignored
// If the file pointer is at the EOF, then EOF is returned
// Otherwise the number of fields in the line is returned
// If the maxLen is too short, an empty line string is returned
// (But the file pointer is still moved on and the number of fields is still returned)
int getLine( FILE* fp, char* line, int maxLen )
{
    int ch     = 0;
    int fields = 0;
    int i      = 0;

    line[0] = '\0';
    ch = sgetc( fp );
    while( ch == '\n' ) ch = sgetc( fp );       // Step over any empty lines
    if( ch == EOF ) return EOF;

    fields = 1;
    while( ch != '\n' && ch != EOF )
    {
        if( i < maxLen - 1 )
        {
            line[i]   = ch;
            line[i+1] = '\0';
        }
        else
        {
            line[0] = '\0';
        }
        i += 1;
        if( ch == ',' ) fields += 1;
        ch = sgetc( fp );
    }

    return fields;
}

// Special version of fgetc to convert \r\n combinations into \n
// (Windows and Excel introduce the \r\n combination)
int sgetc( FILE* fp )
{
    int ch = fgetc( fp );
    if( ch == '\r' ) ch = fgetc( fp );
    return ch;
}