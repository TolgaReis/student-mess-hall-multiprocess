#ifndef PROGRAM_UTILS_H
#define PROGRAM_UTILS_H

/* Macro Constants */
#define OPT_USE_ERR "Wrong input option usage! Use such: ./program -N 3 -M 12 -T 5 -S 4 -L 13\n"
#define TRUE 1
#define FALSE 0
/* Macro Constants End */

/* Function Definitions */
int check_constraint(const int, const int, const int, const int, const int, const int);
char* handle_options(int, char**, int*, int*, int*, int*, int*);
void choose_sent(const int, int*, const int, const int, const int, const int);
int find_min(const int, const int, const int);
/* Function Definitions End*/

#endif
