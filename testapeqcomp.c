/*
 * Carolina de Assis Souza 2320860 3WC
 * Breno de Andrade Soares 2320363 3WC
 * ------------
 * Este programa é responsável por compilar um arquivo SBas e executar
 * o código de máquina gerado. Ele aceita até três argumentos numéricos
 * na linha de comando, que são passados como parâmetros para a função
 * compilada. 
 */

#include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

/* 
 * Define o tipo da função compilada.
 * A função recebe até 3 inteiros como parâmetros e retorna um inteiro.
 */
typedef int (*func3_t)(int, int, int);

/* 
 * Exibe os primeiros 64 bytes do código de máquina gerado.
 * Essa função é usada apenas para fins de depuração e estudo,
 * permitindo que o usuário veja os bytes binários criados pelo compilador.
 *
 * @param code Buffer contendo o código de máquina gerado.
 */
void dump_code(const unsigned char *code) {
    printf("Machine code (primeiros 64 bytes):\n");
    for (int i = 0; i < 64; i++) {
        printf("[%02d] %02X\n", i, code[i]);
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Uso: %s <p1> [p2] [p3]\n", argv[0]);
        return 1;
    }

    /* 
     * Armazena os valores dos parâmetros passados pela linha de comando.
     * Caso o usuário não informe todos os parâmetros, os demais serão zero.
     */
    int p[3] = {0, 0, 0};
    for (int i = 0; i < argc - 1; i++) {
        p[i] = atoi(argv[i + 1]);
    }

    FILE *f = fopen("testsExamples/programa7.sbas", "r");
    if (!f) {
        perror("Erro ao abrir programa.sbas");
        return 1;
    }

    /*
     * Buffer para armazenar o código de máquina gerado.
     * O tamanho 1024 é mais do que suficiente para armazenar os bytes de um programa SBas simples.
     * Esse buffer é passado para peqcomp, que escreve o código de máquina diretamente nele.
     */
    unsigned char code[1024];

    /* Gera o código de máquina compilando o arquivo SBas */
    func3_t fn = (func3_t) peqcomp(f, code);
    fclose(f);

    /* 
     * A chamada abaixo é opcional e usada apenas para testes e verificação dos bytes gerados.
     */
    // dump_code(code);

    /* Chama a função compilada, passando os parâmetros lidos */
    int result = fn(p[0], p[1], p[2]);

    printf("Resultado: %d\n", result);
    return 0;
}
