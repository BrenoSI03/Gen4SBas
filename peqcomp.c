#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "peqcomp.h"

#define MODRM(src,dst) (0xC0 | ((src) << 3) | (dst))

// Retorna o registrador associado a uma variável local ou parâmetro
static int reg_for(const char *t) {
    if (t[0]=='v' && t[1]>='1' && t[1]<='5') {
        int v = t[1] - '1';
        if (v < 4) return v;  // v1..v4 → eax, ecx, edx, ebx
        return 5;             // v5 → ebp
    }
    if (strcmp(t, "p1")==0) return 7;
    if (strcmp(t, "p2")==0) return 6;
    if (strcmp(t, "p3")==0) return 2;
    return -1;
}

// Função principal que compila o código SBas para código de máquina
funcp peqcomp(FILE *f, unsigned char *code) {
    char line[256], lines[100][256];
    int n = 0;

    // Lê as linhas do arquivo e remove comentários
    while (fgets(line, sizeof line, f) && n < 100) {
        char *c = strstr(line, "//");
        if (c) *c = '\0';
        strcpy(lines[n++], line);
    }

    int line_pos[100];         // armazena índice de código de cada linha
    int jump_pos[100];         // armazena posição do byte do offset de salto
    int jump_target[100];      // armazena linha de destino do salto
    int nj = 0;
    int idx = 0;

    // Prólogo: salva EBX e EBP na pilha
    code[idx++] = 0x53; // push rbx
    code[idx++] = 0x55; // push rbp

    // Percorre todas as linhas
    for (int i = 0; i < n; i++) {
        char *p = lines[i];
        while (isspace((unsigned char)*p)) p++;
        if (!*p) continue;
        line_pos[i] = idx;

        char a[16], b[16], c[16], op;

        // Trata instrução de retorno
        if (sscanf(p, "ret %15s", a) == 1) {
            if (a[0] == '$') {
                int v = atoi(a + 1);
                code[idx++] = 0xB8; // mov eax, imed
                memcpy(code + idx, &v, 4);
                idx += 4;
            } else {
                int r = reg_for(a);
                if (r > 0) {
                    code[idx++] = 0x89; // mov r, eax
                    code[idx++] = MODRM(r, 0);
                }
            }
            code[idx++] = 0x5D; // pop rbp
            code[idx++] = 0x5B; // pop rbx
            code[idx++] = 0xC3; // ret
        }
        // Trata salto condicional
        else if (sscanf(p, "iflez %15s %15s", a, b) == 2) {
            int r = reg_for(a);
            int t = atoi(b);
            code[idx++] = 0x83; // cmp r, 0
            code[idx++] = MODRM(7, r);
            code[idx++] = 0x00;
            code[idx++] = 0x7E; // jle
            code[idx++] = 0x00; // placeholder
            jump_pos[nj] = idx - 1;
            jump_target[nj++] = t;
        }
        // Trata expressão com operador
        else if (sscanf(p, "%15s = %15s %c %15s", a, b, &op, c) == 4) {
            int rd = reg_for(a), rs = reg_for(b);
            if (rd >= 0 && rs >= 0) {
                code[idx++] = 0x89; // mov rs, rd
                code[idx++] = MODRM(rs, rd);
                if (c[0] == '$') {
                    int v = atoi(c + 1);
                    code[idx++] = 0x83;
                    code[idx++] = (op=='+' ? 0xC0|rd : 0xE8|rd);
                    code[idx++] = (unsigned char)v;
                } else {
                    int rt = reg_for(c);
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
        }
        // Trata atribuição simples
        else if (sscanf(p, "%15s : %15s", a, b) == 2) {
            int rd = reg_for(a);
            if (b[0] == '$') {
                int v = atoi(b + 1);
                code[idx++] = 0xB8 | rd; // mov reg, imed
                memcpy(code + idx, &v, 4);
                idx += 4;
            } else {
                int rs = reg_for(b);
                code[idx++] = 0x89; // mov rs, rd
                code[idx++] = MODRM(rs, rd);
            }
        }
    }

    // Ajusta os saltos
    for (int j = 0; j < nj; j++) {
        int pos = jump_pos[j];
        int dest = -1;
        for (int k = 0; k < n; k++) {
            if (k+1 == jump_target[j]) dest = line_pos[k];
        }
        if (dest >= 0)
            code[pos] = (unsigned char)(dest - (pos + 1));
    }

    return (funcp) code;
}
