#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS_SENSOR 20
#define NUM_THREADS_ATUADOR 10
#define ARRAY_LENGTH 100

pthread_t threads_sensor[NUM_THREADS_SENSOR];
pthread_t threads_atuador[NUM_THREADS_ATUADOR];

// Define a struct to hold multiple arguments
struct thread_args {
  int id;
  int activity_level;
};

// Estrutura para representar uma tarefa do atuador.
typedef struct {
  int id; // ID do atuador
  int activity_level; // Nível de atividade a ser definido
} AtuadorTask;

// Fila para armazenar as tarefas dos atuadores.
AtuadorTask atuador_tasks[ARRAY_LENGTH];
int atuador_task_size = 0;

// Mutexe para controlar o acesso à fila de dados dos sensores.
pthread_mutex_t sensor_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex para controlar o acesso à fila de tarefas dos atuadores.
pthread_mutex_t atuador_task_mutex = PTHREAD_MUTEX_INITIALIZER;

// Feito para controlar fluxo.
long sensor_queue[ARRAY_LENGTH];
int sensor_queue_size = 0;

// Função a ser executada por cada thread do pool de threads dos atuadores.
void *atuadorThread(void *arg) {
  while (1) {
    AtuadorTask task;

    // Bloqueia o acesso à fila de tarefas dos atuadores.
    pthread_mutex_lock(&atuador_task_mutex);

    // Verifica se há tarefas pendentes na fila.
    if (atuador_task_size > 0) {
      // Remove a primeira tarefa da fila.
      task = atuador_tasks[0];

      if (task.id == arg) {
        for (int i = 0; i < atuador_task_size - 1; i++) {
          atuador_tasks[i] = atuador_tasks[i + 1];
        }
        atuador_task_size--;

        // printf("atuador_task_size: %d\n", atuador_task_size);  
      } else {
        // Se não houver tarefas pendentes, libera o acesso à fila e dorme até que uma nova tarefa seja adicionada.
        pthread_mutex_unlock(&atuador_task_mutex);
        sleep(1);
        continue;
      }      
    } else {
      // Se não houver tarefas pendentes, libera o acesso à fila e dorme até que uma nova tarefa seja adicionada.
      pthread_mutex_unlock(&atuador_task_mutex);
      sleep(1);
      continue;
    }

    // Libera o acesso à fila de tarefas dos atuadores.
    pthread_mutex_unlock(&atuador_task_mutex);

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
  if ((rand() % 5) == 0) return 1; // Retorna 0 para indicar falha

  // printf("esperando atuador alterar a atividade na thread: %d\n", id);
  sleep(2 + rand() % 2);
  // printf("ALTERADO na thread: %d\n", id);
  return 0; // Retorna 0 para indicar sucesso
}

// Função a ser executada pela thread da subtask de mudança de nível de atividade.
void *changeActivityLevelThread(void *arg) {
  // Cast the argument back to the correct type
  struct thread_args *args = (struct thread_args *)arg;
  int id = args->id;
  int activity_level = args->activity_level;

  return (void *)(int)changeActivityLevel(id, activity_level);
}

// Subtask 2 do atuador: enviar mensagem para o painel.
void *sendToPanelThread(void*arg) {
  // Cast the argument back to the correct type
  struct thread_args *args = (struct thread_args *)arg;
  int id = args->id;
  int activity_level = args->activity_level;

  return (void *)(int)sendToPanel(id, activity_level);
}

// Subtask 2 do atuador: enviar mensagem para o painel.
int sendToPanel(int id, int activity_level) {
  // Verificar se a subtask vai falhar (20% de chance de falha).
  if ((rand() % 5) == 0) return 1; // Retorna 1 para indicar falha

  printf("Alterando: Atuador %d com valor %d\n", id, activity_level);
  sleep(1); // Esperar um segundo antes de imprimir outra coisa.
  return 0; // Retorna 0 para indicar sucesso
}

// Função para executar a tarefa do atuador com Fork-Join.
void execAtuadorTask(int id, int activity_level) {
  pthread_t activity_change;
  int activity_change_result;

  pthread_t painel_change;
  int painel_result;

  // Create a struct to hold the arguments
  struct thread_args args = {id, activity_level};

  // Criar uma thread para a subtask de mudança de nível de atividade.
  pthread_create(&activity_change, NULL, changeActivityLevelThread, (void *)&args);

  // Criar uma thread para a subtask de printar no painel.
  pthread_create(&painel_change, NULL, sendToPanelThread, (void *)&args); 

  // printf("esperando subtasks terminarem na thread: %d\n", id);

  // Esperar pela conclusão da subtask de mudança de nível de atividade.
  pthread_join(activity_change, (void **)&activity_change_result);
  pthread_join(painel_change, (void **)&painel_result);

  // printf("JOIN TERMINADO na thread: %d\n", id);

  // printf("subtask PAINEL result: %d\n", painel_result);
  // printf("subtasks CHANGE ACTIVITY result: %d\n", activity_change_result);
  // printf("LAST RESULT: %d\n", activity_change_result + painel_result);

  // Se uma das subtarefas falhou, exibir uma mensagem de falha no painel.
  if (activity_change_result + painel_result != 0) {
    printf("Falha: Atuador %d\n", id);
  }
}

int main() {
  // Inicializar o pool de threads dos atuadores.
  for (int thread = 0; thread < NUM_THREADS_ATUADOR; thread++) {
    pthread_create(&threads_atuador[thread], NULL, atuadorThread, (void *)thread);
  }

  for (long thread = 0; thread < NUM_THREADS_SENSOR; thread++) {
    // Criar as threads eternas dos sensores (vão gerar dados enquanto a main processa eles).
    pthread_create(&threads_sensor[thread], NULL, sensor, (void *)thread);
  }

  while (1) {
    // Verificar se há dados na fila de sensores.
    if (sensor_queue_size > 0) {
      // Decidir qual atuador pegará o dado.
      int thread_atuadora = sensor_queue[0] % NUM_THREADS_ATUADOR;

      // Randomizar a atividade dos atuadores.
      int random_int = rand() % 100;
      // printf("thread_atuadora: %d\n", thread_atuadora);

      // Enviar tarefa para o atuador.
      pthread_mutex_lock(&atuador_task_mutex);
      atuador_tasks[atuador_task_size].id = thread_atuadora;
      atuador_tasks[atuador_task_size].activity_level = random_int;
      atuador_task_size++;

      // printf("START PRINTING\n");
      // for (int i = 0; i < atuador_task_size - 1; i++) {
      // printf("atuador_tasks[%d]: id: %d // activity: %d \n", i, atuador_tasks[i].id, atuador_tasks[i].activity_level);
      // }

      // printf("FINISH PRINTING\n");
      pthread_mutex_unlock(&atuador_task_mutex);

      // Remover o dado da fila de sensores.
      pthread_mutex_lock(&sensor_mutex);
      // printf("START PRINTING\n");
      for (int position = 0; position < sensor_queue_size - 1; position++) {
        sensor_queue[position] = sensor_queue[position + 1];
        // printf("sensor_queue[%d]: %ld\n", position, sensor_queue[position]);
      }
      // printf("FINISH PRINTING\n");
      sensor_queue_size--;
      pthread_mutex_unlock(&sensor_mutex);
    } else {
      // Cooldown para caso não haja dados não precisar processar desnecessariamente.
      sleep(1);
      // printf("Array is empty, nothing to remove.\n");
    }
  }

  return 0;
}
