# Mastermind solution checker

Program to check the validity of a Mastermind solution produced by MMopt
One parameter is manditory, that is the name (and path if necessary) of a solution file, as written by MMopt
 ( For example SolnMM(4,6)_full_282970100085955.csv )
 ( Or /Users/brucetandy/Documents/Mastermind/Results/SolnMM(4,6)_full_282970100085955.csv )

This program will produce a short report to stdout giving details of number of pegs, number of colours..
..as well as the completeness and validity of the solution.

If the solution is not satisfactory, a list of codes not resolved and a list of erroneous resolutions will be reported.

This program makes no statement or claim about whether a solution is optimal or not