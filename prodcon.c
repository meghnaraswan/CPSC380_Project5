#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* buffer.h */
#define BUFFER_SIZE 30
#define NUM_ITEMS 1

typedef struct {
    uint16_t cksum;
    uint8_t buffer[BUFFER_SIZE];
} buffer_item;

buffer_item buf_item[NUM_ITEMS];
sem_t sem_empty, sem_full, sem_mutex;
pthread_mutex_t mutex;
int in = 0, out = 0, counter = 0;

uint16_t calculate_checksum(uint8_t *buffer, int size) {
    uint16_t cksum = 0;
    int i;
    for (i = 0; i < size; i++) {
        cksum += buffer[i];
    }
    return cksum;
}

int insert_item(buffer_item item) {
    pthread_mutex_lock(&mutex);
    if (counter < BUFFER_SIZE) {
        buf_item[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        counter++;
        pthread_mutex_unlock(&mutex);
        return 0;
    } else {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
}

int remove_item(buffer_item *item) {
    pthread_mutex_lock(&mutex);
    if (counter > 0) {
        *item = buf_item[out];
        out = (out + 1) % BUFFER_SIZE;
        counter--;
        pthread_mutex_unlock(&mutex);
        return 0;
    } else {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
}

void *producer(void *arg) {
    int i;
    buffer_item item;
    for (i = 0; i < NUM_ITEMS; i++) {
        /* produce an item in next.produced */
        int j;
        for (j = 0; j < BUFFER_SIZE; j++) {
            item.buffer[j] = rand() % 256;
        }
        item.cksum = calculate_checksum(item.buffer, BUFFER_SIZE);
        sem_wait(&sem_empty);
        pthread_mutex_lock(&mutex);
        /* add next.produced to the buffer */
        insert_item(item);
        pthread_mutex_unlock(&mutex);
        sem_post(&sem_full);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    int i;
    buffer_item item;
    for (i = 0; i < NUM_ITEMS; i++) {
        sem_wait(&sem_full);
        pthread_mutex_lock(&mutex);
        /* remove an item from buffer to next.consumed */
        remove_item(&item);
        pthread_mutex_unlock(&mutex);
        sem_post(&sem_empty);
        /* consume the item in next.consumed */
        uint16_t cksum = 0;
        int j;
        for (j = 0; j < BUFFER_SIZE; j++) {
            cksum += item.buffer[j];
        }
        if (item.cksum != cksum) {
            printf("Error: there was a Checksum mismatch. We were expecting %u but instead we calculated %u\n", item.cksum, cksum);
            continue;
        } 
        printf("Checksum calculated for %u item(s): %u\n", NUM_ITEMS, item.cksum);
    }
    printf("All items were identified; their checksums were successfully matched!\n");
    pthread_exit(NULL);
}

void init_buffer() {
    sem_unlink("sem_mutex");
    sem_unlink("sem_empty");
    sem_unlink("sem_full");

    sem_t *sem_empty;
    sem_t *sem_full;
    sem_t *sem_mutex;

    // Create the mutex semaphore
    sem_mutex = sem_open("sem_mutex", O_CREAT, 0644, 1);
    if (sem_mutex == SEM_FAILED) {
        perror("Failed to create mutex semaphore");
        exit(EXIT_FAILURE);
    }
    
    // Create the empty semaphore
    sem_empty = sem_open("sem_empty", O_CREAT, 0644, BUFFER_SIZE);
    if (sem_empty == SEM_FAILED) {
        perror("Failed to create empty semaphore");
        exit(EXIT_FAILURE);
    }
    
    // Create the full semaphore
    sem_full = sem_open("sem_full", O_CREAT, 0644, 0);
    if (sem_full == SEM_FAILED) {
        perror("Failed to create full semaphore");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char *argv[]) {

    /* 1. Get command line arguments argv[1], argv[2], argv[3] */
    if (argc != 4) {
        printf("Usage: %s <delay> <#producer threads> <#consumer threads>\n", argv[0]);
        return 1;
    }
    int delay = atoi(argv[1]);
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int i;

    /* 2. Initialize buffer */
    init_buffer();

    /* 3. Create producer thread(s) */
    pthread_t producers[num_producers];
    for (i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }

    /* 4. Create consumer thread(s) */
    pthread_t consumers[num_consumers];
    for (i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    /* 5. Sleep */
    sleep(delay);

    /* 6. Exit */
    return 0;
}
