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
#ifndef MMSORTFNS_H
#define MMSORTFNS_H

#include "MMchk.h"

int cmpCodeOrder(const void* a, const void* b);
int cmpLineOrder(const void* a, const void* b);
int cmpMarkOrder(const void* a, const void* b);
int cmpMarkLevel( Turn* a, Turn* b );
int cmpAbsentOrder(const void* a, const void* b);

#endif  /* MMSORTFNS_H */