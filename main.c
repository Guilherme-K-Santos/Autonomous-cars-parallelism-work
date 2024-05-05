#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS_SENSOR 5
#define NUM_THREADS_ATUADOR 5

int sensor_data[NUM_THREADS_SENSOR];
pthread_t threads_sensor[NUM_THREADS_SENSOR];
pthread_t threads_atuador[NUM_THREADS_ATUADOR];

void *sensor(void *arg) {
  // seed para os random numeros dos sensores (baseado no id da thread e no tempo atual)
  srand(time(NULL) ^ (long)arg);

  // sleep time para pegar os dados
  int sleep_time = rand() % 6;
  sleep(sleep_time);

  // random data dos sensores
  int random_int = rand() % 1000;

  // salva dados desse sensor
  sensor_data[(long)arg] = random_int;

  printf("Sensor (thread) %ld gerou: %d em %d segundos\n", (long)arg, random_int, (long)sleep_time);

  pthread_exit(NULL);
}

void *atuador(void *arg) {
  // processa os dados do sensor aqui
  int sensor_data = *((int *)arg);
  printf("ATUADOR THREAD with sensor data numa nova thread: %d\n", sensor_data);

  pthread_exit(NULL);
}

void create_sensors() {
  for (long i = 0; i < NUM_THREADS_SENSOR; i++) {
    // cria thread que representa sensor
    pthread_create(&threads_sensor[i], NULL, sensor, (void *)i);
  }
}

void create_atuadores() {
  for (long i = 0; i < NUM_THREADS_ATUADOR; i++) {
    // cria thread que representa atuador
    pthread_create(&threads_atuador[i], NULL, atuador, (void *)&sensor_data[i]);
  }
}

// main = central de controle (devemos fzr assim?)
int main() {
  while (1) {
    // simula dados sensorias com múltiplas threads
    printf("Criando sensores (threads)...\n");
    create_sensors();

    // espera todos os sensores terminar de captar (devemos fzr assim?)
    for (long i = 0; i < NUM_THREADS_SENSOR; i++) {
      pthread_join(threads_sensor[i], NULL);
    }
    printf("Sensores (threads) finalizadas.\n");

    // pega dados de sensor da variável global e imprimir
    printf("Dados dos sensores:\n");
    for (long i = 0; i < NUM_THREADS_SENSOR; i++) {
      printf("Sensor %ld: %d\n", i, sensor_data[i]);
    }

    printf("Criando atuadores (threads)...\n");
    create_atuadores();

    // espera todos os atuadores terminar de processar os dados (devemos fzr assim?)
    for (long i = 0; i < NUM_THREADS_ATUADOR; i++) {
      pthread_join(threads_atuador[i], NULL);
    }
    printf("Atuadores (threads) finalizados.\n");
  }

  return 0;
}
