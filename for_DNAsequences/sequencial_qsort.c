#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SEQ_LENGTH 1000000
#define DNA_CHARS "ACGT"

// gera sequencias de modo aleatorio
void generate_dna_sequence(char* seq, int length) {
    for (int i = 0; i < length; i++) {
        seq[i] = DNA_CHARS[rand() % 4];
    }
    seq[length] = '\0';
}

// realiza a comparacao entre os chars (A, C, G e T)
int compare_dna(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// realiza a ordenacao usando o qsort sequencial
void sequential_sort(char** data, int n) {
    qsort(data, n, sizeof(char*), compare_dna);
}

// função para salvar os resultados no arquivo
void save_results_to_file(int n, int length, double time_taken) {
    FILE *file = fopen("/home/jhonathan/T1_ProgP/for_DNAsequences/resultados_ordenacao.txt", "a");
    if (file == NULL) {
        printf("erro ao abrir o arquivo para salvar resultados!\n");
        return;
    }
    
    fprintf(file, "sequencias: %d, tamanho: %d, tempo: %.6f segundos\n", 
            n, length, time_taken);
    fclose(file);
    
    printf("Resultados salvos em 'resultados_ordenacao.txt'\n");
}

int main() {
    char** sequences;
    int n; // quant de sequencias
    int length; // tamanho de cada sequencia
    clock_t start, end;
    double cpu_time_used;
    
    // inicializa a semente de numeros aleatorios
    srand(time(NULL));
    

    printf("informe o numero de sequencias de DNA: ");
    scanf("%d", &n);
    
    printf("informe o tamanho de cada sequencia: ");
    scanf("%d", &length);
    
    if (length > MAX_SEQ_LENGTH) {
        printf("Erro: O tamanho maximo permitido eh: %d\n", MAX_SEQ_LENGTH);
        return 1;
    }
   
    sequences = (char**)malloc(n * sizeof(char*));
    if (sequences == NULL) {
        printf("Erro na alocacao de memoria!\n");
        return 1;
    }
    

    printf("\nGerando %d sequencias de DNA com tamanho %d...\n", n, length);
    for (int i = 0; i < n; i++) {
        sequences[i] = (char*)malloc((length + 1) * sizeof(char));
        if (sequences[i] == NULL) {
            printf("erro ao alocar memoria para sequencia %d!\n", i);
            return 1;
        }
        
        // gera a sequencia de modo aleatorio
        generate_dna_sequence(sequences[i], length);
        
    }
    
    // Mede o tempo de ordenação
    printf("\nIniciando ordenacao...\n");
    start = clock();
    sequential_sort(sequences, n);
    end = clock();
    
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    printf("Ordenacao concluida!\n");
    printf("Tempo gasto para ordenar: %.6f segundos\n", cpu_time_used);
    
    // salvando no arquivo
    save_results_to_file(n, length, cpu_time_used);
    
    for (int i = 0; i < n; i++) {
        free(sequences[i]);
    }
    free(sequences);
    
    return 0;
}