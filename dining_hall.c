#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Estrutura para o Monitor
typedef struct {
    int eating;          // Número de estudantes comendo
    int ready_to_eat;    // Número de estudantes com a bandeja na mão (esperando)
    int ready_to_leave;  // Número de estudantes que acabaram de comer (esperando para sair)
    
    pthread_mutex_t lock;
    pthread_cond_t ok_to_sit;
    pthread_cond_t ok_to_leave;
} DiningHall;

DiningHall hall;

// Parâmetros de simulação
int NUM_STUDENTS = 0;

void init_hall() {
    hall.eating = 0;
    hall.ready_to_eat = 0;
    hall.ready_to_leave = 0;
    pthread_mutex_init(&hall.lock, NULL);
    pthread_cond_init(&hall.ok_to_sit, NULL);
    pthread_cond_init(&hall.ok_to_leave, NULL);
}

void getFood(int id) {
    printf("[Estudante %d] Pegando comida...\n", id);
    usleep((rand() % 500) * 1000); // Simula tempo para pegar comida
}

void dine(int id) {
    printf(" -> [Estudante %d] COMENDO. (Total na mesa: %d)\n", id, hall.eating);
    usleep((rand() % 1000) * 1000); // Simula tempo comendo
}

void leave(int id) {
    printf("    <- [Estudante %d] SAIU. (Restam na mesa: %d)\n", id, hall.eating);
}

// --- Lógica de Sincronização: ENTRADA ---
void readyToEat(int id) {
    pthread_mutex_lock(&hall.lock);
    
    hall.ready_to_eat++;
    printf("[Estudante %d] Pronto para comer. (Esperando: %d, Comendo: %d)\n", 
           id, hall.ready_to_eat, hall.eating);

    // CONDIÇÃO DE BLOQUEIO:
    // Se não há ninguém comendo E eu sou o único esperando (não tenho par), devo esperar.
    while (hall.eating == 0 && hall.ready_to_eat < 2) {
        pthread_cond_wait(&hall.ok_to_sit, &hall.lock);
    }

    // Transição de estado
    hall.ready_to_eat--;
    hall.eating++;

    // SINALIZAÇÃO:
    // Se eu sou o segundo a chegar (destravei a mesa) ou entrei num grupo,
    // devo avisar outros que podem estar esperando.
    pthread_cond_signal(&hall.ok_to_sit);
    
    // Se minha entrada fez o número de pessoas subir para 3 (estava em 2 inseguro),
    // posso ter liberado alguém que estava preso querendo sair.
    if (hall.eating > 2) {
        pthread_cond_broadcast(&hall.ok_to_leave);
    }

    pthread_mutex_unlock(&hall.lock);
}

// --- Lógica de Sincronização: SAÍDA ---
void readyToLeave(int id) {
    pthread_mutex_lock(&hall.lock);

    hall.ready_to_leave++; // Indico que terminei
    printf("[Estudante %d] Terminou. Quer sair. (Comendo: %d, Querem sair: %d)\n", 
           id, hall.eating, hall.ready_to_leave);

    // CONDIÇÃO DE BLOQUEIO:
    // Não posso sair se deixar exatamente 1 pessoa sozinha.
    // (hall.eating - 1 == 1) significa que sobrará 1.
    // A exceção (&& hall.ready_to_leave < hall.eating) trata o caso onde
    // temos 2 pessoas e AMBAS querem sair. Se ambas querem sair, o último libera o bloqueio.
    while ((hall.eating - 1 == 1) && (hall.ready_to_leave < hall.eating)) {
        printf("[Estudante %d] Bloqueado para não deixar colega sozinho.\n", id);
        pthread_cond_wait(&hall.ok_to_leave, &hall.lock);
    }

    // Transição de estado
    hall.eating--;
    hall.ready_to_leave--;

    // SINALIZAÇÃO:
    // Aviso a todos. Se éramos 2 e eu saí, o contador foi para 1, mas como
    // o outro também já está em 'ready_to_leave' (garantido pela lógica acima),
    // ele vai acordar, ver que eating agora é 1 (1-1=0, seguro), e sair também.
    pthread_cond_broadcast(&hall.ok_to_leave);
    
    // Se a mesa esvaziou, a lógica de entrada cuida disso, mas sinalizamos por segurança
    if (hall.eating == 0) {
        // Resetamos qualquer estado residual se necessário, mas aqui ok.
    }

    pthread_mutex_unlock(&hall.lock);
}

// Função da Thread
void* student_routine(void* arg) {
    long id = (long)arg;

    getFood(id);
    
    readyToEat(id); // Barreira de entrada
    
    dine(id);
    
    readyToLeave(id); // Barreira de saída
    
    leave(id);

    return NULL;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    if (argc > 1) {
        NUM_STUDENTS = atoi(argv[1]);
    } else {
        printf("Digite o numero de estudantes: ");
        scanf("%d", &NUM_STUDENTS);
    }

    if (NUM_STUDENTS < 2) {
        printf("Erro: É necessário pelo menos 2 estudantes para satisfazer as regras.\n");
        return 1;
    }

    printf("--- Iniciando Simulação Dining Hall com %d estudantes ---\n", NUM_STUDENTS);
    init_hall();

    pthread_t* threads = malloc(sizeof(pthread_t) * NUM_STUDENTS);

    for (long i = 0; i < NUM_STUDENTS; i++) {
        if (pthread_create(&threads[i], NULL, student_routine, (void*)(i + 1)) != 0) {
            perror("Falha ao criar thread");
            return 1;
        }
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    printf("--- Todos os estudantes saíram. Simulação encerrada. ---\n");

    return 0;
}
