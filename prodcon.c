#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 30
#define NUM_ITEMS 100

typedef struct {
    uint16_t cksum;
    uint8_t buffer[BUFFER_SIZE];
} buffer_item;

buffer_item buffer[BUFFER_SIZE];
sem_t empty, full;
pthread_mutex_t mutex;
int in = 0, out = 0, counter = 0;

int insert_item(buffer_item item) {
    if (counter < BUFFER_SIZE) {
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        counter++;
        return 0;
    } else {
        return -1;
    }
}

int remove_item(buffer_item *item) {
    if (counter > 0) {
        *item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        counter--;
        return 0;
    } else {
        return -1;
    }
}

void *producer(void *arg) {
    int i;
    buffer_item item;
    for (i = 0; i < NUM_ITEMS; i++) {
        /* produce an item in next.produced */
        item.cksum = rand() % 65536;
        int j;
        for (j = 0; j < BUFFER_SIZE; j++) {
            item.buffer[j] = rand() % 256;
        }
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
        /* add next.produced to the buffer */
        insert_item(item);
        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    int i;
    buffer_item item;
    for (i = 0; i < NUM_ITEMS; i++) {
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        /* remove an item from buffer to next.consumed */
        remove_item(&item);
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        /* consume the item in next.consumed */
        uint16_t cksum = 0;
        int j;
        for (j = 0; j < BUFFER_SIZE; j++) {
            cksum += item.buffer[j];
        }
        if (item.cksum != cksum) {
            printf("Error: Checksum mismatch, expected %u but calculated %u\n", item.cksum, cksum);
            exit(1);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <delay> <#producer threads> <#consumer threads>\n", argv[0]);
        return 1;
    }
    int delay = atoi(argv[1]);
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);
    int i;
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);
    pthread_mutex_init(&mutex, NULL);
    pthread_t producers[num_producers];
    pthread_t consumers[num_consumers];
    for (i = 0; i < num_producers; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }
    for (i = 0; i < num_consumers; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }
    sleep(delay);
    return 0;
}
