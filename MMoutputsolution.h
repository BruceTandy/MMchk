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
#ifndef MMOUTPUTSOLUTION_H
#define MMOUTPUTSOLUTION_H

#include "MMopt.h"

int outputSolutionHeader( Puzzle* pPuzzle );
int outputSolution( Puzzle* pPuzzle, Guess* pGuess, char* pDetails, int turns );
int outputGuessAndMark( Puzzle* pPuzzle, char* pBuffer, int code, bool inCodeList, char mark );
int outputSolutionLine( Puzzle* pPuzzle, int code, char* pDetails, int turns );

#endif  /* MMOUTPUTSOLUTION_H */