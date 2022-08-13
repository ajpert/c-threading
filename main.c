#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <err.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h> 
#include <stdint.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

 //holds a shared buffer and semaphore for synch
struct buffer {
    sem_t* sem;
    char shared_mem[50][1000];
    size_t shared_mem_size;
};

//holds two buffers to pass into thread calls
struct binary_buffer {
    struct buffer* in_buf;
    struct buffer* out_buf;
};

//fills buffer with input from a thread
void fill_buffer(struct buffer* buf, char input[1000], int curr_buff_line) {
    int val;
    do {
      /* Loop while buffer is full */
      sem_getvalue(buf->sem, &val);
    } while (val >= buf->shared_mem_size);
    
    strcpy(buf->shared_mem[curr_buff_line], input);
    sem_post(buf->sem); //increment the semaphore
}
void eat_buffer(struct buffer* buf, char input[1000], int curr_buff_line) {
    int val;
    sem_wait(buf->sem); //check if you can decrement the semaphore, else wait
    strcpy(input, buf->shared_mem[curr_buff_line]);
    sem_getvalue(buf->sem,&val);
}

//get input and check if there is a STOP call
void* input_thread(void* args) {
    bool cont = true;
    char input[1000];
    int curr_buff_line = 0;
    while(cont) { //while a stop line is not given
        fgets(input, 1000, stdin);
        
        if(strstr(input, "STOP\n") != NULL) { //if there is a stop call 
            int stop_location = strstr(input, "STOP\n") - input;
            if(stop_location == 0 || (input[stop_location - 1]) == ' ') { //if it is a legal stop, put in a notifier
                input[stop_location] = '\1';
                cont = false;
            }
        }
   
        fill_buffer(((struct binary_buffer*)args)->out_buf,input,curr_buff_line);
        
        curr_buff_line = curr_buff_line + 1;

        memset(input,0,strlen(input));
    }
   
    return 0;
}

//Output thread, print every 80 characters
void* output_thread(void* args) {
    int len;
    int curr_buff_line = 0;
    char input[1000];
    char line_helper[2000];
    bool cont = true;
    int i;
    while(cont) {
        eat_buffer(((struct binary_buffer*)args)->in_buf,input,curr_buff_line);

        
        if(strstr(input,"\1")) { //if there was a stop call, only use chars up to the stop notifier
            cont = false;
            len = strstr(input,"\1") - input;
        }
        else {
            len = strlen(input);
        }
        
        input[len] = '\0'; //needed for strcat
        strcat(line_helper,input); //add input to another string for character count

        //print only lines 80 characters long and remove those characters
        while(strlen(line_helper) >= 80) {
            printf("%.*s\n", 80, line_helper);
            fflush(stdout);
            strcpy(line_helper, line_helper+80);
        }
        fflush(stdout);

        curr_buff_line++;
        memset(input,0,strlen(input));
    }
    return 0;
}

//Thread to change ++ to ^
void* plus_sign_thread(void* args) {
    char input[1000];
    int curr_buff_line = 0;
    bool cont = true;
    char front_string[1000];
    char back_string[1000];
    char full_string[1000];
    while(cont) {
        eat_buffer(((struct binary_buffer*)args)->in_buf, input, curr_buff_line);

        while(strstr(input, "++") != NULL) { //check for ++, if it cotains, turn into ^
            strncpy(front_string, input, strstr(input, "++") - input);
            strcpy(back_string, strstr(input, "++") + 2);
            sprintf(full_string, "%s%s%s", front_string, "^",back_string);
            strcpy(input,full_string);
            memset(full_string,0,strlen(full_string));
            memset(front_string,0,strlen(front_string));
            memset(back_string,0,strlen(back_string));
        }

        if(strstr(input,"\1")) { //if there was a stop call
            cont = false;
        }

        fill_buffer(((struct binary_buffer*)args)->out_buf,input,curr_buff_line);
        
        curr_buff_line++;

        memset(input,0,strlen(input));
    }
}

//Thread for removing the newline character from input
void* line_seperataed_thread(void* args) {
    char input[1000];
    int curr_buff_line = 0;
    bool cont = true;
    while(cont) {
        eat_buffer(((struct binary_buffer*)args)->in_buf,input, curr_buff_line);
        
        while(strstr(input, "\n") != NULL) { //check for newline character and change to space if so
            input[strstr(input,"\n") - input] = ' ';
        }
        
        if(strstr(input,"\1")) { //if there was a stop call in the input thread
            cont = false;
        }

        fill_buffer(((struct binary_buffer*)args)->out_buf, input, curr_buff_line);
        memset(input,0,strlen(input));
        curr_buff_line++;
    }
    
}

int main() {
    //define variables
    pthread_t t_1;
    pthread_t t_2;
    pthread_t t_3;
    pthread_t t_4;

    struct buffer* input_seperate_buffer;
    struct buffer* seperate_plus_buffer;
    struct buffer* plus_output_buffer;

    struct binary_buffer* bin_buf_1;
    struct binary_buffer* bin_buf_2;
    struct binary_buffer* bin_buf_3;
    struct binary_buffer* bin_buf_4;

    //initialize buffers
    input_seperate_buffer = calloc(1,sizeof(struct buffer));
    seperate_plus_buffer = calloc(1,sizeof(struct buffer));
    plus_output_buffer = calloc(1,sizeof(struct buffer));

    input_seperate_buffer->shared_mem_size = sizeof input_seperate_buffer->shared_mem;
    seperate_plus_buffer->shared_mem_size = sizeof seperate_plus_buffer->shared_mem;
    plus_output_buffer->shared_mem_size = sizeof plus_output_buffer->shared_mem;

    //initialize semaphores
    sem_t sem1;
    input_seperate_buffer->sem = &sem1;
    sem_init(&sem1, 0, 0);

    sem_t sem2;
    seperate_plus_buffer->sem = &sem2;
    sem_init(&sem2,0,0);
    
    sem_t sem3;
    plus_output_buffer->sem = &sem3;
    sem_init(&sem3, 0, 0);

    
    //initialize binary buffers
    bin_buf_1 = calloc(1,sizeof(struct binary_buffer));
    bin_buf_2 = calloc(1,sizeof(struct binary_buffer));
    bin_buf_3 = calloc(1,sizeof(struct binary_buffer));
    bin_buf_4 = calloc(1,sizeof(struct binary_buffer));

    bin_buf_1->in_buf = NULL;
    bin_buf_1->out_buf = input_seperate_buffer;

    bin_buf_2->in_buf = input_seperate_buffer;
    bin_buf_2->out_buf = seperate_plus_buffer;

    bin_buf_3->in_buf = seperate_plus_buffer;
    bin_buf_3->out_buf = plus_output_buffer;

    bin_buf_4->in_buf = plus_output_buffer;
    bin_buf_4->out_buf = NULL;

    
    //start threads
    pthread_create(&t_1, NULL, input_thread, bin_buf_1);
    pthread_create(&t_2, NULL, output_thread, bin_buf_4);
    pthread_create(&t_3, NULL, plus_sign_thread, bin_buf_3);
    pthread_create(&t_4, NULL, line_seperataed_thread, bin_buf_2);

    //end threads
    pthread_join(t_1, NULL);
    pthread_join(t_2, NULL);
    pthread_join(t_3, NULL);
    pthread_join(t_4, NULL);

    //free semaphores
    sem_destroy(input_seperate_buffer->sem);
    sem_destroy(seperate_plus_buffer->sem);
    sem_destroy(plus_output_buffer->sem);

    //free buffers
    free(input_seperate_buffer);
    free(seperate_plus_buffer);
    free(plus_output_buffer);
}