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
#ifndef MMUTILITY_H
#define MMUTILITY_H

#include "MMchk.h"

#include <stdbool.h>

int   stringToInt( char* str );
char* printCode( Repo* pRepo, unsigned short code, bool feasible, char* buffer );
char  marking( Repo* pRepo, unsigned short guess, unsigned short solution );
unsigned short getCode( Repo* pRepo, char* codeString );
int   getMark( Repo* pRepo, char* markString );

#endif  /* MMUTILITY_H */