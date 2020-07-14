/* Libraries */
#include "program-utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* Libraries End*/

int check_constraint(const int N, const int M, const int T, const int S, const int L, const int K)
{
    int isPass = TRUE;
    if(N <= 2)
	{
		char *err_msg = "Error! Constraint: (N > 2)\n";
		write(STDERR_FILENO, err_msg, strlen(err_msg));
        isPass = FALSE;
    }
	if(S <= 3)
	{
		char *err_msg = "Error! Constraint: (S > 3)\n";
		write(STDERR_FILENO, err_msg, strlen(err_msg));
        isPass = FALSE;
    }
	if(T < 1)
	{
		char *err_msg = "Error! Constraint: (T >= 1)\n";
		write(STDERR_FILENO, err_msg, strlen(err_msg));
        isPass = FALSE;
	}
	if((M <= N) || (M <= 2))
	{
		char *err_msg = "Error! Constraint: (M > N > 2)\n";
		write(STDERR_FILENO, err_msg, strlen(err_msg));
        isPass = FALSE;
    }
	if(M <= T || T < 1)
	{
		char *err_msg = "Error! Constraint: (M > T >= 1)\n";
		write(STDERR_FILENO, err_msg, strlen(err_msg));
        isPass = FALSE;
    }
	if(L < 3)
	{
		char *err_msg = "Error! Constraint: (L >= 3)\n";
		write(STDERR_FILENO, err_msg, strlen(err_msg));
        isPass = FALSE;
    }

    return isPass;
}

char* handle_options(int argc, char **argv, int *N, int *M, int *T, int *S, int *L)
{
    if(argc != 13)
	{
		char *err_msg = OPT_USE_ERR;
		write(STDERR_FILENO, err_msg, strlen(err_msg));
		exit(EXIT_FAILURE);
	}
    int option;
    char *file_name;
    while ((option = getopt (argc, argv, "NMTSLF")) != -1)
  	{
	    if(option == 'N')
			*N = atoi(argv[optind]);
        else if(option == 'M')
			*M = atoi(argv[optind]);
		else if(option == 'T')
			*T = atoi(argv[optind]);
		else if(option == 'S')
			*S = atoi(argv[optind]);
		else if(option == 'L')
			*L = atoi(argv[optind]);	
		else if(option == 'F')
			file_name = argv[optind];
		else
        {
            char *err_msg = OPT_USE_ERR;
			write(STDERR_FILENO, err_msg, strlen(err_msg));
		    exit(EXIT_FAILURE);
        }			
	}
    return file_name;
}

int find_min(const int P, const int C, const int D)
{
	if (P <= C && P <= D) 
        return P;
    else if (C <= P && C <= D) 
        return C;
    else
        return D;
}