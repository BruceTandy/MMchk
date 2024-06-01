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
#ifndef MMOUTPUTSUMMARY_H
#define MMOUTPUTSUMMARY_H

#include "MMopt.h"

int outputSetup( Puzzle* pPuzzle );
char* outputFileSummary( Puzzle* pPuzzle, char* distribution, int argc, char **argv  );
int assessSummary( Puzzle* pPuzzle, Guess* pGuess, int* scores, int turns );
void outputScreenSummary( Puzzle* pPuzzle, char* distribution );

#endif  /* MMOUTPUTSUMMARY_H */