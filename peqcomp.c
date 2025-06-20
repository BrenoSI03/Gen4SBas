#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "peqcomp.h"

#define MODRM(src, dst) (0xC0 | (((src)&7)<<3) | ((dst)&7))

static int reg_for(const char *tok) {
    if (tok[0] == 'v' && isdigit((unsigned char)tok[1])) {
        return tok[1] - '1';  
    }
    if (tok[0] == 'p' && tok[1] == '1') {
        return 7;  // p1 em EDI
    }
    return -1;
}

funcp peqcomp(FILE *f, unsigned char *code) {
    char linha[256];
    int idx = 0;

    while (fgets(linha, sizeof(linha), f)) {
        // Remove comentário
        char *cmt = strstr(linha, "//");
        if (cmt) *cmt = '\0';

        // Remove espaços iniciais
        char *p = linha;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) continue;

        // ret
        char var_ret[16];
        if (sscanf(p, "ret %15s", var_ret) == 1) {
            int r = reg_for(var_ret);
            if (r >= 0 && r != 0) {
                code[idx++] = 0x89;
                code[idx++] = MODRM(r, 0);
            }
            code[idx++] = 0xC3;
            break;
        }

        // iflez
        char condvar[16];
        int offset;
        if (sscanf(p, "iflez %15s %d", condvar, &offset) == 2) {
            int rv = reg_for(condvar);
            if (rv >= 0) {
                code[idx++] = 0x83;
                code[idx++] = MODRM(7, rv);  // CMP r/m32, imm8
                code[idx++] = 0x00;         // cmp com 0
                code[idx++] = 0x7E;         // JLE rel8
                code[idx++] = (unsigned char)offset;
            }
            continue;
        }

        // atribuição com operação
        char dst[16], src[16], op, imm[16];
        if (sscanf(p, "%15s = %15s %c %15s", dst, src, &op, imm) == 4) {
            int rd = reg_for(dst);
            int rs = reg_for(src);
            if (rd >= 0 && rs >= 0) {
                if (rd != rs) {
                    code[idx++] = 0x89;
                    code[idx++] = MODRM(rs, rd);
                }
                if (imm[0] == '$') {
                    int val = atoi(imm + 1);
                    code[idx++] = 0x83;
                    if (op == '+') {
                        code[idx++] = 0xC0 | rd;
                    } else if (op == '-') {
                        code[idx++] = 0xE8 | rd;
                    }
                    code[idx++] = (unsigned char)val;
                }
            }
            continue;
        }

        // atribuição direta
        char var1[16], var2[16];
        if (sscanf(p, "%15s : %15s", var1, var2) == 2) {
            int rd = reg_for(var1);
            int rs = reg_for(var2);
            if (rd >= 0 && rs >= 0) {
                code[idx++] = 0x89;
                code[idx++] = MODRM(rs, rd);
            }
            continue;
        }
    }

    return (funcp) code;
}
