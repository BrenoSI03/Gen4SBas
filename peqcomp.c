#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "peqcomp.h"

#define MODRM(src, dst) (0xC0 | ((src) << 3) | (dst))

// Mapa de linha → índice do byte onde começa o código
typedef struct {
    int line_num;
    int code_idx;
} LineMap;

// Registro de cada salto a ser ajustado
typedef struct {
    int code_idx;    // índice em code[] onde está o byte de offset
    int target_line; // linha para a qual saltar
} JumpPatch;

// Converte “v1”→0, “v2”→1, …, “p1”→7
static int reg_for(const char *t) {
    if (t[0]=='v' && isdigit((unsigned char)t[1]))  /* v1..v4 → eax..ebx */
        return t[1]-'1';
    if (strcmp(t,"p1")==0) return 7;                /* p1 → edi */
    if (strcmp(t,"p2")==0) return 6;                /* p2 → esi */
    return -1;
}


funcp peqcomp(FILE *f, unsigned char *code) {
    char linhas[100][256];
    int nlin = 0;

    // 1ª PASSAGEM: lê e limpa comentários
    while (fgets(linhas[nlin], sizeof linhas[nlin], f)) {
        char *c = strstr(linhas[nlin], "//");
        if (c) *c = '\0';
        nlin++;
    }

    LineMap   linemap[100];
    JumpPatch jumps[100];
    int njump = 0;
    int idx   = 0;

    // 2ª PASSAGEM: gera bytes e marca saltos
    for (int i = 0; i < nlin; i++) {
        char *p = linhas[i];
        while (isspace((unsigned char)*p)) p++;
        if (!*p) continue;

        linemap[i].line_num  = i + 1;
        linemap[i].code_idx  = idx;

        char A[16], B[16], C[16];
        char op;

        // RET
        /* peqcomp.c – agora sem o break que interrompia a geração */
        /* RET $N ou RET var */
        if (sscanf(p, "ret %15s", A) == 1) {
            if (A[0] == '$') {                 /* imediato */
                int v = atoi(A + 1);
                code[idx++] = 0xB8;            /* mov eax, imm32 */
                code[idx++] =  v        & 0xFF;
                code[idx++] = (v >> 8)  & 0xFF;
                code[idx++] = (v >> 16) & 0xFF;
                code[idx++] = (v >> 24) & 0xFF;
            } else {                           /* registrador */
                int r = reg_for(A);
                if (r > 0) {                   /* se já está em eax não precisa */
                    code[idx++] = 0x89;        /* mov r -> eax */
                    code[idx++] = MODRM(r, 0);
                }
            }
            code[idx++] = 0xC3;                /* ret */
            /*  >>>  REMOVIDO o break;  <<<  */
        }

        // IFLEZ
        else if (sscanf(p, "iflez %15s %15s", A, B) == 2) {
            int r      = reg_for(A);
            int target = atoi(B);
            code[idx++] = 0x83;                   // cmp r/m32, imm8
            code[idx++] = MODRM(7, r);            // /7 → CMP r,0
            code[idx++] = 0x00;
            code[idx++] = 0x7E;                   // JLE rel8
            code[idx++] = 0x00;                   // placeholder
            jumps[njump].code_idx   = idx - 1;
            jumps[njump].target_line= target;
            njump++;
        }
        // VAR = VAR OP VAR|$N
        else if (sscanf(p, "%15s = %15s %c %15s", A, B, &op, C) == 4) {
            int rd = reg_for(A), rs = reg_for(B);
            if (rd >= 0 && rs >= 0) {
                code[idx++] = 0x89;               // mov B->A
                code[idx++] = MODRM(rs, rd);
                if (C[0] == '$') {
                    int v = atoi(C+1);
                    code[idx++] = 0x83;           // add/sub imm8
                    code[idx++] = (op=='+'? (0xC0|rd) : (0xE8|rd));
                    code[idx++] = (unsigned char)v;
                } else {
                    int rt = reg_for(C);
                    if (op == '+') {
                        code[idx++] = 0x01;       // add A, C
                        code[idx++] = MODRM(rt, rd);
                    } else if (op == '-') {
                        code[idx++] = 0x29;       // sub A, C
                        code[idx++] = MODRM(rt, rd);
                    } else if (op == '*') {
                        code[idx++] = 0x0F;       // imul A, C
                        code[idx++] = 0xAF;
                        code[idx++] = MODRM(rd, rt);
                    }
                }
            }
        }
        // VAR : VAR|$N
        else if (sscanf(p, "%15s : %15s", A, B) == 2) {
            int rd = reg_for(A);
            if (B[0] == '$') {
                int v = atoi(B+1);
                code[idx++] = 0xB8 | rd;         // mov reg, imm32
                code[idx++] = v & 0xFF;
                code[idx++] = (v>>8) & 0xFF;
                code[idx++] = (v>>16)& 0xFF;
                code[idx++] = (v>>24)& 0xFF;
            } else {
                int rs = reg_for(B);
                code[idx++] = 0x89;             // mov B->A
                code[idx++] = MODRM(rs, rd);
            }
        }
    }

    // 3ª PASSAGEM: corrige offsets dos JLE
    for (int j = 0; j < njump; j++) {
        int loc  = jumps[j].code_idx;
        int line = jumps[j].target_line;
        int dest = -1;
        for (int k = 0; k < nlin; k++) {
            if (linemap[k].line_num == line) {
                dest = linemap[k].code_idx;
                break;
            }
        }
        if (dest >= 0) {
            int rel = dest - (loc + 1);
            code[loc] = (unsigned char)rel;
        }
    }

    return (funcp)code;
}
