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
// Only set headerOK true if it looks like a header
// If it does look like a header, set up the max number of guesses as well
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
            if( strcmp( field, "#" ) != 0 ) return 0;
        }
        if( fields > 1 )
        {
            fieldLen = nextField( line + offset, field, 256 );
            offset += fieldLen + 1;
            if( strcmp( field, "Solution" ) != 0 ) return 0;
        }
        if( fields > 2 )
        {
            fieldLen = nextField( line + offset, field, 256 );
            offset += fieldLen + 1;
            if( strcmp( field, "Turns" ) != 0 ) return 0;
        }
        if( fields == ( fields / 2 ) * 2 ) return 0;  // Not expecting an even number of fields
        if( fields > 3 + 10 * 2 )          return 0;  // Not expection more than 10 guesses

        pRepo->guesses = ( fields - 3 ) / 2;
        pRepo->headerOK = true;
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

    if( pRepo->headerOK ) getLine( pRepo->fp, line, 256 );
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
    if( pRepo->headerOK ) pRepo->actualCodes -= 1;

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
    int  i        = 0;
    int  j        = 0;
    int  rc       = 0;

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
        pRepo->missing[i].code        = -1;
        pRepo->missing[i].codeMissing = false;
    }

    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        pRepo->data[i].code            = -1;
        pRepo->data[i].codeOK          = false;
        pRepo->data[i].codeRepeated    = false;
        pRepo->data[i].noTurns         = -1;
        pRepo->data[i].turnsOK         = false;
        pRepo->data[i].actualNoTurns   = 99999;
        pRepo->data[i].resolved        = false;
        pRepo->data[i].marksOK         = true;
        pRepo->data[i].guessesOK       = false;     // Guess data is in the form Guess / Mark as expected
        pRepo->data[i].guessConsistant = true;      // Same guess as all similar marks? We will only ever turn this false, so start positive

        pRepo->data[i].turns = malloc( sizeof(Turn) * pRepo->guesses );
        if( pRepo->data[i].turns != NULL )
        {
            for( j = 0; j < pRepo->guesses; j++ )
            {
                pRepo->data[i].turns[j].guess        = -1;
                pRepo->data[i].turns[j].guessOK      = false;
                pRepo->data[i].turns[j].guessIllegal = true;
                pRepo->data[i].turns[j].mark         = -1;
                pRepo->data[i].turns[j].markOK       = false;
                pRepo->data[i].turns[j].used         = false;
            }
        }
        else
        {
            fprintf( stderr, "Failed to create one of the Turn arrays\n" );
            return -1;
        }
    }

    rc = fseek( pRepo->fp, 0, SEEK_SET );      // Go to the beginning of the file

    // Throw away header line if there is one
    if( pRepo->headerOK ) getLine( pRepo->fp, line, 256 );

    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        fields = getLine( pRepo->fp, line, 256 );
        if( fields != EOF )
        {
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
                pRepo->data[i].turnsOK = false;                             // 'til proved otherwise
            }

            guesses = (fields - 3) / 2;
            pRepo->data[i].guessesOK = ( guesses * 2 + 3 == fields );       // Must have pairs of fields (Guess + Mark)

            if( guesses > pRepo->guesses )
            {
                fprintf( stderr, "More guesses than expected\n" );
                return -1;
            }

            for( j = 0; j < guesses; j++ )
            {
                pRepo->data[i].turns[j].used = true;

                fieldLen = nextField( line + offset, field, 256 );
                offset += fieldLen + 1;
                pRepo->data[i].turns[j].guess = parseCode( pRepo, field );
                pRepo->data[i].turns[j].guessOK = ( pRepo->data[i].turns[j].guess == -1 );

                fieldLen = nextField( line + offset, field, 256 );
                offset += fieldLen + 1;
                pRepo->data[i].turns[j].mark = getMark( pRepo, field );
                pRepo->data[i].turns[j].markOK = ( pRepo->data[i].turns[j].mark == -1 );
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
    int thisCode = 0;
    int nextCode = 0;
    int i        = 0;

    // Sort into code order
    qsort( pRepo->data, pRepo->actualCodes, sizeof(Solution), cmpCodeOrder );
    for( i = 0; i < pRepo->actualCodes; i++ )
    {
        if( pRepo->data[i].code == nextCode )
        {
            thisCode = pRepo->data[i].code;
            pRepo->data[i].codeRepeated = false;
            nextCode = thisCode + 1;
        }
        else if( pRepo->data[i].code == thisCode )
        {
            pRepo->data[i].codeRepeated = true;
            nextCode = thisCode + 1;
        }
        else if( pRepo->data[i].code > nextCode )
        {
            for( i = nextCode; i < pRepo->data[i].code; i++ )
            {
                pRepo->missing[i].code        = i;
                pRepo->missing[i].codeMissing = true;
            }
        }
    }
    // Did we stop solving codes early?
    for( i = pRepo->actualCodes + 1; i < pRepo->codes; i++ )
    {
        pRepo->missing[i].code        = i;
        pRepo->missing[i].codeMissing = true;
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
        }
    }
    return 0;
}

// Check that only one guess is made per group of codes
int checkGuesses( Repo* pRepo )
{
    int  code      = 0;
    int  level     = 0;
    int  prevMark  = 0;
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

    // Now check lower levels
    for( level = 0; level < pRepo->guesses; level++ )
    {
        prevMark = pRepo->data[0].turns[level].mark;
        prevGuess = pRepo->data[0].turns[level+1].guess;
        for( i = 1; i < pRepo->actualCodes; i++ )
        {
            if( prevMark != allBlack && prevMark != -1 && pRepo->data[i].turns[level].mark == prevMark )
            {
                if( pRepo->data[i].turns[level+1].guess != prevGuess ) pRepo->data[i].guessConsistant = false;
            }
            else
            {
                prevMark = pRepo->data[i].turns[level].mark;
                prevGuess = pRepo->data[i].turns[level+1].guess;
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
            else
                pRepo->data[i].marksOK = false;
        }
    }
    return 0;
}

int report( Repo* pRepo )
{
    fprintf( stdout, "Placeholder\n" );
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
    for( i = pRepo->pegs - 1; i >= 0; i-- )
    {
        code *= pRepo->pegs;
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
    ch = fgetc( fp );
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
        ch = fgetc( fp );
    }

    return i;
}

// Get the next field from the string passed
// Field is delimited by a comma or nul delimiter
// The returned field does not contain the comma delimiter
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
    ch = fgetc( fp );
    while( ch == '\n' ) ch = fgetc( fp );       // Step over any empty lines
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
        ch = fgetc( fp );
    }

    return fields;
}