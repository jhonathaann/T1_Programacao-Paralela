#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// valores entre 10-20 geralmente sao otimos
#define INSERTION_THRESHOLD 16
#define NUM_BALDES 10  // Número de baldes para o bucket sort

// Protótipos das funções
void insertion_sort(int *lista, int left, int right);
void merge(int *lista, int left, int mid, int right);
void splitsort(int *lista, int left, int right);
void imprime(int *lista, int size);
void bucket_sort(int *arr, int n, int num_baldes);

// Estrutura para representar um balde
typedef struct {
    int *elementos;
    int tamanho;
    int capacidade;
} Balde;

int main() {
    // Teste com o bucket sort
    printf("=== TESTE COM BUCKET SORT ===\n");
    
    // Teste com vetor maior
    printf("\n=== teste com vetor maior ===\n");
    int random_lista[30];
    srand(time(NULL));
    
    for (int i = 0; i < 30; i++) {
        random_lista[i] = rand() % 100;
    }
    
    printf("Vetor random original:\n");
    imprime(random_lista, 30);
    
    bucket_sort(random_lista, 30, NUM_BALDES);
    
    printf("\nVetor random ordenado com bucket sort:\n");
    imprime(random_lista, 30);
    
    return 0;
}

// insertionsort para subvetores pequenos
void insertion_sort(int *lista, int esq, int dir) {
    for (int i = esq + 1; i <= dir; i++) {
        int key = lista[i];
        int j = i - 1;
        
        while (j >= esq && lista[j] > key) {
            lista[j + 1] = lista[j];
            j--;
        }
        lista[j + 1] = key;
    }
}

// merge dos dois vetores ordenados
void merge(int *lista, int esq, int meio, int dir){
    int n1 = meio - esq + 1;
    int n2 = dir - meio;

    // cria vetores temporarios
    int *lista_esq = (int*) malloc(n1 * sizeof(int));
    int *lista_dir = (int*) malloc(n2 * sizeof(int));

    // copia os dados para essas listas temporarias
    for(int i = 0; i < n1; i++){
        lista_esq[i] = lista[esq + i];
    }
    for(int i = 0; i < n2; i++){
        lista_dir[i] = lista[meio + 1 + i];
    }

    // merge dos vetores temporarios de volta para o vetor original
    int i = 0, j = 0, k = esq;

    while(i < n1 && j < n2){
        if(lista_esq[i] <= lista_dir[j]){
            lista[k] = lista_esq[i];
            i++;
        }else{
            lista[k] = lista_dir[j];
            j++;
        }
        k++;
    }

    // copia os elementos restantes da parte esquerda, se houver
    while (i < n1) {
        lista[k] = lista_esq[i];
        i++;
        k++;
    }
    
    // copia os elementos restantes da parte direita, se houver
    while (j < n2) {
        lista[k] = lista_dir[j];
        j++;
        k++;
    }
    
    free(lista_esq);
    free(lista_dir);
}

// funcao principal do split sort
void splitsort(int *lista, int left, int right) {
    // se o subvetor for pequeno, usa o insertion
    if (right - left + 1 <= INSERTION_THRESHOLD) {
        insertion_sort(lista, left, right);
    } else {
        // divide recursivamente
        int mid = left + (right - left) / 2;
        
        // ordena a 1° e a 2° metade
        splitsort(lista, left, mid);
        splitsort(lista, mid + 1, right);
        
        // faz o merge das partes ordenadas
        merge(lista, left, mid, right);
    }
}

void imprime(int *lista, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", lista[i]);
    }
    printf("\n");
}

// Função para criar um balde
Balde* criar_balde() {
    Balde *b = (Balde*) malloc(sizeof(Balde));
    b->capacidade = 10;
    b->tamanho = 0;
    b->elementos = (int*) malloc(b->capacidade * sizeof(int));
    return b;
}

// add um elemento a um balde
void adicionar_ao_balde(Balde *b, int elemento) {
    if (b->tamanho == b->capacidade) {
        b->capacidade *= 2;
        b->elementos = (int*) realloc(b->elementos, b->capacidade * sizeof(int));
    }
    b->elementos[b->tamanho++] = elemento;
}

void liberar_balde(Balde *b) {
    free(b->elementos);
    free(b);
}


void bucket_sort(int *arr, int n, int num_baldes) {
    if (n <= 0) return;
    
    // encontra o max e min (intervalo)
    int min_val = arr[0];
    int max_val = arr[0];
    
    for (int i = 1; i < n; i++) {
        if (arr[i] < min_val) min_val = arr[i];
        if (arr[i] > max_val) max_val = arr[i];
    }
    
    // calcula o intervalo de cada balde
    double intervalo = (double)(max_val - min_val + 1) / num_baldes;
    
    // cria os baldes
    Balde **baldes = (Balde**) malloc(num_baldes * sizeof(Balde*));
    for (int i = 0; i < num_baldes; i++) {
        baldes[i] = criar_balde();
    }
    
    // distribui os elementos no baldes
    for (int i = 0; i < n; i++) {
        int index = (int)((arr[i] - min_val) / intervalo);
        if (index == num_baldes) index--; // 
        adicionar_ao_balde(baldes[index], arr[i]);
    }
    
    // ordena cada balde usando o splitsort
    int index = 0;
    for (int i = 0; i < num_baldes; i++) {
        if (baldes[i]->tamanho > 0) {

            splitsort(baldes[i]->elementos, 0, baldes[i]->tamanho - 1);
            
            // copia os valores para o dado original
            for (int j = 0; j < baldes[i]->tamanho; j++) {
                arr[index++] = baldes[i]->elementos[j];
            }
        }
        liberar_balde(baldes[i]);
    }
    
    free(baldes);
}