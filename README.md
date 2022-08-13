# c-threading
basic threading program in c<br>
To run: gcc --std=gnu99 -o a main.c -lm -pthread  <br>
then: ./a < inputX.txt <br>

Here is how it works<br>
    Thread 1, called the Input Thread, reads in lines of characters from the standard input.<br>
    Thread 2, called the Line Separator Thread, replaces every line separator in the input by a space. <br>
    Thread, 3 called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", by a "^". <br>
    Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters. <br>
Each acts as a producer and consumer, and uses a semaphore to address race conditions and wait times.
