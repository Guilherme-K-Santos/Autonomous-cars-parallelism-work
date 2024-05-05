#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS_SENSOR 5
#define NUM_THREADS_ATUADOR 5
#define EMPTY -1

pthread_t threads_sensor[NUM_THREADS_SENSOR];
pthread_t threads_atuador[NUM_THREADS_ATUADOR];
pthread_t sensor_creator;

// feito para controlar fluxo.
int sensor_data[NUM_THREADS_SENSOR];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// ++ condição de melhora: usar cond para assim que um dado for processado e settado para EMPTY já processar outro?
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


// atuadores
void *atuador(void *arg) {
  // processa os dados do sensor aqui
  int sensor_data = *((int *)arg);
  printf("ATUADOR THREAD with sensor data numa nova thread: %d\n", sensor_data);

  pthread_exit(NULL);
}

void create_atuadores() {
  for (long i = 0; i < NUM_THREADS_ATUADOR; i++) {
    // cria thread que representa atuador
    pthread_create(&threads_atuador[i], NULL, atuador, (void *)&sensor_data[i]);
  }
}

// sensores
void *sensor(void *arg) {
  // seed para os random numeros dos sensores (baseado no id da thread e no tempo atual)
  srand(time(NULL) ^ (long)arg);

  // sleep time para pegar os dados
  int sleep_time = rand() % 6;
  sleep(sleep_time);

  // random data dos sensores
  int random_int = rand() % 1000;

  pthread_mutex_lock(&mutex);
  // salva dados desse sensor
  sensor_data[(long)arg] = random_int;
  pthread_mutex_unlock(&mutex);

  printf("Sensor (sensor SIDE THREAD) %ld gerou: %d em %d segundos\n", (long)arg, random_int, (long)sleep_time);

  pthread_exit(NULL);
}

// Esta side thread é responsável por criar sensores se algum slot de dados estão disponiveis.
// + possível melhora (usar thread condition para assim que algum dado virar EMPTY instantaneamente aviasr o sensor para mandar mais dados)?
void *create_sensors() {
  printf("create Sensor\n");

  long sensor_id;
  while (1) {
    sleep(1);

    // reseta sensor_id para o numero correto
    if (sensor_id >= NUM_THREADS_SENSOR) sensor_id = 0;

    // se ele estiver EMPTY pode prosseguir para simular dado de sensor, caso contrário ignora.
    if (sensor_data[sensor_id] != EMPTY) continue;

    printf("não pulou\n");

    // cria side thread para armazenar dado de sensor
    pthread_create(&threads_sensor[sensor_id], NULL, sensor, (void *)sensor_id);
    
    // aumenta o id do sensor.
    sensor_id++;
  }
}

// main = central de controle (devemos fzr assim?)
int main() {
  long sensor_id;

  // instancia a side thread que cria threads de sensores.
  pthread_create(&sensor_creator, NULL, create_sensors, NULL);

  while (1) {
    // simulate processing time
    sleep(1);

    // checka para resetar o sensor_id para processamento
    if (sensor_id > NUM_THREADS_SENSOR) sensor_id = 0;
    
    if (sensor_data[sensor_id] == EMPTY) continue;

    // isto só serve para printar e conferir se está tudo indo nos conformes.
    for (long id = 0; id < NUM_THREADS_ATUADOR; id++) {
      printf("(MAIN THREAD) checando array POSITION %d, VALOR: %d\n", id, sensor_data[id]);
    }

    pthread_mutex_lock(&mutex);
    // settando data já processado para EMPTY (assim um novo sensor pode aderir novo valor)
    printf("(MAIN THREAD) set POSITION %d, VALOR: %d para EMPTY\n", sensor_id, sensor_data[sensor_id]);
    sensor_data[sensor_id] = EMPTY;
    pthread_mutex_unlock(&mutex);

    // increase sensor_id para checar próximo dado de sensor.
    sensor_id++;

    // printf("Criando atuadores (threads)...\n");
    // create_atuadores();

    // espera todos os atuadores terminar de processar os dados (devemos fzr assim?)
    // for (long i = 0; i < NUM_THREADS_ATUADOR; i++) {
    //   pthread_join(threads_atuador[i], NULL);
    // }
    // printf("Atuadores (threads) finalizados.\n");
  }

  return 0;
}
