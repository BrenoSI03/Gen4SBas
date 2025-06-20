#include <stdio.h>
#include <stdlib.h>
#include "peqcomp.h"

int main(int argc, char *argv[]){
    FILE *myfp;
    unsigned char codigo[1024]; // Vetor de código de máquina
    funcp funcaoSBas;
    int res;

    // Abre o arquivo de entrada
    if ((myfp = fopen("programa.sbas","r")) == NULL){
        perror("Falha na abertura do arquivo fonte");
        exit(1);
    }

    // Compila o código SBas
    funcaoSBas = peqcomp(myfp, codigo);
    fclose(myfp);

    // Chama a função gerada (passando os parâmetros necessários)
    res = (*funcaoSBas)(10);    // Exemplo de chamar a função gerada com argumento 10

    printf("Resultado: %d\n", res); // Imprime o resultado da execução da função

    return 0;
}