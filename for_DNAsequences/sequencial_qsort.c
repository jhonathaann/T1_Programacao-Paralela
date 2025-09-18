#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SEQ_LENGTH 1000000
#define DNA_CHARS "ACGT"

// gera sequencias de modo aleatorio
void generate_dna_sequence(char *seq, int length) {
  for (int i = 0; i < length; i++) {
    seq[i] = DNA_CHARS[rand() % 4];
  }
  seq[length] = '\0';
}

// realiza a comparacao entre os chars (A, C, G e T)
int compare_dna(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

// realiza a ordenacao usando o qsort sequencial
void sequential_sort(char **data, int n) {
  qsort(data, n, sizeof(char *), compare_dna);
}

// função para salvar os resultados no arquivo
void save_results_to_file(int n, int length, double time_taken) {
  FILE *file = fopen(
      "/home/jhonathan/T1_ProgP/for_DNAsequences/resultados_ordenacao.txt",
      "a");
  if (file == NULL) {
    printf("erro ao abrir o arquivo para salvar resultados!\n");
    return;
  }

  fprintf(file, "sequencias: %d, tamanho: %d, tempo: %.6f segundos\n", n,
          length, time_taken);
  fclose(file);

  printf("Resultados salvos em 'resultados_ordenacao.txt'\n");
}

// Leitura de arquivo de sequencias (uma por linha)
char **read_dna_file(const char *filename, int *total_seqs) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Erro ao abrir arquivo");
    return NULL;
  }

  char buffer[MAX_SEQ_LENGTH];
  int capacity = 10000;
  int count = 0;

  char **sequences = (char **)malloc(capacity * sizeof(char *));
  if (!sequences) {
    fclose(file);
    return NULL;
  }

  while (fgets(buffer, MAX_SEQ_LENGTH, file)) {
    buffer[strcspn(buffer, "\n")] = 0;
    if (count >= capacity) {
      capacity *= 2;
      sequences = (char **)realloc(sequences, capacity * sizeof(char *));
      if (!sequences) {
        fclose(file);
        return NULL;
      }
    }
    sequences[count] = (char *)malloc((strlen(buffer) + 1) * sizeof(char));
    strcpy(sequences[count], buffer);
    count++;
  }

  fclose(file);
  *total_seqs = count;
  return sequences;
}

void write_dna_file(const char *filename, char **sequences, int total_seqs) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    perror("Erro ao criar arquivo de saída");
    return;
  }
  for (int i = 0; i < total_seqs; i++) {
    fprintf(file, "%s\n", sequences[i]);
  }
  fclose(file);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Uso: %s <arquivo_entrada> <arquivo_saida>\n", argv[0]);
    return 1;
  }

  const char *input_file = argv[1];
  const char *output_file = argv[2];

  int n = 0;
  char **sequences = read_dna_file(input_file, &n);
  if (!sequences) {
    return 1;
  }

  int length = 0;
  if (n > 0)
    length = strlen(sequences[0]);

  clock_t start = clock();
  sequential_sort(sequences, n);
  clock_t end = clock();
  double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Ordenacao concluida!\n");
  printf("Tempo gasto para ordenar: %.6f segundos\n", cpu_time_used);

  // salvar resultados (mantemos o caminho existente)
  save_results_to_file(n, length, cpu_time_used);

  // escrever sequencias ordenadas
  write_dna_file(output_file, sequences, n);

  for (int i = 0; i < n; i++)
    free(sequences[i]);
  free(sequences);

  return 0;
}