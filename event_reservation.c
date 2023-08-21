#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<time.h>
#include <unistd.h>

#define CAPACITY 500 //  capacity of the event
#define MAX_EVENTS 100 // number of events
#define K_MIN 5 // minimum number of seats allowed
#define K_MAX 10 // maximum number of seats allowed
#define MAX_THREADS 20 // maximum number of threads allowed
#define MAX_ACTIVE_QUERIES 5 // maximum number of active threads to access the shared variable
#define MAX_TIME 600 // in sec

int event_array[MAX_EVENTS]; // array of event
int number_of_active_queries; // number of active queries accessing the database simultaneously
pthread_t threads[MAX_THREADS]; // mutex array of threads
pthread_mutex_t query_table; // mutex for query table
pthread_cond_t query_cond; // condition variable to access the shared variable
time_t now; // time when main thread is executing


/**
 * @brief structure of the query
 */
struct query
{
    int event_num;
    int type;
    int thread_num;
};

struct query active_queries[MAX_ACTIVE_QUERIES]; // array to store the number of active queries

/**
 * @brief structure to hold the active booking done by a thread
 * 
 */
struct thread_events{
    int event_id;
    int seats;
    struct thread_events *thread_events;
};

/**
 * @brief Get the random type of execution
 * 
 * @return int 
 */
int get_random_type(){
    return rand() % 3 + 1;
}

/**
 * @brief Get the random event number
 * 
 * @return int 
 */
int get_random_event_number(){
    return rand() % MAX_EVENTS;
}

/**
 * @brief Get the random seats in range of (5 - 10)
 * 
 * @return int 
 */
int get_random_seats(){
    return rand() % (K_MAX - K_MIN + 1) + K_MIN;
}

/**
 * @brief checking the access of the query to access the shared table
 * 
 * @param query 
 */
void checkForAccess(struct query query){
    pthread_mutex_lock(&query_table);
    
    // iterating till the thread gets access of the shared table
    while(1){
        int conflict_flag = 0;
        for(int i = 0; i < MAX_ACTIVE_QUERIES; i++){
            struct query active_query = active_queries[i];

            if(active_query.event_num == query.event_num){
                if(active_query.type > 1 || query.type > 1){
                    conflict_flag = 1;
                    printf("\nCONFLICT : (Event Number : %d, Thread Number : %d )\n",query.event_num,query.thread_num);
                    break;
                }
            }
        }

        // if no conflict is there then check for the availability in the shared table
        if(!conflict_flag){
            for(int i = 0; i < MAX_ACTIVE_QUERIES; i++){
                if(active_queries[i].event_num == -1){
                    active_queries[i] = query;
                    number_of_active_queries++;
                    printf("\nINSERTING : (Event Number : %d, Thread Number : %d )\n", query.event_num, query.thread_num);
                    pthread_mutex_unlock(&query_table);
                    return;
                }
            }

            printf("\nWAITING : (Event Number : %d, Thread Number : %d ).\n",query.event_num, query.thread_num);
            pthread_cond_wait(&query_cond, &query_table);
        }
        else{
            pthread_cond_wait(&query_cond, &query_table);
        }
    }
}

/**
 * @brief To release the query from the table
 * 
 * @param query 
 */
void releasing_lock(struct query query){
    pthread_mutex_lock(&query_table);
    for(int i = 0; i < MAX_ACTIVE_QUERIES; i++){
        if(active_queries[i].thread_num == query.thread_num){
            printf("\nSIGNALLING : (Event Number : %d, Thread Number : %d )\n", query.event_num, query.thread_num);
            active_queries[i].event_num = -1;
            number_of_active_queries -= 1;
            break;
        }
    }
    pthread_cond_signal(&query_cond);
    pthread_mutex_unlock(&query_table);
}

/**
 * @brief to inquire the current status of the event
 * 
 * @param query 
 * @return int 
 */

int inquiry(struct query query){
    checkForAccess(query);
    int val = event_array[query.event_num];
    releasing_lock(query);
    return val;
    
}

/**
 * @brief fuction to cancel the seats
 * 
 * @param query 
 * @param seats_to_cancel 
 * @return int 
 */
int cancellation_seats(struct query query, int seats_to_cancel){

    int status = 0; // 0 : unsuccessfull operation, 1 : successful operation
    checkForAccess(query);
    if(event_array[query.event_num]+seats_to_cancel <= 500){
        event_array[query.event_num] += seats_to_cancel;
        status = 1;
    }

    releasing_lock(query);
    return status;
}

/**
 * @brief Fuction to book the seats in the event
 * 
 * @param query 
 * @param reservation 
 * @return int 
 */
int booking_seats(struct query query, int reservation){
    
    int status = 0;
    checkForAccess(query);
    
    sleep(2);
    if(event_array[query.event_num] >= reservation){
        event_array[query.event_num] -= reservation;
        status = 1;
    }

    releasing_lock(query);
    return status;
}

/**
 * @brief To print the query
 * 
 * @param query 
 */
void print_query(struct query query){
    printf("\nThread Number : %d, Event Number : %d, Operation Type : %d\n", query.thread_num, query.event_num, query.type);
}

/**
 * @brief helper fuction which runs for all the threads
 * 
 * @param thread_num 
 * @return void* 
 */
void* helper(void* thread_num){
    int thread_number = *(int *) thread_num;
    srand(time(NULL) + thread_number);
    struct thread_events *head = NULL;
    int active_bookings = 0;

    while(1){

        // sleeping the thread for random amount of time
        sleep((rand() % MAX_THREADS) + 1);
        struct query query;
        query.thread_num = thread_number;
        query.event_num = get_random_event_number();
        query.type = get_random_type();
        
        print_query(query);

        int num_of_seats = 0, available_seats = 0;

        switch(query.type){
            case 1 :
                available_seats = inquiry(query); 
                printf("\n INSERTING : (Thread Num : %d, Event Num : %d, Seats : %d )\n", thread_number, query.event_num, available_seats);
                break;
            case 2 :
                // booking
                num_of_seats = get_random_seats();

                // start booking the seats
                int status = booking_seats(query, num_of_seats);

                if(status){
                    printf("\n BOOKING : (Thread Num : %d, Event Num : %d, Seats : %d )\n", thread_number, query.event_num, num_of_seats);

                    if(head == NULL){
                        head = malloc(sizeof(struct thread_events));
                        head->event_id = query.event_num;
                        head->seats = num_of_seats;
                        head->thread_events = NULL;
                        active_bookings += 1;
                    }else{
                        struct thread_events *temp = head;

                        while(temp != NULL){
                            if(temp->event_id == query.event_num){
                                temp->seats += num_of_seats;
                                break;
                            }

                            temp = temp->thread_events;
                        }

                        if(temp == NULL){
                            temp = malloc(sizeof(struct thread_events));
                            temp->event_id = query.event_num;
                            temp->seats = num_of_seats;
                            temp->thread_events = head;
                            head = temp;
                            active_bookings += 1;
                        }
                    }
                }else{
                    printf("\n UNABLE_TO_BOOK : (Thread Num : %d, Event Num : %d, Seats : %d )\n", thread_number, query.event_num, num_of_seats);
                }
                break;
            case 3 :
                // cancelling
                if(active_bookings == 0){
                    printf("\n NO_EVENTS_PRESENT_TO_CANCEL : (Thread Num : %d)\n", thread_number);
                }else{

                    int index = rand() % active_bookings;

                    struct thread_events *temp = head, *prev = NULL;
                    while(index > 0){
                        prev = temp;
                        temp = temp->thread_events;
                        index -= 1;
                    }

                    available_seats = temp->seats;
                    int seats_to_cancel = rand() % available_seats + 1;

                    status = cancellation_seats(query, seats_to_cancel);

                    if(status){
                        printf("\n CANCELLING : (Thread Num : %d, Event Num : %d, Seats : %d )\n", thread_number, query.event_num , seats_to_cancel);

                        if(prev == NULL){
                            // head node

                            if(seats_to_cancel == temp->seats){
                                head = temp->thread_events;
                                active_bookings -= 1;
                            }else{
                                temp->seats -= seats_to_cancel;
                            }

                        }else{
                            if(seats_to_cancel == temp->seats){
                                prev = temp->thread_events;
                                active_bookings -= 1;
                            }else{
                                temp->seats -= seats_to_cancel;
                            }
                        }
                    }
                }
                break;
            default :
                printf("\n INVALID OPERATION \n");
                break;
        }
        
        time_t curr = time(NULL);
        if ((curr - now) >= MAX_TIME) {
            printf("\nTIMEOUT : Thread Number : %d\n", thread_number);
           break;
        }
    }
}

int main(){
    
    pthread_cond_init(&query_cond, NULL);
    pthread_mutex_init(&query_table, NULL);
    
    now = time(NULL);
    for(int i = 0; i < MAX_EVENTS; i++){
        event_array[i] = 500;
    }

    for(int i = 0; i <MAX_ACTIVE_QUERIES; i++){
        active_queries[i].event_num = -1;
    }

    // creating multiple threads
    for(int i = 0; i < MAX_THREADS; i++){
        int* thread_number = malloc(sizeof(int));
        *thread_number = i + 1;
        pthread_create(&threads[i], NULL, helper, thread_number);
    }

    // joining all the threads
    for(int i = 0; i < MAX_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    pthread_cond_destroy(&query_cond);
    pthread_mutex_destroy(&query_table);

    // Current status for the events
    for(int i = 0; i < MAX_EVENTS; i++){
        printf("Event %d, remaining seats %d \n", i, event_array[i]);
    }

    return 0;

}
