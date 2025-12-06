#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;  
void* provider(void* arg) {
    int count = *(int*)arg;
    for (int i = 0; i < count; ++i) {
        sleep(1);  
        
        pthread_mutex_lock(&lock);
        if (ready == 1) {
            pthread_mutex_unlock(&lock);
            continue;
        }
        ready = 1;
        printf("provided\n");
        pthread_cond_signal(&cond1); 
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void* consumer(void* arg) {
    int count = *(int*)arg;
    for (int i = 0; i < count; ++i) {
        pthread_mutex_lock(&lock);
        while (ready == 0) {
            pthread_cond_wait(&cond1, &lock); 
        }
        ready = 0;
        printf("consumed\n");
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    const int N = 5;
    pthread_t prov_thr, cons_thr;
    int count = N;

    pthread_create(&prov_thr, NULL, provider, &count);
    pthread_create(&cons_thr, NULL, consumer, &count);

    pthread_join(prov_thr, NULL);
    pthread_join(cons_thr, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);

    return 0;
}