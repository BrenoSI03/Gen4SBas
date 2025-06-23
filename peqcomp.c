/*
 * Carolina de Assis Souza 2320860 3WC
 * Breno de Andrade Soares 2320363 3WC
 * -------------
 * Este arquivo implementa o compilador peqcomp, que traduz código SBas
 * para código de máquina em um buffer de bytes. O compilador suporta operações
 * aritméticas simples, atribuições, saltos condicionais e retorno de valores.
 * Além disso, gera prólogo e epílogo para preservar registradores utilizados.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "peqcomp.h"

#define MODRM(src,dst) (0xC0 | ((src) << 3) | (dst))

/**
* Função: reg_for
 * ----------------
 * Retorna o índice do registrador associado a uma variável local (v1..v5)
 * ou a um parâmetro (p1..p3).

 * @param t Nome da variável local (v1..v5) ou parâmetro (p1..p3).
 * @return Índice do registrador correspondente ou -1 se inválido.
 */
static int reg_for(const char *t) {
    if (t[0] == 'v' && t[1] >= '1' && t[1] <= '5') {
        int v = t[1] - '1';
        if (v < 4)
            return v;        /* v1..v4 → eax, ecx, edx, ebx */
        return 5;            /* v5 → ebp */
    }
    if (strcmp(t, "p1") == 0) return 7; /* p1 → edi */
    if (strcmp(t, "p2") == 0) return 6; /* p2 → esi */
    if (strcmp(t, "p3") == 0) return 2; /* p3 → edx */
    return -1;
}


/*
 * Função: peqcomp
 * ----------------
 * Compila o código-fonte SBas fornecido em um arquivo para código de máquina,
 * armazenando os bytes gerados no buffer code. A função realiza duas passagens:
 * 1. Gera o código de máquina para cada linha.
 * 2. Ajusta os deslocamentos dos saltos condicionais.
 *
 * @param f Arquivo com o código-fonte SBas.
 * @param code Buffer onde será armazenado o código de máquina gerado.
 * @return Ponteiro para a função compilada que pode ser chamada como funcp.
 */

funcp peqcomp(FILE *f, unsigned char *code) {
    char buf[256], lines[100][256];
    int n = 0;

    /* Lê todas as linhas do arquivo, removendo comentários e armazenando em lines */
    while (fgets(buf, sizeof buf, f) && n < 100) {
        char *com = strstr(buf, "//");
        if (com)
            *com = '\0';
        strcpy(lines[n++], buf);
    }

    int line_pos[100];      /* Armazena a posição no buffer de código de cada linha */
    int jump_pos[100];      /* Armazena a posição do byte do offset de cada salto */
    int jump_tgt[100];      /* Armazena o número da linha destino de cada salto */
    int nj = 0;
    int idx = 0;

    /* Gera prólogo: salva EBX e EBP na pilha para preservação */
    code[idx++] = 0x53; /* push rbx */
    code[idx++] = 0x55; /* push rbp */

    /* Geração do código de máquina linha por linha */
    for (int i = 0; i < n; i++) {
        char *p = lines[i];
        while (isspace((unsigned char)*p)) p++;
        if (!*p) continue;  /* Ignora linhas vazias */
        line_pos[i] = idx;

        char a[16], b[16], cst[16], op;
        int rd, rs, rt, val;

        /* Trata comando de retorno: ret $valor ou ret var */
        if (sscanf(p, "ret %15s", a) == 1) {
            if (a[0] == '$') {
                val = atoi(a + 1);
                code[idx++] = 0xB8;
                memcpy(code + idx, &val, 4);
                idx += 4;
            } else if ((rd = reg_for(a)) > 0) {
                code[idx++] = 0x89;
                code[idx++] = MODRM(rd, 0);
            }
            code[idx++] = 0x5D; /* pop rbp */
            code[idx++] = 0x5B; /* pop rbx */
            code[idx++] = 0xC3; /* ret */
            continue;
        }

        /* Trata salto condicional iflez */
        if (sscanf(p, "iflez %15s %15s", a, b) == 2) {
            rd = reg_for(a);
            val = atoi(b);
            code[idx++] = 0x83;
            code[idx++] = MODRM(7, rd);
            code[idx++] = 0x00;
            code[idx++] = 0x7E;
            code[idx++] = 0x00; /* Offset a ser ajustado depois */
            jump_pos[nj] = idx - 1;
            jump_tgt[nj++] = val;
            continue;
        }

        /* Trata operações aritméticas: var = var op var/$const */
        if (sscanf(p, "%15s = %15s %c %15s", a, b, &op, cst) == 4) {
            rd = reg_for(a);
            rs = reg_for(b);
            if (rd >= 0 && rs >= 0) {
                code[idx++] = 0x89;
                code[idx++] = MODRM(rs, rd);
                if (cst[0] == '$') {
                    val = atoi(cst + 1);
                    code[idx++] = 0x83;
                    code[idx++] = (op == '+' ? (0xC0 | rd) : (0xE8 | rd));
                    code[idx++] = (unsigned char) val;
                } else if ((rt = reg_for(cst)) >= 0) {
                    if (op == '+') {
                        code[idx++] = 0x01;
                        code[idx++] = MODRM(rt, rd);
                    } else if (op == '-') {
                        code[idx++] = 0x29;
                        code[idx++] = MODRM(rt, rd);
                    } else if (op == '*') {
                        code[idx++] = 0x0F;
                        code[idx++] = 0xAF;
                        code[idx++] = MODRM(rd, rt);
                    }
                }
            }
            continue;
        }

        /* Trata atribuições simples: var : var/$const */
        if (sscanf(p, "%15s : %15s", a, b) == 2) {
            rd = reg_for(a);
            if (b[0] == '$') {
                val = atoi(b + 1);
                code[idx++] = 0xB8 | rd;
                memcpy(code + idx, &val, 4);
                idx += 4;
            } else if ((rs = reg_for(b)) >= 0) {
                code[idx++] = 0x89;
                code[idx++] = MODRM(rs, rd);
            }
            continue;
        }
    }

    /* Segunda passagem: ajusta os deslocamentos dos saltos gerados */
    for (int j = 0; j < nj; j++) {
        int dst = -1;
        for (int k = 0; k < n; k++) {
            if (k + 1 == jump_tgt[j])
                dst = line_pos[k];
        }
        if (dst >= 0)
            code[jump_pos[j]] = (unsigned char)(dst - (jump_pos[j] + 1));
    }

    return (funcp) code;
}
