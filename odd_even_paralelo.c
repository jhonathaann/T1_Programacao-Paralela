#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define DNA_CHARS "ACGT"

// Funções auxiliares
void generate_dna_sequence(char* seq, int length) {
    for (int i = 0; i < length; i++) {
        seq[i] = DNA_CHARS[rand() % 4];
    }
    seq[length] = '\0';
}

int compare_dna(const void* a, const void* b) {
    return strcmp((char*)a, (char*)b);
}

// Função para mesclar dois arrays ordenados
void merge_sorted_arrays(char** arr1, int size1, char** arr2, int size2, char** result) {
    int i = 0, j = 0, k = 0;
    
    while (i < size1 && j < size2) {
        if (strcmp(arr1[i], arr2[j]) <= 0) {
            strcpy(result[k++], arr1[i++]);
        } else {
            strcpy(result[k++], arr2[j++]);
        }
    }
    
    while (i < size1) {
        strcpy(result[k++], arr1[i++]);
    }
    
    while (j < size2) {
        strcpy(result[k++], arr2[j++]);
    }
}

// Operação compare-separa (em vez de compare-troca)
void compare_separate(int partner, char** local_data, int local_size, 
                     char** partner_data, int partner_size, int seq_length, 
                     int keep_smaller, int my_rank) {
    
    // Envia tamanho primeiro
    MPI_Send(&local_size, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
    
    // Envia todos os dados locais
    for (int i = 0; i < local_size; i++) {
        MPI_Send(local_data[i], seq_length + 1, MPI_CHAR, partner, 1, MPI_COMM_WORLD);
    }
    
    // Recebe tamanho do parceiro
    MPI_Recv(&partner_size, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // Aloca memória para dados do parceiro
    partner_data = (char**)malloc(partner_size * sizeof(char*));
    for (int i = 0; i < partner_size; i++) {
        partner_data[i] = (char*)malloc((seq_length + 1) * sizeof(char));
        MPI_Recv(partner_data[i], seq_length + 1, MPI_CHAR, partner, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Mescla os dois arrays
    char** merged = (char**)malloc((local_size + partner_size) * sizeof(char*));
    for (int i = 0; i < local_size + partner_size; i++) {
        merged[i] = (char*)malloc((seq_length + 1) * sizeof(char));
    }
    
    merge_sorted_arrays(local_data, local_size, partner_data, partner_size, merged);
    
    // Decide quais elementos manter
    if (keep_smaller) {
        // Mantém os menores
        for (int i = 0; i < local_size; i++) {
            strcpy(local_data[i], merged[i]);
        }
    } else {
        // Mantém os maiores
        for (int i = 0; i < local_size; i++) {
            strcpy(local_data[i], merged[partner_size + i]);
        }
    }
    
    // Libera memória
    for (int i = 0; i < local_size + partner_size; i++) {
        free(merged[i]);
    }
    free(merged);
    
    for (int i = 0; i < partner_size; i++) {
        free(partner_data[i]);
    }
    free(partner_data);
}

// Algoritmo Odd-Even Sort paralelo com blocos
void odd_even_parallel_sort_blocks(char** local_data, int local_size, int seq_length, 
                                  int n, int my_rank, int num_procs) {
    
    // Primeiro passo: ordenação local
    qsort(local_data, local_size, sizeof(char*), compare_dna);
    
    for (int i = 1; i <= num_procs; i++) {
        if (i % 2 == 1) { // Iteração ímpar
            if (my_rank % 2 == 1) { // Processo ímpar
                if (my_rank < num_procs - 1) {
                    int partner_size;
                    char** partner_data;
                    compare_separate(my_rank + 1, local_data, local_size, 
                                   partner_data, partner_size, seq_length, 
                                   1, my_rank); // Mantém os menores
                }
            } else { // Processo par
                if (my_rank > 0) {
                    int partner_size;
                    char** partner_data;
                    compare_separate(my_rank - 1, local_data, local_size, 
                                   partner_data, partner_size, seq_length, 
                                   0, my_rank); // Mantém os maiores
                }
            }
        } else { // Iteração par
            if (my_rank % 2 == 0) { // Processo par
                if (my_rank < num_procs - 1) {
                    int partner_size;
                    char** partner_data;
                    compare_separate(my_rank + 1, local_data, local_size, 
                                   partner_data, partner_size, seq_length, 
                                   1, my_rank); // Mantém os menores
                }
            } else { // Processo ímpar
                if (my_rank > 0) {
                    int partner_size;
                    char** partner_data;
                    compare_separate(my_rank - 1, local_data, local_size, 
                                   partner_data, partner_size, seq_length, 
                                   0, my_rank); // Mantém os maiores
                }
            }
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

int main(int argc, char** argv) {
    int my_rank, num_procs;
    int n, seq_length;
    double start_time, end_time;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    if (argc != 3) {
        if (my_rank == 0) {
            printf("Uso: %s <numero_total_de_sequencias> <comprimento_das_sequencias>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    n = atoi(argv[1]);
    seq_length = atoi(argv[2]);
    
    // Calcula o número de elementos por processo
    int local_n = n / num_procs;
    int remainder = n % num_procs;
    
    // Distribui elementos extras
    if (my_rank < remainder) {
        local_n++;
    }
    
    // Semente aleatória consistente
    srand(42 + my_rank);
    
    // Cada processo gera suas próprias sequências
    char** local_sequences = (char**)malloc(local_n * sizeof(char*));
    for (int i = 0; i < local_n; i++) {
        local_sequences[i] = (char*)malloc((seq_length + 1) * sizeof(char));
        generate_dna_sequence(local_sequences[i], seq_length);
    }
    
    // Processo 0 coleta e exibe TODAS as sequências iniciais
    if (my_rank == 0) {
        printf("\n=== ODD-EVEN SORT PARALELO COM BLOCOS ===\n");
        printf("Numero total de sequencias: %d\n", n);
        printf("Numero de processos: %d\n", num_procs);
        printf("Comprimento das sequencias: %d\n", seq_length);
        printf("Elementos por processo: ~%d\n", n / num_procs);
        
        // Coleta informações sobre distribuição
        int* counts = (int*)malloc(num_procs * sizeof(int));
        int* displacements = (int*)malloc(num_procs * sizeof(int));
        
        counts[0] = local_n;
        displacements[0] = 0;
        
        for (int i = 1; i < num_procs; i++) {
            int other_n = n / num_procs;
            if (i < remainder) other_n++;
            counts[i] = other_n;
            displacements[i] = displacements[i-1] + counts[i-1];
        }
        
        // Aloca memória para todas as sequências iniciais
        char* all_initial_sequences = (char*)malloc(n * (seq_length + 1) * sizeof(char));
        
        // Coleta todas as sequências
        MPI_Gatherv(local_sequences[0], local_n * (seq_length + 1), MPI_CHAR,
                   all_initial_sequences, counts, displacements, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        printf("\n=== DISTRIBUICAO INICIAL ===\n");
        int offset = 0;
        for (int i = 0; i < num_procs; i++) {
            printf("\nProcesso %d (%d elementos):\n", i, counts[i]);
            for (int j = 0; j < counts[i]; j++) {
                char* seq_ptr = all_initial_sequences + (offset + j) * (seq_length + 1);
                printf("  [%d] %s\n", j, seq_ptr);
            }
            offset += counts[i];
        }
        
        free(counts);
        free(displacements);
        free(all_initial_sequences);
    } else {
        // Outros processos enviam suas sequências
        MPI_Gatherv(local_sequences[0], local_n * (seq_length + 1), MPI_CHAR,
                   NULL, 0, NULL, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    
    // Mede o tempo da ordenação paralela
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    odd_even_parallel_sort_blocks(local_sequences, local_n, seq_length, n, my_rank, num_procs);
    
    end_time = MPI_Wtime();
    double parallel_time = end_time - start_time;
    
    // Coleta os tempos
    double max_time;
    MPI_Reduce(&parallel_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    // Processo 0 coleta e exibe os resultados finais
    if (my_rank == 0) {
        // Prepara para Gatherv
        int* counts = (int*)malloc(num_procs * sizeof(int));
        int* displacements = (int*)malloc(num_procs * sizeof(int));
        
        // Primeiro obtém os tamanhos de todos os processos
        int my_count = local_n;
        MPI_Gather(&my_count, 1, MPI_INT, counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        displacements[0] = 0;
        for (int i = 1; i < num_procs; i++) {
            displacements[i] = displacements[i-1] + counts[i-1];
        }
        
        // Aloca memória para todas as sequências finais
        char* all_final_sequences = (char*)malloc(n * (seq_length + 1) * sizeof(char));
        
        // Coleta todas as sequências ordenadas
        MPI_Gatherv(local_sequences[0], local_n * (seq_length + 1), MPI_CHAR,
                   all_final_sequences, counts, displacements, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        printf("\n=== RESULTADO FINAL ===\n");
        printf("Tempo de execucao: %.6f segundos\n", max_time);
        printf("Tempo de execucao: %.3f milissegundos\n", max_time * 1000);
        
        printf("\nSequencias ordenadas por processo:\n");
        int offset = 0;
        for (int i = 0; i < num_procs; i++) {
            printf("\nProcesso %d (%d elementos):\n", i, counts[i]);
            for (int j = 0; j < counts[i]; j++) {
                char* seq_ptr = all_final_sequences + (offset + j) * (seq_length + 1);
                printf("  [%d] %s\n", j, seq_ptr);
            }
            offset += counts[i];
        }
        
        // Verifica ordenação global
        printf("\nVerificacao de ordenacao global:\n");
        int sorted = 1;
        for (int i = 0; i < n - 1; i++) {
            char* seq1 = all_final_sequences + i * (seq_length + 1);
            char* seq2 = all_final_sequences + (i + 1) * (seq_length + 1);
            if (strcmp(seq1, seq2) > 0) {
                sorted = 0;
                printf("ERRO: %s > %s (posicoes %d-%d)\n", seq1, seq2, i, i+1);
                break;
            }
        }
        printf("Ordenacao global: %s\n", sorted ? "CORRETO" : "INCORRETO");
        
        free(counts);
        free(displacements);
        free(all_final_sequences);
        
    } else {
        // Outros processos enviam tamanho e dados
        MPI_Gather(&local_n, 1, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gatherv(local_sequences[0], local_n * (seq_length + 1), MPI_CHAR,
                   NULL, 0, NULL, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    
    // Libera memória
    for (int i = 0; i < local_n; i++) {
        free(local_sequences[i]);
    }
    free(local_sequences);
    
    MPI_Finalize();
    return 0;
}