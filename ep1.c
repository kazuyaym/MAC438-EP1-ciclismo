/**********************************/
/* EP 1 - Programação Concorrente */
/*                                */
/* Felipe Túlio           7557709 */
/* Marcos Kazuya          7577622 */
/*                                */
/* Corrida de bicicletas estilo   */
/* miss-and-out                   */
/**********************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <math.h>

/* Struct do ciclista */
typedef struct ciclista {

	pthread_t thread_id;

	unsigned int id;				/* Número do ciclista */
	unsigned int d;					/* Posição do ciclista na pista */
	unsigned int posicao; 			/* Posição do ciclista em relacao ao outros ciclistas */
	unsigned int volta;				/* Volta do ciclista */
	unsigned int velocidade;		/* Velocidade do ciclista (considerando o modo) */
	unsigned int meioCaminhoAndado;	/* Flag para verificar quando o ciclista está no lugar "imaginário" na pista (modo velocidades aleatórias (v)) */
	unsigned int ativo;             /* Se o ciclista esta ou nao na corrida, 1 = true, 0 = false */
	struct ciclista *next;			/* Aponta para os ciclistas que estao na mesma posicao que ele */

} CICLISTA;

/* Struct para guardar q quantidade de ciclistas numa posicao, e um ponteiro indicando quais sao os ciclistas que estao ali */
typedef struct pistinha {

	unsigned int qtdCiclistas;
	CICLISTA *next;

} PISTA;

/* Argumentos recebidos na entrada */
int arg_distancia;				/* Tamanho da pista */
int arg_numCiclistas;			/* Número de ciclistas */
int arg_modo;					/* Velocidade constante (50 km/h) ou aleatória (50 km/h ou 25 km/h) == u (vel constante) = 0 & v (vel aleatoria) = 1 */
int arg_debug = 0;				/* Modo debug */

/* Variável compartilhada e seu semáforo */
PISTA* pista;   	    	    /* este eh o nosso vetor da pista, que tem valores iguais ao numero de ciclista numa posicao especifica */
sem_t mutex;	 				/* Semáforo para controlar o acesso à variável pista */

/* Variáveis para calcular as voltas */
unsigned int contVoltas;			/* Contador utilizado pra verificar quando uma volta foi completada */
unsigned int voltaCompletada;		/* Flag para verificar se o primeiro ciclista completou uma volta */
CICLISTA *ultimoColocado;			/* Ponteiro para localizar o último colocado */
CICLISTA *ciclistaRetirado;			/* Ponteiro para o ciclista a ser retirado */
CICLISTA *ouro,						/* Variáveis para armazenar os ciclistas vencedores */
		  *prata,
		  *bronze;
unsigned int debugCounter;			/* Contador para verificarb as 200 iterações para o modo debug */


/* Variáveis da barreira sincronizada */
unsigned int numCiclistasAtivos;
unsigned int arrive;
pthread_mutex_t mutexBarreira;
pthread_cond_t condicaoBarreira;

/* Seta os parâmetros recebido pela linha de comando */
void set_parametros(int argc, char *argv[]) {

	arg_distancia = atoi(argv[1]);

	arg_numCiclistas = atoi(argv[2]);

	if ( strcmp(argv[3], "u") == 0 )
		arg_modo = 0;
	else
		arg_modo = 1;

	if ( argc > 4 )
		if ( strcmp(argv[4], "d") == 0 )
			arg_debug = 1;

}

void *pedala(void *arg) {

	CICLISTA *tinfo = arg, 		/* Armazena o argumento passado pela função pthread_create */
			 *q;	           	/* ponteiro auxiliar */ 
	int proximo_d = 0;		   	/* Variável para calcular qual será a próxima posição do ciclista */
	int i;

	/* Loop onde cada iteração seria equivalente a 72ms */
	while (1) {

		/* Barreira, espera todos os processos a chegarem neste estagio antes de proseguirem */
		pthread_mutex_lock(&mutexBarreira);
		arrive--; 
		if ( arrive == 0 ) { 

			/* Caso o modo debug for selecionado, imprimirá a cada 200 iterações um log */
			if (arg_debug) {
				debugCounter++;
				if (debugCounter == 200) {
					debugCounter = 0;
					printf("\nMODO DEBUG - 14,4 segundos se passaram!\n");
					for (i = 0; i < arg_distancia; i++)
						for (q = pista[i].next; q; q = q->next)
							printf("\tO ciclista %3d está na posição %3d! Sua volta é a %3d e está no metro %3d da pista! Com velocidade: %2d\n",
									  q->id, q->posicao, q->volta, q->d, q->velocidade);
				}
			}

			/* Verifica se o primeiro ciclista completou uma volta para retirar
			   o último ciclista da corrida */
			if (voltaCompletada) {
				voltaCompletada = 0;
				contVoltas++;

				/* Retirar o último ciclista da corrida*/
				ciclistaRetirado = ultimoColocado;

				/* Retira agora da lista ligada da pista */
				if (pista[ciclistaRetirado->d].next == ciclistaRetirado)
					pista[ciclistaRetirado->d].next = ciclistaRetirado->next;
				else {
					for (q = pista[ciclistaRetirado->d].next; q->next != ciclistaRetirado; q = q->next) ;
					q->next = ciclistaRetirado->next;
				}

				/* Procura o novo último lugar */
				i = ciclistaRetirado->d;
				ultimoColocado = NULL;
				do {
					if (pista[i].next) {
						for (q = pista[i].next; q; q = q->next)
							if (ultimoColocado == NULL)
								ultimoColocado = q;
							else {
								if (q->posicao >= ultimoColocado->posicao)
									ultimoColocado = q;
							}
					}
					i++;
					if (i == arg_distancia)
						i = 0;
				} while (i != ciclistaRetirado->d);

				printf("\nVolta %3d completada! O ciclista %3d foi retirado da competição na posição %3d no metro %3d da volta %3d!\n",
						 contVoltas, ciclistaRetirado->id, ciclistaRetirado->posicao, ciclistaRetirado->d, ciclistaRetirado->volta);
				/* Decrementa o número de ciclistas */
				numCiclistasAtivos--;
				pista[ciclistaRetirado->d].qtdCiclistas--;
			}

			arrive = numCiclistasAtivos;
			pthread_cond_broadcast(&condicaoBarreira);
		}
		else
		  	pthread_cond_wait(&condicaoBarreira, &mutexBarreira); 
		pthread_mutex_unlock(&mutexBarreira);

		/* Veririca se o ciclista a ser retirado é ele mesmo */
		if (ciclistaRetirado && ciclistaRetirado == tinfo) {
			ciclistaRetirado = NULL;
			break;
		}

		/* Verifica se a corrida chegou ao fim */
		if (numCiclistasAtivos <= 3) {
			if (tinfo->posicao == 1 && ouro == NULL) {
				ouro = tinfo;
				break;
			}
			if (tinfo->posicao <= 2 && prata == NULL) {
				prata = tinfo;
				break;
			}
			if (tinfo->posicao <= 3 && bronze == NULL) {
				bronze = tinfo;
				break;
			}
		}

		/* Checa a velocidade para verificar se o ciclista possui velocidade 25 ou 50 */
		if (tinfo->velocidade == 25 && tinfo->meioCaminhoAndado == 0)
			tinfo->meioCaminhoAndado = 1;
		/* Anda uma posição se ciclista possui essa possibilidade (v == 50 ou meioCaminhoAndado == 1) */
		else {

			/* Zera maio caminha andado */
			tinfo->meioCaminhoAndado = 0;

			/* Checa a próxima posição do ciclista */
			if (tinfo->d + 1 == arg_distancia)
				proximo_d = 0;
			else
				proximo_d = tinfo->d + 1;

			/* Muda semáforo para acessar a variável compartilhada */
			sem_wait(&mutex);

			if (pista[proximo_d].qtdCiclistas < 4) {

				/* Alterações na lista do vetor pista */
				if( pista[tinfo->d].next == tinfo ) {
					pista[tinfo->d].next = tinfo->next;
					tinfo->next = pista[proximo_d].next;
					pista[proximo_d].next = tinfo;
				}
				else {
					for(q = pista[tinfo->d].next; q->next != tinfo; q = q->next);
					q->next = tinfo->next;
					tinfo->next = pista[proximo_d].next;
					pista[proximo_d].next = tinfo;
				}

				if (proximo_d == 0) /* Checa se o ciclista completou uma volta */
					tinfo->volta++;

				/* Checa se há algum ciclista para igualar a posição */
				for (q = tinfo->next; q; q = q->next)
					if (q->volta == tinfo->volta && q->posicao <= tinfo->posicao)
						tinfo->posicao = q->posicao;

				/* Aumenta a posição caso haja algum ciclista na posão antiga */
				for(q = pista[tinfo->d].next; q; q = q->next) {
					if (proximo_d == 0) {
						if (tinfo->volta == q->volta + 1 && q->posicao <= tinfo->posicao)
							q->posicao++;
					}
					else {
						if (tinfo->volta == q->volta && q->posicao <= tinfo->posicao)
							q->posicao++;
					}
				}

				/* Arruma a qtd de ciclistas na posição */
				pista[tinfo->d].qtdCiclistas--;
				pista[proximo_d].qtdCiclistas++;

				/* Verifica se o ciclista ainda é o último colocado, senão, transfere o valor */
				if (tinfo == ultimoColocado) {
					for (q = pista[tinfo->d].next; q; q = q->next)
						if (q->posicao > tinfo->posicao)
							ultimoColocado = q;
				}

				tinfo->d = proximo_d;

				/* Checa se o primeiro colocado completou uma volta */
				if (tinfo->d == 0 && tinfo->posicao == 1 && tinfo->volta)
					voltaCompletada++;

			}

			/* Muda o semáforo para deixar que outras threads utilizem a variável compartilhada */
			sem_post(&mutex);

			/* Seta algumas variáveis caso o ciclista complete uma volta*/
			if (tinfo->d == 0 && tinfo->volta > 0) {
				/* Se o modo é velocidade aleatória, a cada volta tem a chance de mudar de velocidade */
				if (arg_modo == 1) {
					if(rand()%2 == 0) tinfo->velocidade = 50;
        			else tinfo->velocidade = 25;
				}
			}		
		}

	}
	return NULL;
}

int main(int argc, char *argv[]) {
	CICLISTA *ciclista_info, *q;
	int i, /* iterador */ 
		j, /* tamanho do espaço ocupado pelos ciclistas no incio da corrida */
		k, /* posicao aleatoria dentro de j */
		l, /* posicao real no vetor (que no nosso algoritmo todos os ciclistas
		      começam na volta -1 antes da largada) */
		w; /* iteracao para arrumar posicoes dos ciclistas */

	set_parametros(argc, argv);

	/* Aloca os vetores da pista e da struct dos ciclistas (ptheads) */
	pista = calloc(arg_distancia, sizeof(PISTA));
	if (!pista) {
		printf("Não foi possível alocar memória!\n");
		return EXIT_FAILURE;
	}
	for(i = 0; i < arg_distancia; i++) pista[i].next = NULL;

	ciclista_info = calloc(arg_numCiclistas, sizeof(CICLISTA));
	if (!ciclista_info) {
		printf("Não foi possível alocar memória!\n");
		return EXIT_FAILURE;
	}

	/* Inicializa o semáforo */
	if (sem_init(&mutex,0,1)) {
      printf("Erro ao criar o semáforo!\n");
      return EXIT_FAILURE;
	}

	/* Inicializando mutex e condicao da barreira */
	pthread_mutex_init(&mutexBarreira, NULL);
  	pthread_cond_init(&condicaoBarreira, NULL);

	/* Seta variável de ciclistas ativos */
	numCiclistasAtivos = arg_numCiclistas;
	arrive = numCiclistasAtivos;

	/* Inicializa variável */
	contVoltas = 0;
	voltaCompletada = 0;
	ultimoColocado = NULL;
	ciclistaRetirado = NULL;
	ouro = prata = bronze = NULL;
	debugCounter = 0;

	/* set srand para pegar num aleatorios */
	srand (time(NULL));

	/* cria cada ciclista/pthreads */
	for(i = 0; i < arg_numCiclistas; i++) {
		ciclista_info[i].id = i + 1;

		/* Insere ciclistas de forma aleatória */
		do {
			j = arg_numCiclistas/4 + 1;
			k = rand() % j;
			l = k + arg_distancia - j;
		} while(pista[l].qtdCiclistas == 4);
		pista[l].qtdCiclistas++;
		ciclista_info[i].d = l;

		ciclista_info[i].next = pista[l].next;
		pista[l].next = &ciclista_info[i];

		ciclista_info[i].posicao = 0;

		/* verifica se tem alguem na minha posicao, se nao procura o primeiro a frente dele */
		if(ciclista_info[i].next) ciclista_info[i].posicao = ciclista_info[i].next->posicao;
		else {
			/* procuras o primeiro ciclista a frente, se nao tiver, conta quantos estao nesta posicao, se nao, ele é o primeiro*/
			for(w = l + 1; w < arg_distancia; w++) { 
				if(pista[w].next) { 
					ciclista_info[i].posicao = pista[w].next->posicao + pista[w].qtdCiclistas;
					break;
				}
			}
			/* se nao tem ninguem a frente dele na pista, ele é o primeiro*/
			if(ciclista_info[i].posicao == 0) ciclista_info[i].posicao = 1;
		}
		/* Para cada ciclista que estao atras dele, aumenta uma posicao */
		for(w = l - 1; w >= j; w--) 
			if(pista[w].next) 
				for(q = pista[w].next; q != NULL; q = q->next) 
					q->posicao++;

		if (ultimoColocado == NULL || ciclista_info[i].posicao > ultimoColocado->posicao)
			ultimoColocado = &ciclista_info[i];

		ciclista_info[i].volta = -1;

		if (!arg_modo) ciclista_info[i].velocidade = 50;
		else ciclista_info[i].velocidade = 25;

		ciclista_info[i].meioCaminhoAndado = 0;

		ciclista_info[i].ativo = 1;

		if ( pthread_create(&ciclista_info[i].thread_id, NULL, &pedala, &ciclista_info[i]) ) {
			printf("Não foi possível criar a thread %d \n", ciclista_info[i].id);
			return EXIT_FAILURE;
		}
	}

	/* Espera todos os processos chegarem a esse ponto */
	for (i = 0; i < arg_numCiclistas; ++i) {
        if (ciclista_info[i].ativo) {
        	if ( pthread_join(ciclista_info[i].thread_id, NULL) ) {
         		printf("Não foi possível dar join na thread %d \n", ciclista_info[i].id);
				return EXIT_FAILURE;
			}
       	}
    }

    /* Imprime as posições finais */
    printf("\n\n\tFIM DE CORRIDA!\n");
	printf("O primeiro colocado foi o ciclista: %d!\n", ouro->id);
    printf("O  segundo colocado foi o ciclista: %d!\n", prata->id);
    printf("O terceiro colocado foi o ciclista: %d!\n", bronze->id);

	/* libera memoria */
	free(pista);
 	free(ciclista_info);

	return 0;
}