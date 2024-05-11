#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS_SENSOR 5
#define NUM_THREADS_ATUADOR 5
#define ARRAY_LENGTH 1000

pthread_t threads_sensor[NUM_THREADS_SENSOR];
pthread_t threads_atuador[NUM_THREADS_ATUADOR];

// Tabela de atuadores
int actuators_array[NUM_THREADS_ATUADOR] = {0};

// feito para controlar fluxo.
long sensor_queue[ARRAY_LENGTH];
int sensor_queue_size = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// atuadores
void *atuador(void *arg) {
  while (1) {
    sleep(2);
    // printf("simulating\n");
  }
}

// sensores
void *sensor(void *arg) {
  while (1) {
    // Gerar numero randomico para usar de sleep.
    int sleep_time = (rand() % 5) + 1;

    // printf("thread %ld sleeping for %d\n", (long)arg, sleep_time);

    sleep(sleep_time);

    // random data dos sensores.
    unsigned int seed = rand();
    int  random_int = rand_r(&seed) % 1000;

    // Mutex para sincronizar.
    pthread_mutex_lock(&mutex);

    // printf("thread %ld generated %d\n", (long)arg, random_int);

    // salva dados desse sensor
    sensor_queue[sensor_queue_size] = random_int;
    sensor_queue_size++;

    pthread_mutex_unlock(&mutex);
  }
}

// main = central de controle.
int main() {
  for (long thread = 0; thread < NUM_THREADS_SENSOR; thread++) {
    // cria as threads eternas de sensores (vão gerar dados enquanto a main processa eles).
    pthread_create(&threads_sensor[thread], NULL, sensor, (void *)thread);
  }

  for (long thread = 0; thread < NUM_THREADS_ATUADOR; thread++) {
    // cria as threads eternas de atuadores (vão processar os dados que a main os passar).
    pthread_create(&threads_atuador[thread], NULL, atuador, (void *)thread);
  }

  while (1) {
    // simulate processing time
    sleep(1);

    // Checking if there are elements in the queue
    if (sensor_queue_size > 0) {
      // decida qual atuador pegará o dado.
      int thread_atuadora = sensor_queue[0] % NUM_THREADS_ATUADOR;

      // random activity dos atuadores.
      unsigned int seed = rand();
      int  random_int = rand_r(&seed) % 100;
      printf("thread_atuadora: %d\n", thread_atuadora);

      // Update the actuators struct with thread IDs and activity levels
      actuators_array[thread_atuadora] = random_int;

      for (int thread = 0; thread < NUM_THREADS_ATUADOR; thread++) {
        printf("Actuator[%d]: activity_level=%d\n", thread, actuators_array[thread]);
      }

      // more things...

      // remove o primeiro elemento da esquerda e shifta os outros.
      for (int position = 0; position < sensor_queue_size - 1; position++) {
        // Mutex para sincronizar
        pthread_mutex_lock(&mutex);
        sensor_queue[position] = sensor_queue[(position + 1)];
        pthread_mutex_unlock(&mutex);
      }

      // loop para printar.
      printf("START\n");
      for (int position = 0; position < sensor_queue_size - 1; position++) {
        printf("sensor_queue[%d]: %ld \n", position, sensor_queue[position]);
      }
      printf("FINISH\n");

      // decrementa o tamanho para salvar-mos a posição atual do array.
      sensor_queue_size--;
    } else {
      printf("Array is empty, nothing to remove.\n");
    }

  }

  return 0;
}
