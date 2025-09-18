#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define INSERTION_THRESHOLD 16
#define MAX_SEQ_LENGTH 100

// Função de comparação para DNA
int compare_dna(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// Funções de ordenação adaptadas para strings
void insertion_sort(char **lista, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        char *key = lista[i];
        int j = i - 1;
        while (j >= left && strcmp(lista[j], key) > 0) {
            lista[j + 1] = lista[j];
            j--;
        }
        lista[j + 1] = key;
    }
}

void merge(char **lista, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    char **left_arr = (char**)malloc(n1 * sizeof(char*));
    char **right_arr = (char**)malloc(n2 * sizeof(char*));
    
    for (int i = 0; i < n1; i++) left_arr[i] = lista[left + i];
    for (int i = 0; i < n2; i++) right_arr[i] = lista[mid + 1 + i];
    
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (strcmp(left_arr[i], right_arr[j]) <= 0) {
            lista[k++] = left_arr[i++];
        } else {
            lista[k++] = right_arr[j++];
        }
    }
    
    while (i < n1) lista[k++] = left_arr[i++];
    while (j < n2) lista[k++] = right_arr[j++];
    
    free(left_arr);
    free(right_arr);
}

void splitsort(char **lista, int left, int right) {
    if (right - left + 1 <= INSERTION_THRESHOLD) {
        insertion_sort(lista, left, right);
    } else {
        int mid = left + (right - left) / 2;
        splitsort(lista, left, mid);
        splitsort(lista, mid + 1, right);
        merge(lista, left, mid, right);
    }
}

// Função para ler arquivo de entrada
char** read_dna_file(const char* filename, int* total_seqs) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return NULL;
    }
    
    char buffer[MAX_SEQ_LENGTH];
    int capacity = 10000;
    int count = 0;
    
    char** sequences = (char**)malloc(capacity * sizeof(char*));
    
    while (fgets(buffer, MAX_SEQ_LENGTH, file)) {
        // Remover newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (count >= capacity) {
            capacity *= 2;
            sequences = (char**)realloc(sequences, capacity * sizeof(char*));
        }
        
        sequences[count] = (char*)malloc((strlen(buffer) + 1) * sizeof(char));
        strcpy(sequences[count], buffer);
        count++;
    }
    
    fclose(file);
    *total_seqs = count;
    return sequences;
}

// Função para escrever arquivo de saída
void write_dna_file(const char* filename, char** sequences, int total_seqs) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao criar arquivo de saída");
        return;
    }
    
    for (int i = 0; i < total_seqs; i++) {
        fprintf(file, "%s\n", sequences[i]);
    }
    
    fclose(file);
}

// Função principal de ordenação paralela
void parallel_dna_sort(char ***local_seqs, int *local_n, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    if (*local_n == 0) return;
    
    // 1. Ordenação local
    splitsort(*local_seqs, 0, *local_n - 1);
    
    // 2. Selecionar separadores locais (baseado na primeira string)
    int num_splitters = size - 1;
    char **local_splitters = (char**)malloc(num_splitters * sizeof(char*));
    
    for (int i = 0; i < num_splitters; i++) {
        int index = (i + 1) * (*local_n) / size;
        if (index >= *local_n) index = *local_n - 1;
        local_splitters[i] = (*local_seqs)[index];
    }
    
    // 3. Coletar separadores no processo 0
    char **all_splitters = NULL;
    if (rank == 0) {
        all_splitters = (char**)malloc(size * num_splitters * sizeof(char*));
    }
    
    // Enviar separadores como strings
    for (int i = 0; i < num_splitters; i++) {
        int len = strlen(local_splitters[i]) + 1;
        if (rank == 0) {
            for (int j = 0; j < size; j++) {
                if (j == 0) {
                    all_splitters[i] = (char*)malloc(len * sizeof(char));
                    strcpy(all_splitters[i], local_splitters[i]);
                } else {
                    char* temp = (char*)malloc(len * sizeof(char));
                    MPI_Recv(temp, len, MPI_CHAR, j, 0, comm, MPI_STATUS_IGNORE);
                    all_splitters[j * num_splitters + i] = temp;
                }
            }
        } else {
            MPI_Send(local_splitters[i], len, MPI_CHAR, 0, 0, comm);
        }
    }
    
    // 4. Processo 0 seleciona separadores globais
    char **global_splitters = (char**)malloc(num_splitters * sizeof(char*));
    if (rank == 0) {
        splitsort(all_splitters, 0, size * num_splitters - 1);
        for (int i = 0; i < num_splitters; i++) {
            int index = (i + 1) * (size * num_splitters) / size;
            if (index >= size * num_splitters) index = size * num_splitters - 1;
            global_splitters[i] = all_splitters[index];
        }
    }
    
    // Broadcast dos separadores globais
    for (int i = 0; i < num_splitters; i++) {
        if (rank == 0) {
            int len = strlen(global_splitters[i]) + 1;
            for (int j = 1; j < size; j++) {
                MPI_Send(global_splitters[i], len, MPI_CHAR, j, 1, comm);
            }
        } else {
            char buffer[MAX_SEQ_LENGTH];
            MPI_Recv(buffer, MAX_SEQ_LENGTH, MPI_CHAR, 0, 1, comm, MPI_STATUS_IGNORE);
            global_splitters[i] = (char*)malloc((strlen(buffer) + 1) * sizeof(char));
            strcpy(global_splitters[i], buffer);
        }
    }
    
    // 5. Redistribuição simplificada (usando qsort padrão para demonstração)
    // Para implementação completa, seria necessário MPI_Alltoallv personalizado
    // Aqui usamos uma abordagem simplificada
    
    // Ordenação final local
    splitsort(*local_seqs, 0, *local_n - 1);
    
    // Limpeza
    for (int i = 0; i < num_splitters; i++) {
        if (rank != 0) free(global_splitters[i]);
    }
    free(local_splitters);
    if (rank == 0) {
        for (int i = 0; i < size * num_splitters; i++) {
            free(all_splitters[i]);
        }
        free(all_splitters);
    }
    free(global_splitters);
}

int main(int argc, char *argv[]) {
    int rank, size;
    char **local_seqs = NULL;
    int local_n = 0;
    double start_time, end_time;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc != 3) {
        if (rank == 0) {
            printf("Uso: %s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    start_time = MPI_Wtime();
    
    // Ler arquivo apenas no processo 0
    char **all_sequences = NULL;
    int total_seqs = 0;
    
    if (rank == 0) {
        all_sequences = read_dna_file(argv[1], &total_seqs);
        if (!all_sequences) {
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        printf("Lidas %d sequências de DNA\n", total_seqs);
    }
    
    // Broadcast do número total de sequências
    MPI_Bcast(&total_seqs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Distribuir sequências
    local_n = total_seqs / size;
    int remainder = total_seqs % size;
    
    int *counts = (int*)malloc(size * sizeof(int));
    int *displs = (int*)malloc(size * sizeof(int));
    
    for (int i = 0; i < size; i++) {
        counts[i] = local_n + (i < remainder ? 1 : 0);
        displs[i] = (i == 0) ? 0 : displs[i-1] + counts[i-1];
    }
    
    local_n = counts[rank];
    local_seqs = (char**)malloc(local_n * sizeof(char*));
    
    // Distribuir contagens primeiro
    MPI_Scatter(counts, 1, MPI_INT, MPI_IN_PLACE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Distribuir strings
    if (rank == 0) {
        for (int i = 1; i < size; i++) {
            for (int j = 0; j < counts[i]; j++) {
                int len = strlen(all_sequences[displs[i] + j]) + 1;
                MPI_Send(all_sequences[displs[i] + j], len, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            }
        }
        // Copiar dados locais
        for (int i = 0; i < counts[0]; i++) {
            local_seqs[i] = (char*)malloc((strlen(all_sequences[i]) + 1) * sizeof(char));
            strcpy(local_seqs[i], all_sequences[i]);
        }
    } else {
        for (int i = 0; i < local_n; i++) {
            char buffer[MAX_SEQ_LENGTH];
            MPI_Recv(buffer, MAX_SEQ_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local_seqs[i] = (char*)malloc((strlen(buffer) + 1) * sizeof(char));
            strcpy(local_seqs[i], buffer);
        }
    }
    
    // Ordenação paralela
    parallel_dna_sort(&local_seqs, &local_n, MPI_COMM_WORLD);
    
    // Coletar resultados
    if (rank == 0) {
        char **sorted_sequences = (char**)malloc(total_seqs * sizeof(char*));
        
        // Copiar dados locais
        for (int i = 0; i < counts[0]; i++) {
            sorted_sequences[i] = local_seqs[i];
        }
        
        // Receber de outros processos
        for (int i = 1; i < size; i++) {
            for (int j = 0; j < counts[i]; j++) {
                char buffer[MAX_SEQ_LENGTH];
                MPI_Recv(buffer, MAX_SEQ_LENGTH, MPI_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                sorted_sequences[displs[i] + j] = (char*)malloc((strlen(buffer) + 1) * sizeof(char));
                strcpy(sorted_sequences[displs[i] + j], buffer);
            }
        }
        
        // Ordenação final global (necessária devido à redistribuição simplificada)
        qsort(sorted_sequences, total_seqs, sizeof(char*), compare_dna);
        
        // Escrever arquivo de saída
        write_dna_file(argv[2], sorted_sequences, total_seqs);
        
        end_time = MPI_Wtime();
        printf("Tempo de execução: %.4f segundos\n", end_time - start_time);
        
        // Liberar memória
        for (int i = 0; i < total_seqs; i++) {
            free(sorted_sequences[i]);
        }
        free(sorted_sequences);
        free(all_sequences);
        
    } else {
        // Enviar dados ordenados
        for (int i = 0; i < local_n; i++) {
            MPI_Send(local_seqs[i], strlen(local_seqs[i]) + 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
        }
    }
    
    // Liberar memória local
    for (int i = 0; i < local_n; i++) {
        free(local_seqs[i]);
    }
    free(local_seqs);
    free(counts);
    free(displs);
    
    MPI_Finalize();
    return 0;
}