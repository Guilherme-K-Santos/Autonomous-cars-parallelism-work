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

// Tabela de atuadores.
int actuators_array[NUM_THREADS_ATUADOR] = {0};

// Estrutura para representar uma tarefa do atuador.
typedef struct
{
  int id;             // ID do atuador
  int activity_level; // Nível de atividade a ser definido
} AtuadorTask;

// Fila para armazenar as tarefas dos atuadores.
AtuadorTask atuador_tasks[ARRAY_LENGTH];
int atuador_task_size = 0;

// Mutexes
pthread_mutex_t actuators_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sensor_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex para controlar o acesso à fila de tarefas dos atuadores.
pthread_mutex_t atuador_task_mutex = PTHREAD_MUTEX_INITIALIZER;

// Feito para controlar fluxo.
long sensor_queue[ARRAY_LENGTH];
int sensor_queue_size = 0;

// Função a ser executada por cada thread do pool de threads dos atuadores.
void *atuadorThread(void *arg)
{
  while (1)
  {
    AtuadorTask task;

    // Bloqueia o acesso à fila de tarefas dos atuadores.
    pthread_mutex_lock(&atuador_task_mutex);

    // Verifica se há tarefas pendentes na fila.
    if (atuador_task_size > 0)
    {
      // Remove a primeira tarefa da fila.
      task = atuador_tasks[0];
      for (int i = 0; i < atuador_task_size - 1; i++)
      {
        atuador_tasks[i] = atuador_tasks[i + 1];
      }
      atuador_task_size--;
    }
    else
    {
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
void *sensor(void *arg)
{
  while (1)
  {
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
int changeActivityLevel(int id, int activity_level)
{
  // Verificar se a subtask vai falhar (20% de chance de falha).
  if (rand() % 5 == 0)
  {
    printf("Falha: Atuador %d\n", id);
    return 0; // Retorna 0 para indicar falha
  }
  else
  {
    // Bloquear o acesso à tabela de atuadores.
    pthread_mutex_lock(&actuators_mutex);
    actuators_array[id] = activity_level;
    pthread_mutex_unlock(&actuators_mutex);
    return 1; // Retorna 1 para indicar sucesso
  }
}

// Subtask 2 do atuador: enviar mensagem para o painel.
void sendToPanel(int id, int activity_level)
{
  // Verificar se a subtask vai falhar (20% de chance de falha).
  if (rand() % 5 != 0)
  {
    printf("Alterando: Atuador %d com valor %d\n", id, activity_level);
    sleep(1); // Esperar um segundo antes de imprimir outra coisa.
  }
}

// Função a ser executada pela thread da subtask de mudança de nível de atividade.
void *changeActivityLevelThread(void *arg)
{
  int id = *((int *)arg);
  int activity_level = actuators_array[id];

  return (void *)(long)changeActivityLevel(id, activity_level);
}

// Função para executar a tarefa do atuador com Fork-Join.
void execAtuadorTask(int id, int activity_level)
{
  int result_change = 0;
  pthread_t thread_change;

  // Criar uma thread para a subtask de mudança de nível de atividade.
  pthread_create(&thread_change, NULL, changeActivityLevelThread, (void *)&id);

  // Executar a subtask de enviar mudança para o painel.
  sendToPanel(id, activity_level);

  // Esperar pela conclusão da subtask de mudança de nível de atividade.
  pthread_join(thread_change, (void **)&result_change);

  // Se uma das subtarefas falhou, exibir uma mensagem de falha no painel.
  if (result_change == 0)
  {
    printf("Falha: Atuador %d\n", id);
  }
}

int main()
{
  // Inicializar o pool de threads dos atuadores.
  for (int i = 0; i < NUM_THREADS_ATUADOR; i++) {
    pthread_create(&threads_atuador[i], NULL, atuadorThread, NULL);
  }

  for (long thread = 0; thread < NUM_THREADS_SENSOR; thread++) {
    // Criar as threads eternas dos sensores (vão gerar dados enquanto a main processa eles).
    pthread_create(&threads_sensor[thread], NULL, sensor, (void *)thread);
  }

  while (1)
  {
    // Verificar se há dados na fila de sensores.
    if (sensor_queue_size > 0)
    {
      // Decidir qual atuador pegará o dado.
      int thread_atuadora = sensor_queue[0] % NUM_THREADS_ATUADOR;

      // Randomizar a atividade dos atuadores.
      int random_int = rand() % 100;
      // printf("thread_atuadora: %d\n", thread_atuadora);

      // Atualizar a tabela de atuadores com os níveis de atividade.
      actuators_array[thread_atuadora] = random_int;

      // Enviar tarefa para o atuador.
      pthread_mutex_lock(&atuador_task_mutex);
      atuador_tasks[atuador_task_size].id = thread_atuadora;
      atuador_tasks[atuador_task_size].activity_level = random_int;
      atuador_task_size++;
      pthread_mutex_unlock(&atuador_task_mutex);

      // Remover o dado da fila de sensores.
      pthread_mutex_lock(&sensor_mutex);
      printf("START PRINTING\n");
      for (int position = 0; position < sensor_queue_size - 1; position++) {
        sensor_queue[position] = sensor_queue[position + 1];
        printf("sensor_queue[%d]: %ld\n", position, sensor_queue[position]);
      }
      printf("FINISH PRINTING\n");
      sensor_queue_size--;
      pthread_mutex_unlock(&sensor_mutex);
    }
    else
    {
      // Cooldown para caso não haja dados não precisar processar desnecessariamente.
      sleep(1);
      printf("Array is empty, nothing to remove.\n");
    }
  }

  return 0;
}
