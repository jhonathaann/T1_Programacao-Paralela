#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define INSERTION_THRESHOLD 16

void insertion_sort(int *lista, int left, int right);
void merge(int *lista, int left, int mid, int right);
void splitsort(int *lista, int left, int right);
void imprime(int *lista, int size, int rank);
void parallel_splitsort(int **local_arr, int *local_n, MPI_Comm comm);

int main(int argc, char *argv[]) {
    int rank, size;
    int *local_arr = NULL;
    int local_n;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Gerar e distribuir dados
    int total_n = 24;
    local_n = total_n / size;
    
    if (rank == 0) {
        int global_arr[] = {22,7,13,18,2,17,1,14,20,6,10,24,15,9,21,3,16,19,23,4,11,12,5,8};
        printf("Processo %d: Dados globais originais:\n", rank);
        imprime(global_arr, total_n, rank);
        
        local_arr = (int*)malloc(local_n * sizeof(int));
        for (int i = 0; i < local_n; i++) {
            local_arr[i] = global_arr[i];
        }
        
        // Distribuir para outros processos
        for (int i = 1; i < size; i++) {
            MPI_Send(&global_arr[i * local_n], local_n, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        local_arr = (int*)malloc(local_n * sizeof(int));
        MPI_Recv(local_arr, local_n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Ordenação paralela
    parallel_splitsort(&local_arr, &local_n, MPI_COMM_WORLD);
    
    // Coletar resultados CORRETAMENTE
    if (rank == 0) {
        int *sorted_global = (int*)malloc(total_n * sizeof(int));
        int *recv_counts = (int*)malloc(size * sizeof(int));
        int *recv_displs = (int*)malloc(size * sizeof(int));
        
        // Coletar tamanhos primeiro
        recv_counts[0] = local_n;
        for (int i = 1; i < size; i++) {
            MPI_Recv(&recv_counts[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // Calcular deslocamentos
        recv_displs[0] = 0;
        for (int i = 1; i < size; i++) {
            recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
        }
        
        // Coletar dados
        for (int i = 0; i < local_n; i++) {
            sorted_global[i] = local_arr[i];
        }
        
        for (int i = 1; i < size; i++) {
            MPI_Recv(&sorted_global[recv_displs[i]], recv_counts[i], MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        printf("\nProcesso %d: Dados globais ordenados:\n", rank);
        imprime(sorted_global, total_n, rank);
        
        free(sorted_global);
        free(recv_counts);
        free(recv_displs);
    } else {
        // Enviar tamanho e depois dados
        MPI_Send(&local_n, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);  // tag 1: tamanho
        MPI_Send(local_arr, local_n, MPI_INT, 0, 2, MPI_COMM_WORLD); // tag 2: dados
    }
    
    free(local_arr);
    MPI_Finalize();
    return 0;
}

void parallel_splitsort(int **local_arr, int *local_n, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    if (*local_n == 0) return;
    
    // 1. Ordenação local
    splitsort(*local_arr, 0, *local_n - 1);
    printf("Processo %d: Após ordenação local: ", rank);
    imprime(*local_arr, *local_n, rank);
    
    // 2. Selecionar separadores locais
    int num_splitters = size - 1;
    int *local_splitters = (int*)malloc(num_splitters * sizeof(int));
    
    for (int i = 0; i < num_splitters; i++) {
        int index = (i + 1) * (*local_n) / size;
        if (index >= *local_n) index = *local_n - 1;
        local_splitters[i] = (*local_arr)[index];
    }
    
    // 3. Coletar separadores no processo 0
    int *all_splitters = NULL;
    if (rank == 0) {
        all_splitters = (int*)malloc(size * num_splitters * sizeof(int));
    }
    
    MPI_Gather(local_splitters, num_splitters, MPI_INT, 
              all_splitters, num_splitters, MPI_INT, 0, comm);
    
    // 4. Processo 0 seleciona separadores globais
    int *global_splitters = (int*)malloc(num_splitters * sizeof(int));
    if (rank == 0) {
        splitsort(all_splitters, 0, size * num_splitters - 1);
        for (int i = 0; i < num_splitters; i++) {
            int index = (i + 1) * (size * num_splitters) / size;
            if (index >= size * num_splitters) index = size * num_splitters - 1;
            global_splitters[i] = all_splitters[index];
        }
        printf("Processo %d: Separadores globais: ", rank);
        imprime(global_splitters, num_splitters, rank);
    }
    
    // 5. Broadcast dos separadores globais
    MPI_Bcast(global_splitters, num_splitters, MPI_INT, 0, comm);
    
    // 6. Redistribuição
    int *send_counts = (int*)calloc(size, sizeof(int));
    for (int i = 0; i < *local_n; i++) {
        int element = (*local_arr)[i];
        int target = 0;
        while (target < num_splitters && element > global_splitters[target]) {
            target++;
        }
        send_counts[target]++;
    }
    
    int *recv_counts = (int*)malloc(size * sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, comm);
    
    int *send_displs = (int*)malloc(size * sizeof(int));
    int *recv_displs = (int*)malloc(size * sizeof(int));
    send_displs[0] = recv_displs[0] = 0;
    
    for (int i = 1; i < size; i++) {
        send_displs[i] = send_displs[i-1] + send_counts[i-1];
        recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
    }
    
    int total_recv = recv_displs[size-1] + recv_counts[size-1];
    
    int *send_buffer = (int*)malloc(*local_n * sizeof(int));
    int *temp_counts = (int*)calloc(size, sizeof(int));
    
    for (int i = 0; i < *local_n; i++) {
        int element = (*local_arr)[i];
        int target = 0;
        while (target < num_splitters && element > global_splitters[target]) {
            target++;
        }
        int pos = send_displs[target] + temp_counts[target];
        send_buffer[pos] = element;
        temp_counts[target]++;
    }
    
    int *new_local_arr = (int*)malloc(total_recv * sizeof(int));
    MPI_Alltoallv(send_buffer, send_counts, send_displs, MPI_INT,
                 new_local_arr, recv_counts, recv_displs, MPI_INT, comm);
    
    free(*local_arr);
    *local_arr = new_local_arr;
    *local_n = total_recv;
    
    // 7. Ordenação final
    splitsort(*local_arr, 0, *local_n - 1);
    printf("Processo %d: Após redistribuição: ", rank);
    imprime(*local_arr, *local_n, rank);
    
    free(local_splitters);
    free(all_splitters);
    free(global_splitters);
    free(send_counts);
    free(recv_counts);
    free(send_displs);
    free(recv_displs);
    free(send_buffer);
    free(temp_counts);
}

// Funções de ordenação (mantidas)
void insertion_sort(int *lista, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = lista[i];
        int j = i - 1;
        while (j >= left && lista[j] > key) {
            lista[j + 1] = lista[j];
            j--;
        }
        lista[j + 1] = key;
    }
}

void merge(int *lista, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    int *left_arr = (int*)malloc(n1 * sizeof(int));
    int *right_arr = (int*)malloc(n2 * sizeof(int));
    
    for (int i = 0; i < n1; i++) left_arr[i] = lista[left + i];
    for (int i = 0; i < n2; i++) right_arr[i] = lista[mid + 1 + i];
    
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (left_arr[i] <= right_arr[j]) lista[k++] = left_arr[i++];
        else lista[k++] = right_arr[j++];
    }
    while (i < n1) lista[k++] = left_arr[i++];
    while (j < n2) lista[k++] = right_arr[j++];
    
    free(left_arr);
    free(right_arr);
}

void splitsort(int *lista, int left, int right) {
    if (right - left + 1 <= INSERTION_THRESHOLD) {
        insertion_sort(lista, left, right);
    } else {
        int mid = left + (right - left) / 2;
        splitsort(lista, left, mid);
        splitsort(lista, mid + 1, right);
        merge(lista, left, mid, right);
    }
}

void imprime(int *lista, int size, int rank) {
    printf("P%d: [", rank);
    for (int i = 0; i < size; i++) {
        printf("%d", lista[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}