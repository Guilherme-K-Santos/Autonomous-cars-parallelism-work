#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS_SENSOR 50
#define NUM_THREADS_ATUADOR 20
#define ARRAY_LENGTH 100

pthread_t threads_sensor[NUM_THREADS_SENSOR];
pthread_t threads_atuador[NUM_THREADS_ATUADOR];

// Define a struct para manter múltiplos argumentos
struct thread_args {
  int id;
  int activity_level;
};

// Estrutura para representar uma tarefa do atuador.
typedef struct {
  int id;             // ID do atuador
  int activity_level; // Nível de atividade a ser definido
} AtuadorTask;

// Fila para armazenar as tarefas dos atuadores.
AtuadorTask atuadores[NUM_THREADS_ATUADOR];
AtuadorTask atuador_tasks[ARRAY_LENGTH];
int atuador_task_size = 0;

// Mutex para controlar o acesso à fila de dados dos sensores.
pthread_mutex_t sensor_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex para controlar o acesso à fila de tarefas dos atuadores.
pthread_mutex_t atuador_task_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex para controlar o acesso às threads dos atuadores.
static pthread_mutex_t atuador_thread_mutex[NUM_THREADS_ATUADOR];

// Mutex para controlar o acesso aos dados dos atuadores.
static pthread_mutex_t atuador_mutex[NUM_THREADS_ATUADOR];

// Fila de dados dos sensores.
long sensor_queue[ARRAY_LENGTH];
int sensor_queue_size = 0;

// Counter and timers for tests.
int counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

time_t start_time;

// Função a ser executada por cada thread do pool de threads dos atuadores.
void *atuadorThread(void *arg) {
  int id = (intptr_t)arg;
  while (1) {
    AtuadorTask task;

    // Verifica se há tarefas psndentes na fila.
    pthread_mutex_lock(&atuador_task_mutex);
    if (atuador_task_size > 0) {
      // Remove a primeira tarefa da fila.
      task = atuador_tasks[0];

      if (task.id == id) {
        for (int i = 0; i < atuador_task_size - 1; i++) {
          atuador_tasks[i] = atuador_tasks[i + 1];
        }
        atuador_task_size--;

        pthread_mutex_unlock(&atuador_task_mutex); // Desbloqueio da fila de tarefas
      } else {
        pthread_mutex_unlock(&atuador_task_mutex); // Desbloqueio da fila de tarefas
        continue;
      }
    } else {
      pthread_mutex_unlock(&atuador_task_mutex); // Desbloqueio da fila de tarefas
      continue;
    }

    // Bloqueia o acesso à fila de tarefas dos atuadores.
    pthread_mutex_lock(&atuador_thread_mutex[id]);

    // Executa a tarefa do atuador.
    execAtuadorTask(task.id, task.activity_level);
  }

  return NULL;
}

// Função a ser executada por cada thread dos sensores.
void *sensor(void *arg) {
  while (1) {
    // Gerar número aleatório para usar como tempo de espera.
    int sleep_time = (rand() % 5) + 1;
    sleep(sleep_time);

    // Gerar dados aleatórios para o sensor.
    int random_int = rand() % 1000;

    // Bloquear o acesso à fila de sensores.
    pthread_mutex_lock(&sensor_mutex);

    // Salvar os dados deste sensor na fila.
    sensor_queue[sensor_queue_size] = random_int;
    sensor_queue_size++;

    pthread_mutex_unlock(&sensor_mutex);
  }
}

// Subtask 1 do atuador: mudar o nível de atividade do atuador.
int changeActivityLevel(int id, int activity_level) {
  // Verificar se a subtask vai falhar (20% de chance de falha).
  if ((rand() % 5) == 0) return 1; // Retorna 1 para indicar falha

  pthread_mutex_lock(&atuador_mutex[id]);
  // Mudando activity level do atuador.
  atuadores[id].activity_level = activity_level;

  sleep(2 + rand() % 2);

  // Voltando ao normal após 2 segundinhos.
  atuadores[id].activity_level = 0;

  pthread_mutex_unlock(&atuador_mutex[id]); // Desbloqueio do mutex

  return 0; // Retorna 0 para indicar sucesso
}

// Função a ser executada pela thread da subtask de mudança de nível de atividade.
void *changeActivityLevelThread(void *arg) {
  struct thread_args *args = (struct thread_args *)arg;
  int id = args->id;
  int activity_level = args->activity_level;

  return (void *)(intptr_t)changeActivityLevel(id, activity_level);
}

// Subtask 2 do atuador: enviar mensagem para o painel.
void *sendToPanelThread(void *arg) {
  struct thread_args *args = (struct thread_args *)arg;
  int id = args->id;
  int activity_level = args->activity_level;

  return (void *)(intptr_t)sendToPanel(id, activity_level);
}

// Subtask 2 do atuador: enviar mensagem para o painel.
int sendToPanel(int id, int activity_level) {
  // Verificar se a subtask vai falhar (20% de chance de falha).
  if ((rand() % 5) == 0) return 1; // Retorna 1 para indicar falha

  printf("Enviando para painel: Atuador %d com valor %d\n", id, activity_level);
  sleep(1); // Esperar um segundo antes de imprimir outra coisa.
  return 0; // Retorna 0 para indicar sucesso
}

// Função para executar a tarefa do atuador com Fork-Join.
void execAtuadorTask(int id, int activity_level) {
  pthread_t activity_change;
  int activity_change_result;

  pthread_t painel_change;
  int painel_result;

  struct thread_args args = {id, activity_level};

  // Criar uma thread para a subtask de mudança de nível de atividade.
  pthread_create(&activity_change, NULL, changeActivityLevelThread, (void *)&args);

  // Criar uma thread para a subtask de enviar mensagem para o painel.
  pthread_create(&painel_change, NULL, sendToPanelThread, (void *)&args);

  // Esperar pela conclusão da subtask de mudança de nível de atividade.
  pthread_join(activity_change, (void **)&activity_change_result);
  pthread_join(painel_change, (void **)&painel_result);

  // Se uma das subtarefas falhou, exibir uma mensagem de falha no painel.
  if (activity_change_result + painel_result != 0) {
    printf("Falha: Atuador %d\n", id);
  } else {
    pthread_mutex_lock(&counter_mutex);
    counter++;
    pthread_mutex_unlock(&counter_mutex);
  }
  // Libera o acesso à fila de tarefas do atuador.
  pthread_mutex_unlock(&atuador_thread_mutex[id]); // Liberação do mutex da thread do atuador
}

int main() {
  srand(time(NULL));
  start_time = time(NULL);

  // Inicializar o pool de threads dos atuadores.
  for (int thread = 0; thread < NUM_THREADS_ATUADOR; thread++) {
    AtuadorTask atuador = {thread, 0};
    atuadores[thread] = atuador;
    pthread_mutex_init(&atuador_thread_mutex[thread], NULL);
    pthread_mutex_init(&atuador_mutex[thread], NULL); // Inicialização dos mutexes

    pthread_create(&threads_atuador[thread], NULL, atuadorThread, (void *)(intptr_t)thread);
  }

  for (long thread = 0; thread < NUM_THREADS_SENSOR; thread++) {
    // Criar as threads eternas dos sensores (vão gerar dados enquanto a main processa eles).
    pthread_create(&threads_sensor[thread], NULL, sensor, (void *)(intptr_t)thread);
  }

  while (counter < 100) {
    // Verificar se há dados na fila de sensores.
    pthread_mutex_lock(&sensor_mutex);
    if (sensor_queue_size > 0) {
      // Decidir qual atuador pegará o dado.
      int thread_atuadora = sensor_queue[0] % NUM_THREADS_ATUADOR;
      
      // Randomizar a atividade dos atuadores.
      int random_int = rand() % 100;

      // Enviar tarefa para o atuador.
      pthread_mutex_lock(&atuador_task_mutex); // Sincronização ao adicionar tarefa na fila
      atuador_tasks[atuador_task_size].id = thread_atuadora;
      atuador_tasks[atuador_task_size].activity_level = random_int;
      atuador_task_size++;
      pthread_mutex_unlock(&atuador_task_mutex);

      // Remover o dado da fila de sensores.
      for (int position = 0; position < sensor_queue_size - 1; position++) {
        sensor_queue[position] = sensor_queue[position + 1];
      }
      sensor_queue_size--;
    }
    pthread_mutex_unlock(&sensor_mutex);
  }

  time_t end_time = time(NULL);
  double elapsed_seconds = difftime(end_time, start_time);
  printf("Program Finished. Elapsed time: %.2f seconds\n", elapsed_seconds);

  return 0;
}
