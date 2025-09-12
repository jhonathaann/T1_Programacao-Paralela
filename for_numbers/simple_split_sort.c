#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// valores entre 10-20 geralmente sao otimos
#define INSERTION_THRESHOLD 16


void insertion_sort(int *lista, int left, int right);
void merge(int *lista, int left, int mid, int right);
void splitsort(int *lista, int left, int right);
void imprime(int *lista, int size);


int main() {

    int lista[] = {64, 34, 25, 12, 22, 11, 90, 88, 76, 50, 42, 33, 21, 19, 8, 5, 3, 1, 99, 77};
    int n = sizeof(lista) / sizeof(lista[0]);
    
    printf("Vetor original:\n");
    imprime(lista, n);
    
    splitsort(lista, 0, n - 1);
    
    printf("\nVetor ordenado:\n");
    imprime(lista, n);
    
    // 
    printf("\n=== Teste com vetor random ===\n");
    int random_lista[50];
    srand(time(NULL));
    
    for (int i = 0; i < 50; i++) {
        random_lista[i] = rand() % 100;
    }
    
    printf("Vetor random original:\n");
    imprime(random_lista, 50);
    
    splitsort(random_lista, 0, 49);
    
    printf("\nVetor random ordenado:\n");
    imprime(random_lista, 50);
    
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
    int *lista_dir = (int*) malloc(n1*sizeof(int));
    int *lista_esq = (int*) malloc(n2*sizeof(int));

    // copia os dados para essas listas temporarias
    for(int i = 0; i < n1; i++){
        lista_esq[i] = lista[esq+i];
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