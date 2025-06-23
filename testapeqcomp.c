
// testapeqcomp.c
#include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

// Imprime em hex o código gerado até o primeiro RET (0xC3)
void dump_code(unsigned char *codigo) {
    int i = 0;
    printf("Machine code bytes:\n");
    while (i < 256) {
        printf("%02X ", codigo[i]);
        if (codigo[i] == 0xC3) break;  // encontra o RET e para
        i++;
    }
    printf("\n(0xC3 em byte %d)\n\n", i);
}

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <valor>\n", argv[0]);
        return 1;
    }
    int param = atoi(argv[1]);

    FILE *myfp = fopen("programa.sbas","r");
    if (!myfp) { perror("fopen"); return 1; }

    unsigned char codigo[1024];
    funcp funcaoSbas = peqcomp(myfp, codigo);
    fclose(myfp);

    // 1) Espia os bytes gerados:
    dump_code(codigo);

    // 2) Executa com o seu argumento
    int res = funcaoSbas(param);
    printf("Resultado para %d → %d\n", param, res);
    return 0;
}



/* #include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

typedef int (*func2_t)(int, int);

// Imprime índice + byte em hex dos 64 primeiros bytes
static void dump_code(const unsigned char *code) {
    printf("Machine code bytes:\n");
    for (int i = 0; i < 64; i++) {
        printf("[%02d] %02X ", i, code[i]);
        // se a sequência anterior for 5D 5B e agora  C3, é o RET real
        if (i >= 2
            && code[i-2] == 0x5D
            && code[i-1] == 0x5B
            && code[i]   == 0xC3) {
            break;
        }
    }
    printf("\n\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <p1> <p2>\n", argv[0]);
        return 1;
    }

    FILE *myfp;
    unsigned char codigo[1024]; // Vetor de código de máquina
    func2_t funcaoSBas;
    int res;

    // Abre o arquivo de entrada
    if ((myfp = fopen("programa.sbas", "r")) == NULL) {
        perror("Falha na abertura do arquivo fonte");
        exit(1);
    }

    // Compila o código SBas
    funcaoSBas = (func2_t) peqcomp(myfp, codigo);
    fclose(myfp);

    // 1) Espia os bytes gerados:
    dump_code(codigo);

    // Chama a função gerada com os valores 10 e 20
    int p1 = atoi(argv[1]);
    int p2 = atoi(argv[2]);
    res = funcaoSBas(p1, p2);

    printf("Resultado para p1=%d p2=%d → %d\n", p1, p2, res);

    return 0;
}
 */



 /* #include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

// Assinatura para até três parâmetros
typedef int (*func3_t)(int, int, int);

// Imprime índice + byte em hex dos 64 primeiros bytes
static void dump_code(const unsigned char *code) {
    puts("Machine code (primeiros 64 bytes):");
    for (int i = 0; i < 64; ++i) {
        printf("[%02d] %02X\n", i, code[i]);
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <p1> <p2> <p3>\n", argv[0]);
        return 1;
    }
    int p1 = atoi(argv[1]);
    int p2 = atoi(argv[2]);
    int p3 = atoi(argv[3]);

    FILE *fp = fopen("programa.sbas", "r");
    if (!fp) {
        perror("programa.sbas");
        return 1;
    }

    unsigned char code[4096];
    funcp gen = peqcomp(fp, code);
    fclose(fp);

    dump_code(code);

    func3_t f = (func3_t)gen;
    int result = f(p1, p2, p3);

    printf("Resultado para p1=%d p2=%d p3=%d → %d\n", p1, p2, p3, result);
    return 0;
} */