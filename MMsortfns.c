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
// Comparison functions used by various qsort requests
//
#include "MMsortfns.h"
#include "MMchk.h"

#include <string.h>

// Used by qsort to order into ascending order by code
int cmpCodeOrder(const void* a, const void* b)
{
   return ((Solution*)a)->code - ((Solution*)b)->code;
}

// Used by qsort to order into the original order
int cmpLineOrder(const void* a, const void* b)
{
   return ((Solution*)a)->line - ((Solution*)b)->line;
}

// Used by qsort to order into ascending order by marks at each level
int cmpMarkOrder(const void* a, const void* b)
{
   return cmpMarkLevel( ((Solution*)a)->turns, ((Solution*)b)->turns );
}

// Used by qsort to order into ascending order by marks at each level
int cmpMarkLevel( Turn* a, Turn* b )
{
   if( a->mark > b->mark ) return  1;
   if( a->mark < b->mark ) return -1;
   return cmpMarkLevel( ++a, ++b );
}

int cmpAbsentOrder(const void* a, const void* b)
{
   if(   ((Absent*)a)->codeMissing && ! ((Absent*)b)->codeMissing ) return -1;
   if( ! ((Absent*)a)->codeMissing &&   ((Absent*)b)->codeMissing ) return  1;
   return ((Absent*)a)->code - ((Absent*)b)->code;
}