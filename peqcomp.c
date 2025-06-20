#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "peqcomp.h"

#define MODRM(src, dst) (0xC0 | ((src) << 3) | (dst))

typedef struct {
    int line_num;
    int code_idx;
} LineMap;

typedef struct {
    int code_idx;
    int target_line;
} JumpPatch;

/* Mapeia variáveis e parâmetros para registradores */
static int reg_for(const char *t) {
    if (t[0]=='v' && isdigit((unsigned char)t[1])) {
        switch (t[1]) {
            case '1': return 0; /* EAX */
            case '2': return 1; /* ECX */
            case '3': return 2; /* EDX */
            case '4': return 3; /* EBX (callee-saved) */
            case '5': return 5; /* EBP (callee-saved) */
            default: return -1;
        }
    }
    if (strcmp(t,"p1")==0) return 7; /* EDI */
    if (strcmp(t,"p2")==0) return 6; /* ESI */
    if (strcmp(t,"p3")==0) return 2; /* EDX */
    return -1;
}

funcp peqcomp(FILE *f, unsigned char *code) {
    char linhas[100][256];
    int nlin = 0;

    while (fgets(linhas[nlin], sizeof linhas[nlin], f)) {
        char *c = strstr(linhas[nlin], "//");
        if (c) *c = '\0';
        nlin++;
    }

    LineMap linemap[100];
    JumpPatch jumps[100];
    int njump = 0;
    int idx = 0;

    /* prólogo: salva EBX e EBP */
    code[idx++] = 0x53; /* push rbx */
    code[idx++] = 0x55; /* push rbp */

    for (int i = 0; i < nlin; i++) {
        char *p = linhas[i];
        while (isspace((unsigned char)*p)) p++;
        if (!*p) continue;

        linemap[i].line_num = i + 1;
        linemap[i].code_idx = idx;

        char A[16], B[16], C[16];
        char op;

        if (sscanf(p, "ret %15s", A) == 1) {
            if (A[0] == '$') {
                int v = atoi(A + 1);
                code[idx++] = 0xB8;
                code[idx++] = v & 0xFF;
                code[idx++] = (v >> 8) & 0xFF;
                code[idx++] = (v >> 16) & 0xFF;
                code[idx++] = (v >> 24) & 0xFF;
            } else {
                int r = reg_for(A);
                if (r > 0) {
                    code[idx++] = 0x89;
                    code[idx++] = MODRM(r, 0);
                }
            }
            code[idx++] = 0x5D; /* pop rbp */
            code[idx++] = 0x5B; /* pop rbx */
            code[idx++] = 0xC3; /* ret */
            continue;
        }

        if (sscanf(p, "iflez %15s %15s", A, B) == 2) {
            int r = reg_for(A);
            int target = atoi(B);
            code[idx++] = 0x83;
            code[idx++] = MODRM(7, r);
            code[idx++] = 0x00;
            code[idx++] = 0x7E;
            code[idx++] = 0x00; /* placeholder */
            jumps[njump].code_idx = idx - 1;
            jumps[njump].target_line = target;
            njump++;
            continue;
        }

        if (sscanf(p, "%15s = %15s %c %15s", A, B, &op, C) == 4) {
            int rd = reg_for(A), rs = reg_for(B);
            if (rd >= 0 && rs >= 0) {
                code[idx++] = 0x89;
                code[idx++] = MODRM(rs, rd);
                if (C[0] == '$') {
                    int v = atoi(C + 1);
                    code[idx++] = 0x83;
                    code[idx++] = (op == '+' ? (0xC0 | rd) : (0xE8 | rd));
                    code[idx++] = (unsigned char)v;
                } else {
                    int rt = reg_for(C);
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

        if (sscanf(p, "%15s : %15s", A, B) == 2) {
            int rd = reg_for(A);
            if (B[0] == '$') {
                int v = atoi(B + 1);
                code[idx++] = 0xB8 | rd;
                code[idx++] = v & 0xFF;
                code[idx++] = (v >> 8) & 0xFF;
                code[idx++] = (v >> 16) & 0xFF;
                code[idx++] = (v >> 24) & 0xFF;
            } else {
                int rs = reg_for(B);
                code[idx++] = 0x89;
                code[idx++] = MODRM(rs, rd);
            }
            continue;
        }
    }

    /* patch de saltos */
    for (int j = 0; j < njump; j++) {
        int loc = jumps[j].code_idx;
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

    return (funcp) code;
}
