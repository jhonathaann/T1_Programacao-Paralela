// gerar_dados.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

void generate_dna_sequence(char *buffer, int length) {
  const char nucleotides[] = "ACGT";
  for (int i = 0; i < length; i++) {
    buffer[i] = nucleotides[rand() % 4];
  }
  buffer[length] = '\0';
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Uso: %s <num_sequencias> <arquivo_saida>\n", argv[0]);
    return 1;
  }

  int num_seqs = atoi(argv[1]);
  const char *filename = argv[2];
  int seq_length = 50; // comprimento fixo das sequencias
  const char *input_dir = "input";
  char outpath[4096];

  srand(time(NULL));

  // certifica que o caminho de saida esteja dentro de input
  if (strncmp(filename, "input/", 6) == 0) {
    snprintf(outpath, sizeof(outpath), "%s", filename);
  } else {
    snprintf(outpath, sizeof(outpath), "%s/%s", input_dir, filename);
  }

  FILE *file = fopen(outpath, "w");
  if (!file) {
    perror("Erro ao criar arquivo");
    return 1;
  }

  char buffer[seq_length + 1];
  for (int i = 0; i < num_seqs; i++) {
    generate_dna_sequence(buffer, seq_length);
    fprintf(file, "%s\n", buffer);
  }

  fclose(file);
  printf("Geradas %d sequencias em %s\n", num_seqs, outpath);

  return 0;
}