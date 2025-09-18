#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define SEQ_LENGTH 50
#define CHUNK_SIZE (SEQ_LENGTH + 1)
#define DNA_CHARS "ACGT"

void generate_dna_sequence(char* seq, int length) {
    for (int i = 0; i < length; i++) {
        seq[i] = DNA_CHARS[rand() % 4];
    }
    seq[length] = '\0';
}

int compare_dna(const void* a, const void* b) {
    return strcmp(*(char**)a, *(char**)b);
}

static char* current_flat;  // Para qsort dos splitters (alternativa a qsort_r)

static int my_splitter_compare(const void* pa, const void* pb) {
    int ia = *(int*)pa;
    int ib = *(int*)pb;
    return strcmp(current_flat + ia * CHUNK_SIZE, current_flat + ib * CHUNK_SIZE);
}

void free_strings(char** arr, int n) {
    if (arr == NULL) return;
    for (int i = 0; i < n; i++) {
        free(arr[i]);
    }
    free(arr);
}

void imprime(char** lista, int size, int rank) {
    printf("P%d: [\"", rank);
    for (int i = 0; i < size; i++) {
        printf("%s\"", lista[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

void parallel_splitsort(char*** local_arr_ptr, int* local_n, MPI_Comm comm);

int main(int argc, char* argv[]) {
    int rank, size;
    char** local_arr = NULL;
    int local_n;
    int total_n = 100000;  // Ajuste para 100k, 1M, 10M nos experimentos

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(time(NULL) + rank);  // Semente única por rank

    if (argc > 1) {
        // Suporte a arquivo: rank 0 lê e conta linhas
        if (rank == 0) {
            FILE* in = fopen(argv[1], "r");
            if (in) {
                char line[SEQ_LENGTH + 2];
                total_n = 0;
                while (fgets(line, sizeof(line), in)) {
                    total_n++;
                }
                rewind(in);
                // Aqui você pode ler para global_arr; por simplicidade, gera aleatório se arquivo inválido
                fclose(in);
            }
        }
        MPI_Bcast(&total_n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    local_n = total_n / size;

    if (rank == 0) {
        char** global_arr = malloc(total_n * sizeof(char*));
        for (int i = 0; i < total_n; i++) {
            global_arr[i] = malloc(CHUNK_SIZE);
            generate_dna_sequence(global_arr[i], SEQ_LENGTH);
        }

        printf("Processo %d: Dados globais originais:\n", rank);
        imprime(global_arr, total_n, rank);

        local_arr = malloc(local_n * sizeof(char*));
        for (int i = 0; i < local_n; i++) {
            local_arr[i] = malloc(CHUNK_SIZE);
            strcpy(local_arr[i], global_arr[i]);
        }

        for (int p = 1; p < size; p++) {
            char* pack = malloc(local_n * CHUNK_SIZE);
            for (int j = 0; j < local_n; j++) {
                memcpy(pack + j * CHUNK_SIZE, global_arr[p * local_n + j], CHUNK_SIZE);
            }
            MPI_Send(pack, local_n * CHUNK_SIZE, MPI_CHAR, p, 0, MPI_COMM_WORLD);
            free(pack);
        }

        free_strings(global_arr, total_n);
    } else {
        local_arr = malloc(local_n * sizeof(char*));
        char* recv_pack = malloc(local_n * CHUNK_SIZE);
        MPI_Recv(recv_pack, local_n * CHUNK_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int j = 0; j < local_n; j++) {
            local_arr[j] = malloc(CHUNK_SIZE);
            memcpy(local_arr[j], recv_pack + j * CHUNK_SIZE, CHUNK_SIZE);
        }
        free(recv_pack);
    }

    parallel_splitsort(&local_arr, &local_n, MPI_COMM_WORLD);

    if (rank == 0) {
        char** sorted_global = malloc(total_n * sizeof(char*));
        int* recv_counts = malloc(size * sizeof(int));
        int* recv_displs = malloc(size * sizeof(int));

        recv_counts[0] = local_n;
        for (int i = 1; i < size; i++) {
            MPI_Recv(&recv_counts[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        recv_displs[0] = 0;
        for (int i = 1; i < size; i++) {
            recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
        }

        for (int i = 0; i < local_n; i++) {
            sorted_global[i] = malloc(CHUNK_SIZE);
            strcpy(sorted_global[i], local_arr[i]);
        }

        for (int p = 1; p < size; p++) {
            int rcount = recv_counts[p];
            char* temp_pack = malloc(rcount * CHUNK_SIZE);
            MPI_Recv(temp_pack, rcount * CHUNK_SIZE, MPI_CHAR, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < rcount; j++) {
                int gidx = recv_displs[p] + j;
                sorted_global[gidx] = malloc(CHUNK_SIZE);
                memcpy(sorted_global[gidx], temp_pack + j * CHUNK_SIZE, CHUNK_SIZE);
            }
            free(temp_pack);
        }

        FILE* out = fopen("output.txt", "w");
        for (int i = 0; i < total_n; i++) {
            fprintf(out, "%s\n", sorted_global[i]);
        }
        fclose(out);

        printf("\nProcesso %d: Dados globais ordenados:\n", rank);
        imprime(sorted_global, total_n, rank);

        free_strings(sorted_global, total_n);
        free(recv_counts);
        free(recv_displs);
    } else {
        MPI_Send(&local_n, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        char* pack = malloc(local_n * CHUNK_SIZE);
        for (int j = 0; j < local_n; j++) {
            memcpy(pack + j * CHUNK_SIZE, local_arr[j], CHUNK_SIZE);
        }
        MPI_Send(pack, local_n * CHUNK_SIZE, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
        free(pack);
    }

    free_strings(local_arr, local_n);
    MPI_Finalize();
    return 0;
}

void parallel_splitsort(char*** local_arr_ptr, int* local_n, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (*local_n == 0) return;

    char** local_arr = *local_arr_ptr;

    // 1. Ordenação local
    qsort(local_arr, *local_n, sizeof(char*), compare_dna);
    printf("Processo %d: Após ordenação local: ", rank);
    imprime(local_arr, *local_n, rank);

    // 2. Selecionar separadores locais
    int num_splitters = size - 1;
    char local_splitters_flat[num_splitters * CHUNK_SIZE];
    for (int i = 0; i < num_splitters; i++) {
        int index = (i + 1) * (*local_n) / size;
        if (index >= *local_n) index = *local_n - 1;
        strcpy(local_splitters_flat + i * CHUNK_SIZE, local_arr[index]);
    }

    // 3. Coletar separadores no processo 0
    char* all_splitters_flat = NULL;
    if (rank == 0) {
        all_splitters_flat = malloc(size * num_splitters * CHUNK_SIZE);
    }
    MPI_Gather(local_splitters_flat, num_splitters * CHUNK_SIZE, MPI_CHAR,
               all_splitters_flat, num_splitters * CHUNK_SIZE, MPI_CHAR, 0, comm);

    // 4. Processo 0 seleciona separadores globais
    char global_splitters_flat[num_splitters * CHUNK_SIZE];
    if (rank == 0) {
        int total_split = size * num_splitters;
        int* indices = malloc(total_split * sizeof(int));
        for (int ii = 0; ii < total_split; ii++) indices[ii] = ii;

        current_flat = all_splitters_flat;
        qsort(indices, total_split, sizeof(int), my_splitter_compare);

        for (int i = 0; i < num_splitters; i++) {
            int index = (i + 1) * total_split / size;
            if (index >= total_split) index = total_split - 1;
            strcpy(global_splitters_flat + i * CHUNK_SIZE, all_splitters_flat + indices[index] * CHUNK_SIZE);
        }
        free(indices);

        printf("Processo %d: Separadores globais: ", rank);
        for (int i = 0; i < num_splitters; i++) {
            printf("\"%s\" ", global_splitters_flat + i * CHUNK_SIZE);
        }
        printf("\n");
    }

    // 5. Broadcast dos separadores globais
    MPI_Bcast(global_splitters_flat, num_splitters * CHUNK_SIZE, MPI_CHAR, 0, comm);

    // 6. Redistribuição
    int* send_counts = calloc(size, sizeof(int));
    for (int i = 0; i < *local_n; i++) {
        char* element = local_arr[i];
        int target = 0;
        while (target < num_splitters && strcmp(element, global_splitters_flat + target * CHUNK_SIZE) > 0) {
            target++;
        }
        send_counts[target]++;
    }

    int* recv_counts = malloc(size * sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, comm);

    int* send_displs = malloc(size * sizeof(int));
    int* recv_displs = malloc(size * sizeof(int));
    send_displs[0] = recv_displs[0] = 0;
    for (int i = 1; i < size; i++) {
        send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }

    int total_recv = recv_displs[size - 1] + recv_counts[size - 1];

    int* send_counts_char = malloc(size * sizeof(int));
    int* recv_counts_char = malloc(size * sizeof(int));
    int* send_displs_char = malloc(size * sizeof(int));
    int* recv_displs_char = malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        send_counts_char[i] = send_counts[i] * CHUNK_SIZE;
        recv_counts_char[i] = recv_counts[i] * CHUNK_SIZE;
    }
    send_displs_char[0] = recv_displs_char[0] = 0;
    for (int i = 1; i < size; i++) {
        send_displs_char[i] = send_displs_char[i - 1] + send_counts_char[i - 1];
        recv_displs_char[i] = recv_displs_char[i - 1] + recv_counts_char[i - 1];
    }

    char* send_buffer = malloc(send_displs_char[size - 1] + send_counts_char[size - 1]);
    int* temp_counts = calloc(size, sizeof(int));
    for (int i = 0; i < *local_n; i++) {
        char* element = local_arr[i];
        int target = 0;
        while (target < num_splitters && strcmp(element, global_splitters_flat + target * CHUNK_SIZE) > 0) {
            target++;
        }
        int pos = temp_counts[target] * CHUNK_SIZE;  // Pos relativo no buffer de target
        memcpy(send_buffer + send_displs_char[target] + pos, element, CHUNK_SIZE);
        temp_counts[target]++;
    }

    char* recv_buffer = malloc(total_recv * CHUNK_SIZE);
    MPI_Alltoallv(send_buffer, send_counts_char, send_displs_char, MPI_CHAR,
                  recv_buffer, recv_counts_char, recv_displs_char, MPI_CHAR, comm);

    char** new_local_arr = malloc(total_recv * sizeof(char*));
    for (int i = 0; i < total_recv; i++) {
        new_local_arr[i] = malloc(CHUNK_SIZE);
        memcpy(new_local_arr[i], recv_buffer + i * CHUNK_SIZE, CHUNK_SIZE);
    }
    free(recv_buffer);

    free_strings(local_arr, *local_n);
    *local_arr_ptr = new_local_arr;
    *local_n = total_recv;

    // 7. Ordenação final
    qsort(new_local_arr, *local_n, sizeof(char*), compare_dna);
    printf("Processo %d: Após redistribuição e sort final: ", rank);
    imprime(new_local_arr, *local_n, rank);

    free(all_splitters_flat);
    free(send_counts);
    free(recv_counts);
    free(send_displs);
    free(recv_displs);
    free(send_counts_char);
    free(recv_counts_char);
    free(send_displs_char);
    free(recv_displs_char);
    free(send_buffer);
    free(temp_counts);
}