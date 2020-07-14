/* Libraries */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "program-utils.h"
/* Libraries End*/

/* Macro Constants */
#define SUPP_COOK "/supplier-cook"
#define COOK_STUD "/cook-stud"
#define BLK_SIZE 512
/* Macro Constants End*/

/* Shared Memory Structs*/
struct Supplier_Cook
{
	sem_t b_sem;            		//like a binary semaphore   
    sem_t empty_sem;        		//empty semaphore 
    sem_t full_sem;         		//full semaphore
	sem_t sem_P;					//semaphore for soup
	sem_t sem_C;					//semaphore for main course
	sem_t sem_D;					//semaphore for desert
	int P;                  		//number of soup plates 
    int C;                  		//number of main course plates
    int D;                  		//number of desert plates 
	int total_plates;       		//counter for total plates of food 
	int total_taken_plates; 		//counter for total taken plates of food 
};
struct Cook_Stud
{
	sem_t b_sem;            	  	//like a binary semaphore   
    sem_t full_sem;         	  	//full semaphore
	sem_t table;					//table place that students eat 
	int P;                  	  	//number of soup plates 
    int C;                  	  	//number of main course plates
    int D;                  	  	//number of desert plates 
	int number_of_stud;				//number of students at counter
};
/* Shared Memory Structs End */

/* Typdefs */
typedef struct Supplier_Cook Kitchen;
typedef struct Cook_Stud Counter;
/* Typedefs End*/

/* Function Declarations */
int supplier_process(char*);		//process of supplier 
int cook_process(int);				//process of cook
int student_process(int);			//process of student
void init_supp_cook(int*);			//initializes shared memory and semaphores between supplier and cook
void init_cook_stud(int*);			//initializes shared memory and semaphores between cook and student
void end_supp_cook(const int);		//destroys shared memory and semaphores between supplier and cook
void end_cook_stud(const int);		//destroys shared memory and semaphores between cook and student
void handler(int);					//signal handler function
/* Function Declarations End */

/* Global Variables */
int N;              				//number of cook
int M;              				//number of student
int T;              				//number of table
int S;              				//number of counter
int L;              				//number of times get food from counter
int K;              				//size of kitchen
int process_number;					//total number of process
pid_t *child_pids;					//holds child process ids
Kitchen *kitchen_room; 				//shared memory between supplier-cook
Counter *counter_room;  			//shared memory between cook-student and student-student
int counter = 0;
/* Global Variables End */

int main(int argc, char *argv[])
{
	signal(SIGINT, handler);
	
	char *file_name = handle_options(argc, argv, &N, &M, &T, &S, &L);

    if(!check_constraint(N, M, T, S, L, K))
    {
        exit(EXIT_FAILURE);
    }

	process_number = N + M + 1;
	child_pids = (pid_t*)malloc(process_number);
	int fd_supp_cook;
	int fd_cook_stud;
	init_supp_cook(&fd_supp_cook);
	init_cook_stud(&fd_cook_stud);

	for(int i = 0; i < process_number; i++)
	{
        child_pids[i] = fork();
        if(child_pids[i] == -1)
        {
			char *err_msg = "fork(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
            exit(EXIT_FAILURE);
        }
        else if(child_pids[i] == 0)
        {
            if(i == 0)
		    {
				supplier_process(file_name);
				exit(EXIT_SUCCESS);
			}
			else if(i <= N)
			{
				cook_process(i);
				exit(EXIT_SUCCESS);
			}
			else
			{
				student_process(i%M);
				exit(EXIT_SUCCESS);
			}
        }
	}

    int status;
	pid_t pid;
    do
    {
        pid = waitpid(-1, &status, 0);
    }
	while (pid != -1);
	
	free(child_pids);
	end_supp_cook(fd_supp_cook);
	end_cook_stud(fd_cook_stud);

    return 0;
}


int supplier_process(char* input_path)
{
  	int max_plates = 3 * L * M;
	
	int fd_input = open(input_path, O_RDONLY);
	if(fd_input == -1)
	{
		char *err_msg = "open(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
        exit(EXIT_FAILURE);
	}
	
  	while(kitchen_room->total_plates < max_plates)
  	{
  		if(sem_wait(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

		char plate_type;
		int read_byte;
    	if(!((read_byte = read(fd_input, &plate_type, sizeof(char))) == -1) && (errno == EINTR))
    	{
			char *err_msg = "read(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
        	exit(EXIT_FAILURE);
    	}

		char msg[BLK_SIZE];
        switch (plate_type)
        {
        case 'P':
			sprintf(msg, "The supplier is going to the kitchen to deliver soup: kitchen items P:%d,C:%d,D:%d=%d\n", 
					kitchen_room->P, 
                    kitchen_room->C, 
                    kitchen_room->D, 
                    (kitchen_room->P + kitchen_room->C + kitchen_room->D));
            break;
        case 'C':
            sprintf(msg, "The supplier is going to the kitchen to deliver main course: kitchen items P:%d,C:%d,D:%d=%d\n", 
                    kitchen_room->P, 
                    kitchen_room->C, 
                    kitchen_room->D, 
                    (kitchen_room->P + kitchen_room->C + kitchen_room->D));
            break;
        case 'D':
            sprintf(msg, "The supplier is going to the kitchen to deliver desert: kitchen items P:%d,C:%d,D:%d=%d\n", 
                    kitchen_room->P, 
                    kitchen_room->C, 
                    kitchen_room->D, 
                    (kitchen_room->P + kitchen_room->C + kitchen_room->D));
            break;
		default:
			sprintf(msg, "Invalid plate type!\n");
			exit(EXIT_FAILURE);
        }
		write(STDOUT_FILENO, msg, strlen(msg));

  		if(sem_post(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
  		if(sem_wait(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

		int post_stat = 0;
        switch (plate_type)
        {
        case 'P':
            (kitchen_room->P)++;
  			sprintf(msg, "The supplier delivered soup - after delivery: kitchen items P:%d,C:%d,D:%d=%d\n", 
                    kitchen_room->P, 
                    kitchen_room->C, 
                    kitchen_room->D, 
                    (kitchen_room->P + kitchen_room->C + kitchen_room->D));
			post_stat = sem_post(&kitchen_room->sem_P);
            break;
        case 'C':
            (kitchen_room->C)++;
  			sprintf(msg, "The supplier delivered main course - after delivery: kitchen items P:%d,C:%d,D:%d=%d\n", 
                    kitchen_room->P, 
                    kitchen_room->C, 
                    kitchen_room->D, 
                    (kitchen_room->P + kitchen_room->C + kitchen_room->D));
			post_stat = sem_post(&kitchen_room->sem_C);
            break;
        default:
            (kitchen_room->D)++;
  			sprintf(msg, "The supplier delivered desert - after delivery: kitchen items P:%d,C:%d,D:%d=%d\n", 
                    kitchen_room->P, 
                    kitchen_room->C, 
                    kitchen_room->D, 
                    (kitchen_room->P + kitchen_room->C + kitchen_room->D));
			post_stat = sem_post(&kitchen_room->sem_D);
            break;
        }
		write(STDOUT_FILENO, msg, strlen(msg));

  		if(post_stat == -1)
		{
			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
			exit(EXIT_FAILURE);
		}

  		(kitchen_room->total_plates)++;

  		if(sem_post(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
  		if(sem_post(&kitchen_room->full_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
  	}
	close(fd_input);
	char *goodbye = "The supplier finished supplying - GOODBYE!\n";
	write(STDOUT_FILENO, goodbye, strlen(goodbye));

	return 0;
}

int cook_process(int number)
{
	int is_break = FALSE;
	int plate_type = 1;
  	int max_plates = 3 * L * M;

  	while(kitchen_room->total_plates < max_plates || (kitchen_room->P + kitchen_room->C + kitchen_room->D) > 0)
  	{
  		if(sem_wait(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

  		(kitchen_room->total_taken_plates)++;
		char msg[BLK_SIZE];
  	
  		if(kitchen_room->total_taken_plates <= max_plates)
  		{
  			sprintf(msg, "Cook %d going to the kitchen to wait for/get a plate - kitchen items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					kitchen_room->P, 
					kitchen_room->C, 
					kitchen_room->D, 
					(kitchen_room->P + kitchen_room->C + kitchen_room->D));
			write(STDOUT_FILENO, msg, strlen(msg));
  			plate_type = ((kitchen_room->total_taken_plates - 1) % 3) + 1;
  		}
		else
		{
  			(kitchen_room->total_taken_plates)--;
  			is_break = TRUE;
 		}

  		if(sem_post(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

  		if(is_break)
		{
			break;
		} 

		int wait_stat = 0;
		switch (plate_type)
		{
		case 1:
			wait_stat = sem_wait(&kitchen_room->sem_P);
			break;
		case 2:
			wait_stat = sem_wait(&kitchen_room->sem_C);
			break;
		case 3:
			wait_stat = sem_wait(&kitchen_room->sem_D);
			break;
		default:
			break;
		}
  		
		if (wait_stat == -1)
		{
			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
	  		exit(EXIT_FAILURE);
		}
		if (sem_wait(&kitchen_room->full_sem) == -1)
		{
			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
			exit(EXIT_FAILURE);
		}
  		if (sem_wait(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
  		if (sem_wait(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

		switch (plate_type)
		{
		case 1:
  			(kitchen_room->P)--;
  			sprintf(msg, "Cook %d is going to the counter to deliver soup – counter items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					counter_room->P, 
					counter_room->C, 
					counter_room->D, 
					(counter_room->P + counter_room->C + counter_room->D));		
			break;
		case 2:
			(kitchen_room->C)--;
  			sprintf(msg, "Cook %d is going to the counter to deliver main course – counter items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					counter_room->P, 
					counter_room->C, 
					counter_room->D, 
					(counter_room->P + counter_room->C + counter_room->D));
			break;
		case 3:
			(kitchen_room->D)--;
  			sprintf(msg, "Cook %d is going to the counter to deliver desert – counter items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					counter_room->P, 
					counter_room->C, 
					counter_room->D, 
					(counter_room->P + counter_room->C + counter_room->D));
			break;
		default:
			break;
		}
		write(STDOUT_FILENO, msg, strlen(msg));

  		if(sem_post(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}	

  		if(sem_post(&kitchen_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

  		if(sem_post(&kitchen_room->empty_sem) == -1)
		{
			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
			exit(EXIT_FAILURE);
		}

  		if(sem_wait(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
  		
  		if(plate_type == 1)
  		{
  			(counter_room->P)++;
  			sprintf(msg, "Cook %d placed soup on the counter - counter items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					counter_room->P, 
					counter_room->C, 
					counter_room->D, 
					(counter_room->P + counter_room->C + counter_room->D));
  		}
  		else if(plate_type == 2)
  		{
  			(counter_room->C)++;
  			sprintf(msg, "Cook %d placed main course on the counter - counter items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					counter_room->P, 
					counter_room->C, 
					counter_room->D, 
					(counter_room->P + counter_room->C + counter_room->D));
  		}
  		else
  		{
  			(counter_room->D)++;
  			sprintf(msg, "Cook %d placed desert on the counter - counter items P:%d,C:%d,D:%d=%d\n", 
			  		number, 
					counter_room->P, 
					counter_room->C, 
					counter_room->D, 
					(counter_room->P + counter_room->C + counter_room->D));
  		}
		write(STDOUT_FILENO, msg, strlen(msg));

  		if(sem_post(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

  		if (((counter_room->P > 0) && (counter_room->C > 0) && (counter_room->D > 0)))
		{
			int sem_val;
			sem_getvalue(&counter_room->full_sem, &sem_val);
			if(sem_val < find_min(counter_room->P, counter_room->D, counter_room->D))
			{
				if(sem_post(&counter_room->full_sem) == -1)
				{	
					char *err_msg = "sem_post(): unsuccessful!\n";
					write(STDERR_FILENO, err_msg, sizeof(err_msg));
					exit(EXIT_FAILURE);
				}
			}
		}
		  
  	}
	char goodbye[BLK_SIZE];
  	sprintf(goodbye, "Cook %d finished serving - items at kitchen: %d - going home - GOODBYE!!!\n", 
	  		number, 
			(kitchen_room->P + kitchen_room->C + kitchen_room->D));
	write(STDOUT_FILENO, goodbye, strlen(goodbye));
	return 0;
}

int student_process(int number)
{
	int total_eat = 0;
	char msg[BLK_SIZE];
	while(total_eat < L)
	{
		total_eat++;
  		if(sem_wait(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
		(counter_room->number_of_stud)++;
		sprintf(msg, "Student %d is going to the counter (round %d) - # of students at counter: %d and counter items P:%d,C:%d,D:%d=%d\n",
				number,
				total_eat,
				counter_room->number_of_stud,
				counter_room->P,
				counter_room->C,
				counter_room->D,
				(counter_room->P + counter_room->C + counter_room->D));
		write(STDOUT_FILENO, msg, strlen(msg));
		if(sem_post(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

		if(sem_wait(&counter_room->full_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
		(counter_room->P)--;
		(counter_room->C)--;
		(counter_room->D)--;
		if(sem_wait(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_wait(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}

		sprintf(msg, "Student %d got food and is going to get a table (round %d) - # of empty tables: %d\n",
				number,
				total_eat,
				0);
		write(STDOUT_FILENO, msg, strlen(msg));
		(counter_room->number_of_stud)--;
		if(sem_post(&counter_room->b_sem) == -1)
  		{
  			char *err_msg = "sem_post(): unsuccessful!\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg));
  			exit(EXIT_FAILURE);
  		}
		
		if(sem_wait(&counter_room->table) == -1)
  		{
  			perror("sem_wait");
  			exit(EXIT_FAILURE);
  		}
		int table_val;
		sem_getvalue(&counter_room->table, &table_val);
		sprintf(msg, "Student %d sat at table %d to eat (round %d) - empty tables: %d\n", number, table_val, total_eat, T - table_val);
		write(STDOUT_FILENO, msg, strlen(msg));
		if(table_val < T)
		{
			if(sem_post(&counter_room->table) == -1)
			{
				char *err_msg = "sem_post(): unsuccessful!\n";
				write(STDERR_FILENO, err_msg, sizeof(err_msg));
  				exit(EXIT_FAILURE);
			}
			if(total_eat < L)
			{
				int table_val;
				sem_getvalue(&counter_room->table, &table_val);
				sprintf(msg, "Student %d left table %d to eat again (round %d) - empty tables:%d\n", number, table_val - 1, total_eat, table_val);
				write(STDOUT_FILENO, msg, strlen(msg));
			}
		}
	}
	sprintf(msg, "Student %d is done eating %d times - going home - GOODBYE!!!\n", number, total_eat);
	write(STDOUT_FILENO, msg, strlen(msg));
	return 0;
}

void init_supp_cook(int* fd_supp_cook)
{
	*fd_supp_cook = shm_open(SUPP_COOK, O_CREAT | O_RDWR, 0666);
	if (*fd_supp_cook < 0)
	{
		char *err_msg = "shm_open(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
		exit(EXIT_FAILURE);
	}

	if(ftruncate(*fd_supp_cook, sizeof(Kitchen)) == -1)
	{
	    char *err_msg = "ftruncate(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
	    exit(EXIT_FAILURE);
  	}

	kitchen_room = (Kitchen *)mmap(NULL, sizeof(Kitchen), PROT_READ | PROT_WRITE, MAP_SHARED, *fd_supp_cook, 0);
    if(kitchen_room == MAP_FAILED)
	{
		char *err_msg = "mmap(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
		exit(EXIT_FAILURE);
	}
    K = 2 * L * M + 1;
	kitchen_room->P = 0;
	kitchen_room->C = 0;
	kitchen_room->D = 0;
	kitchen_room->total_plates = 0;
	kitchen_room->total_taken_plates = 0;
	if((sem_init(&kitchen_room->b_sem, 1, 1) == -1) || 
       (sem_init(&kitchen_room->empty_sem, 1, K) == -1) || 
       (sem_init(&kitchen_room->full_sem, 1, 0) == -1) || 
	   (sem_init(&kitchen_room->sem_P, 1, 0) == -1) || 
	   (sem_init(&kitchen_room->sem_C, 1, 0) == -1) || 
	   (sem_init(&kitchen_room->sem_D, 1, 0) == -1))
	{
		char *err_msg = "sem_init(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
		exit(EXIT_FAILURE);
	}
}

void init_cook_stud(int *fd)
{
	*fd = shm_open(COOK_STUD, O_CREAT | O_RDWR, 0666);
	if (*fd < 0)
	{
		char *err_msg = "shm_open(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
		exit(EXIT_FAILURE);
	}

	if(ftruncate(*fd, sizeof(Counter)) == -1)
	{
	    char *err_msg = "ftruncate(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
	    exit(EXIT_FAILURE);
  	}

	counter_room = (Counter *)mmap(NULL, sizeof(Counter), PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if(counter_room == MAP_FAILED)
	{
		char *err_msg = "mmap(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
		exit(EXIT_FAILURE);
	}
	counter_room->P = 0;
	counter_room->C = 0;
	counter_room->D = 0;
	counter_room->number_of_stud = 0;
	if((sem_init(&counter_room->b_sem, 1, 1) == -1) ||  
       (sem_init(&counter_room->full_sem, 1, 0) == -1) ||
	   (sem_init(&counter_room->table, 1, T)))
	{
		char *err_msg = "sem_init(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
		exit(EXIT_FAILURE);
	}
}

void end_supp_cook(const int fd_supp_cook)
{
	if(munmap(kitchen_room, sizeof(Kitchen)) == -1)
  	{
  		char *err_msg = "munmap(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
  		exit(EXIT_FAILURE);
  	}

  	if(close(fd_supp_cook) == -1)
  	{
  		char *err_msg = "close(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
  		exit(EXIT_FAILURE);
  	}

  	if(shm_unlink(SUPP_COOK) == -1)
  	{
  		char *err_msg = "shm_unlink(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
  		exit(EXIT_FAILURE);
  	}
}

void end_cook_stud(const int fd_cook_stud)
{
	if(munmap(counter_room, sizeof(Counter)) == -1)
  	{
  		char *err_msg = "munmap(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
  		exit(EXIT_FAILURE);
  	}

  	if(close(fd_cook_stud) == -1)
  	{
  		char *err_msg = "close(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
  		exit(EXIT_FAILURE);
  	}

  	if(shm_unlink(COOK_STUD) == -1)
  	{
  		char *err_msg = "shm_unlink(): unsuccessful!\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg));
  		exit(EXIT_FAILURE);
  	}
}

void handler(int sig)
{
	if (sig == SIGINT)
	{
		for(int i = 0; i < process_number; i++)
    		kill(child_pids[i], SIGKILL);
		free(child_pids);
    	exit(EXIT_SUCCESS);
	}
}