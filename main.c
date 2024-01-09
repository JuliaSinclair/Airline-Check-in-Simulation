#include <pthread.h>  //pthread library
#include <stdio.h>  //printf()
#include <stdlib.h> //exit()
#include <string.h> //strtok()
#include <sys/time.h> //gettimeofday()
#include <unistd.h>  //usleep()
#include <ctype.h> //isdigit()

#define NClerks 5	//as per Assignment 2 description
#define NQUEUE 2	//as per Assignment 2 description
#define TIME_CONVERSION 100000 // 10ths of seconds to micoseconds 
#define MAX_CUSTOMER 30 //Maximum number of customers specified by Kui Wui
#define MAX_SIZE 1024	

void *customer_entry(void *cus_info); 

// struct to record the customer information read from input files
typedef struct customer_info{
    int user_id;
	int class_type;
	int arrival_time;
	int service_time;
} customer_info;

/* global variables */
double init_time; 
double overall_waiting_time[NQUEUE]; 
int queue_length[NQUEUE];
pthread_mutex_t mutex_customer[NQUEUE] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
pthread_mutex_t mutex_clerk = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t convar_clerk = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_economy = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_business = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_time[NQUEUE] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
int business_queue[MAX_CUSTOMER];
int economy_queue[MAX_CUSTOMER];
int clerk[NClerks];

// --------------------------------------Helper Functions-----------------------------------------------
/*
	Function: avg_time
	Arguments:
		time: total waiting time
		people: number of people 
	Purpose: Calculates average waiting time for group of people
	Error Prevention: Prevents from outputting inf
*/
double avg_time(double time, double people){
	if(time == 0){
		return 0;
	}
	else{
		return time/people;
	}
}
/*
	Function: clerk_available
	Purpose: Checks if any clerks are available 
*/
int clerk_available(){
	int i;
	int number_available = 0;
	for(i = 0; i < NClerks; i++){
		if(clerk[i] == 0) {
			number_available++;
		}
	}
	return number_available;
}

/*
	Function: deQueue
	Arguments:
		customer: Customer information (user id, class type, arrival time, service time)
	Purpose: Removes customer from thier class queue 
*/
void deQueue(customer_info* customer){
	int i, position;
	if (customer->class_type == 0){
		pthread_mutex_lock(&mutex_economy);
		for (i=0; i < queue_length[0]; i++){
			if(economy_queue[i] == customer->user_id){
				position = i;
			}
		}
		for (i=position; i < queue_length[0]; i++){
			economy_queue[i] = economy_queue[i+1];
		}
		queue_length[0] --;
		pthread_mutex_unlock(&mutex_economy);
	}
		else{
		pthread_mutex_lock(&mutex_business);
		for (i=0; i < queue_length[1]; i++){
			if(business_queue[i] == customer->user_id){
				position = i;
			}
		}
		for (i=position; i < queue_length[1]; i++){
			business_queue[i] = business_queue[i+1];
		}
		queue_length[1] --;
		pthread_mutex_unlock(&mutex_business);
	}
}

/*
	Function: enQueue
	Arguments:
		customer: Customer information (user id, class type, arrival time, service time)
	Purpose: Adds customer to the end of thier class queue 
*/
void enQueue(customer_info* customer){
	if (customer->class_type == 0){
		pthread_mutex_lock(&mutex_economy);
		economy_queue[queue_length[0]] = customer->user_id;
		queue_length[0] ++;
		fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 0, queue_length[0]);
		pthread_mutex_unlock(&mutex_economy);
	}
		else{
		pthread_mutex_lock(&mutex_business);
		business_queue[queue_length[1]] = customer->user_id;
		queue_length[1] ++;
		fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 1, queue_length[1]);
		pthread_mutex_unlock(&mutex_business);
	}
}

/*
	Function: getSystemTime
	Purpose: Returns relative machine time
	Rerference: Modified from sample_gettimeofday from CSC 360 Tutorial 4
*/ 
double getSystemTime(){
	struct timeval cur_time;
	double cur_secs;

	gettimeofday(&cur_time, NULL);
	cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);

	double waiting_time = cur_secs - init_time;

	return waiting_time;
}

/*
	Function: not_queue_head
	Arguments:
		customer: Customer information (user id, class type, arrival time, service time)
	Purpose: Checks if customer is at the head for thier class queue
	Returns: True (1) if not head, False(0) if head of queue 
*/
int not_queue_head(customer_info* customer){
	int not_head = 1;
	if (customer->class_type == 0){
		pthread_mutex_lock(&mutex_economy);
		if(economy_queue[0] == customer->user_id){
			not_head = 0;
		}
		pthread_mutex_unlock(&mutex_economy);
	}
	else {
		pthread_mutex_lock(&mutex_business);
		if(business_queue[0] == customer->user_id){
			not_head = 0;
		}
		pthread_mutex_unlock(&mutex_business);
	}
	return not_head;
}


//-------------------------------------------Main-------------------------------------------------------
int main(int argc, char *argv[]) {

	//error handling - making sure file name was inputted
	if(argv[1] == NULL){
        fprintf(stderr, "Please input a file \n");
        exit(1);
    }

	//error handling - inputs file is readable
    FILE * file = fopen(argv[1], "r");
    if ((file == NULL)) {
        fprintf(stderr, "Cannot open input file \n");
        exit(1);
     }

	//reading inputs file
    char input [MAX_SIZE][MAX_SIZE];
    int i = 0;
	int lines = 0;
    while(fgets(input[i], MAX_SIZE, file) != NULL) {
            i++;
			lines++;
    }
	 fclose(file);

	//error handling - checking that file is not empty
	 if(lines == 0){
		fprintf(stderr, "Check File! The input file is empty.\n");
		exit(1);
	 }

	//error handling - checking that specified number of customers in inputs file is correct
	int number_of_customers = atoi(input[0]); 
	 if(lines - 1 != number_of_customers || !isdigit(*input[0])){
		fprintf(stderr, "Check File! Number of customers specified in input file does not match number of customers in input file\n");
		exit(1);
	}
	
	//tokenising inputs file
	int number_of_economy = 0;
	int number_of_business = 0;  
    struct customer_info custom_info[number_of_customers];
    int customers_iterator = 0;
    for(i = 1; i <= number_of_customers; i++){ 
        int details[4];
        int details_iterator = 0;
        char delimt[] = ":,";
        char* token = strtok(input[i], delimt);
        while(token != NULL){
			//error handling - making sure all inputs are integers
			if (isdigit(*token) == 0){
				fprintf(stderr, "Check File! Make sure all inputs are integers.\n");
				exit(1);
			}
            details[details_iterator] = atoi(token);
			
            token = strtok(NULL, delimt);
            details_iterator++;
        }

		//adding information from input file into customer structure
        custom_info[customers_iterator].user_id = details[0];
        custom_info[customers_iterator].class_type = details[1];
		custom_info[customers_iterator].arrival_time = details[2];
        custom_info[customers_iterator].service_time = details[3];

		//error handling for input values
		//(1) class type is exclusively 0 or 1 
		if(custom_info[customers_iterator].class_type != 0 && custom_info[customers_iterator].class_type != 1){
			fprintf(stderr, "Check File! Incorrect Class Value for Customer ID: %d\n", custom_info[customers_iterator].user_id);
			exit(1);
		}
		//(2) arrive and service time are postive integers
		if(custom_info[customers_iterator].arrival_time < 0 || custom_info[customers_iterator].service_time < 0){
			printf("Check File! Arrival or service time is a negative integer for customer ID: %d\n", custom_info[customers_iterator].user_id);
			exit(1);
		}
		
		//counting number of customers in business or economy class
		if(custom_info[customers_iterator].class_type == 0){
			number_of_economy++;
		}
		if(custom_info[customers_iterator].class_type == 1){
			number_of_business++;
		}
        customers_iterator++;
	}

	// clerk[] represents clerks; if 0 then available, if 1 then busy serving customer thread.
	// Start all clerks at 0 (available)
	for(i=0; i < NClerks; i++){
		clerk[i] = 0;
	}

	//Initialize machine time
	static struct timeval start_time;
	gettimeofday(&start_time, NULL);
	init_time = (start_time.tv_sec + (double) start_time.tv_usec / 1000000);

	//Create customer threads; one for each customer, pass in custom_info information
	pthread_t customId[number_of_customers];
	for(i = 0; i < number_of_customers; i++){
		//error handling - if pthread_create fails
		if(pthread_create(&customId[i], NULL, customer_entry, (void *)&custom_info[i]) != 0){
			fprintf(stderr, "Failed to create thread for customer ID: %d\n", custom_info[i].user_id);
			exit(1);
		};
	}

	// Wait for all customer threads to terminate
	for(i=0; i < number_of_customers; i++){
		pthread_join(customId[i], NULL);
	}
	
	// destroy mutex & condition variable
	if (pthread_mutex_destroy(&mutex_customer[0]) != 0){
		fprintf(stderr, "Failed to destroy mutex_customer");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex_customer[1]) != 0){
		fprintf(stderr, "Failed to destroy mutex_customer");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex_clerk) != 0){
		fprintf(stderr, "Failed to destroy mutex_clerk");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex_economy) != 0){
		fprintf(stderr, "Failed to destroy mutex_economy");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex_business) != 0){
		fprintf(stderr, "Failed to destroy mutex_business");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex_time[0]) != 0){
		fprintf(stderr, "Failed to destroy mutex_clerk");
		exit(1);
	}
	if (pthread_mutex_destroy(&mutex_time[1]) != 0){
		fprintf(stderr, "Failed to destroy mutex_clerk");
		exit(1);
	}
	if (pthread_cond_destroy(&convar_clerk) != 0){
		fprintf(stderr, "Failed to destroy convar_clerk");
		exit(1);
	}

	// Calculate the average waiting time of all customers
	double avg = avg_time(overall_waiting_time[0]+overall_waiting_time[1], number_of_customers);
	double avg_business = avg_time(overall_waiting_time[1], number_of_business);
	double avg_economy = avg_time(overall_waiting_time[0], number_of_economy);


	fprintf(stdout, "The average waiting time for all customers in the system is: %.2f seconds. \n", avg);
	fprintf(stdout, "The average waiting time for all business-class customers is: %.2f seconds. \n", avg_business);
	fprintf(stdout, "The average waiting time for all economy-class customers is: %.2f seconds. \n", avg_economy);

	return 0;
}

//--------------------------------------------Customer---------------------------------------------------
/*
	Function: customer_entry
	Arguments:
		cus_info: Customer information (user id, class type, arrival time, service time)
	Purpose: Function of entry for customer thread 
*/
void * customer_entry(void * cus_info){

	struct customer_info * p_myInfo = (struct customer_info *) cus_info;

	//Thread sleep until customer arrives
	usleep(p_myInfo->arrival_time*TIME_CONVERSION);
	fprintf(stdout, "A customer arrives: customer ID %2d. \n", p_myInfo->user_id);

	//Enter queue and save relative machine time for when customer entered queue
	enQueue(p_myInfo);
	double queue_enter_time = getSystemTime();

	//Wait for clerk to serve customer thread
	pthread_mutex_lock(&mutex_customer[p_myInfo->class_type]);
	while (not_queue_head(p_myInfo) || clerk_available() == 0 || (p_myInfo->class_type == 0 && queue_length[1] >= clerk_available())) {
		pthread_cond_wait(&convar_clerk, &mutex_customer[p_myInfo->class_type]);
	}
	pthread_mutex_unlock(&mutex_customer[p_myInfo->class_type]);

	//When clerk becomes available for this thread, mark clerk as busy (1)
	int clerkID, i;
	pthread_mutex_lock(&mutex_clerk);
	for(i=0; i < NClerks; i++){
		if(clerk[i] == 0){
			clerkID = i+1;
			clerk[i] = 1;
			break;
		}
	}
	pthread_mutex_unlock(&mutex_clerk);

	//Leave queue and save machine time for when customer left queue
	deQueue(p_myInfo);
	double exit_queue_time = getSystemTime();
	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", exit_queue_time, p_myInfo->user_id, clerkID);

	//Thread sleeps for service time
	usleep(p_myInfo->service_time*TIME_CONVERSION);
	double clerk_finish_time;
	clerk_finish_time = getSystemTime();
	fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", clerk_finish_time, p_myInfo->user_id, clerkID);

	//Now clerk has finsihed serving this customer, mark clerk as available (0)
	pthread_mutex_lock(&mutex_clerk);
	int clerk_position = clerkID - 1;
	clerk[clerk_position] = 0;
	pthread_mutex_unlock(&mutex_clerk);

	//Signal all other wating customers that clerk is available 
	pthread_cond_broadcast(&convar_clerk);

	//Calculate wait time and to overall waiting time 
	pthread_mutex_lock(&mutex_time[p_myInfo->class_type]);
	double waiting_time = exit_queue_time - queue_enter_time;
	
	if(p_myInfo->class_type == 0){
		overall_waiting_time[0] += waiting_time;
	}
	else {
		overall_waiting_time[1] += waiting_time;
	}
	pthread_mutex_unlock(&mutex_time[p_myInfo->class_type]);

	//Customer finished
	pthread_exit(NULL);
	return NULL;
}
