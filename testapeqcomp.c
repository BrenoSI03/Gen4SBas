#include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

/* Mostra os 64 primeiros bytes gerados (independentemente de RET). */
static void dump_code(const unsigned char *code) {
    puts("Machine code (primeiros 64 bytes):");
    for (int i = 0; i < 64; ++i) {
        printf("[%02d] %02X\n", i, code[i]);
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <p1> <p2>\n", argv[0]);
        return 1;
    }
    int p1 = atoi(argv[1]);
    int p2 = atoi(argv[2]);

    FILE *f = fopen("programa.sbas", "r");
    if (!f) { perror("programa.sbas"); return 1; }

    unsigned char code[1024];
    funcp fn = peqcomp(f, code);
    fclose(f);

    dump_code(code);

    /* chama fn(p1,p2)  — p1 (EDI) já virá via operando 'D', p2 (ESI) setamos manual */
    int result;
    asm volatile (
        "mov %[p2], %%esi\n\t"   /* p2 → ESI */
        "call *%[fun]\n\t"       /* EDI já contém p1 */
        : "=a"(result)           /* EAX ← resultado */
        : [fun]"r"(fn), [p1]"D"(p1), [p2]"r"(p2)
        : "esi", "memory"
    );

    printf("Resultado para p1=%d p2=%d → %d\n", p1, p2, result);
    return 0;
}




/* // testapeqcomp.c
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
    funcp func = peqcomp(myfp, codigo);
    fclose(myfp);

    // 1) Espia os bytes gerados:
    dump_code(codigo);

    // 2) Executa com o seu argumento
    int res = func(param);
    printf("Resultado para %d → %d\n", param, res);
    return 0;
} */

/* #include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

typedef int (*func2_t)(int, int);

int main(void) {
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

    // Chama a função gerada com os valores 10 e 20
    res = funcaoSBas(10, 20);

    printf("Resultado: %d\n", res);

    return 0;
}
 */