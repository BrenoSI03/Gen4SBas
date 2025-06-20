#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "peqcomp.h"

#define MODRM(src,dst) (0xC0 | ((src)<<3) | (dst))

typedef struct {
    char txt[256];   /* linha limpa (sem comentários)   */
    int  addr;       /* offset da linha no código gerado */
} Instr;

/* v1-v4 → EAX,ECX,EDX,EBX (0-3) ; p1 → EDI (7) ; p2 → ESI (6) */
static int reg_for(const char *t){
    if (t[0]=='v' && isdigit((unsigned char)t[1])) return t[1]-'1';
    if (!strcmp(t,"p1")) return 7;
    if (!strcmp(t,"p2")) return 6;
    return -1;
}

funcp peqcomp(FILE *fp, unsigned char *code){
    Instr prog[128];
    int n=0, pc=0;

    /* ---------- 1ª PASSADA ---------- */
    char buf[256];
    while (fgets(buf,sizeof buf,fp)){
        char *c=strstr(buf,"//"); if (c) *c='\0';
        char *p=buf; while (isspace((unsigned char)*p)) p++;
        if (!*p) continue;
        strcpy(prog[n].txt,p);
        prog[n].addr = pc;

        if (!strncmp(p,"iflez",5))                  pc+=5;
        else if (!strncmp(p,"ret",3))
            pc+=(strchr(p,'$')?6:3);
        else if (strchr(p,'=')){
            pc+=(strchr(p,'*')?3:(strchr(p,'$')?4:2));
        } else if (strchr(p,':')){
            pc+=(strchr(p,'$')?5:2);
        }
        n++;
    }

    /* ---------- 2ª PASSADA ---------- */
    pc=0;
    for (int i=0;i<n;i++){
        char *p = prog[i].txt;
        char A[16],B[16],C[16];

        /* ret */
        if (sscanf(p,"ret %15s",A)==1){
            if (A[0]=='$'){
                int v=atoi(A+1);
                code[pc++]=0xB8;
                code[pc++]=v&0xFF; code[pc++]=v>>8;
                code[pc++]=v>>16;  code[pc++]=v>>24;
            } else {
                int r=reg_for(A);
                if (r>0){ code[pc++]=0x89; code[pc++]=MODRM(r,0); }
            }
            code[pc++]=0xC3;
            continue;
        }

        /* iflez */
        int lin;
        if (sscanf(p,"iflez %15s %d",A,&lin)==2){
            int r=reg_for(A);
            code[pc++]=0x83; code[pc++]=MODRM(7,r); code[pc++]=0x00;
            code[pc++]=0x7E;
            int rel = prog[lin-1].addr - (pc+1);
            code[pc++]=(unsigned char)rel;
            continue;
        }

        /* operações = */
        char op;
        if (sscanf(p,"%15s = %15s %c %15s",A,B,&op,C)==4){
            int ra=reg_for(A), rb=reg_for(B);
            code[pc++]=0x89; code[pc++]=MODRM(rb,ra);

            if (C[0]=='$'){
                int v=atoi(C+1);
                code[pc++]=0x83;
                code[pc++]=(op=='+'?(0xC0|ra):(op=='-'?(0xE8|ra):0));
                code[pc++]=(unsigned char)v;
            } else {
                int rc=reg_for(C);
                if (op=='+'){ code[pc++]=0x01; code[pc++]=MODRM(rc,ra); }
                else if(op=='-'){ code[pc++]=0x29; code[pc++]=MODRM(rc,ra); }
                else if(op=='*'){
                    code[pc++]=0x0F; code[pc++]=0xAF; code[pc++]=MODRM(rc,ra);
                }
            }
            continue;
        }

        /* atribuições : */
        if (sscanf(p,"%15s : %15s",A,B)==2){
            int ra=reg_for(A);
            if (B[0]=='$'){
                int v=atoi(B+1);
                code[pc++]=0xB8|ra;
                code[pc++]=v&0xFF; code[pc++]=v>>8;
                code[pc++]=v>>16;  code[pc++]=v>>24;
            } else {
                int rb=reg_for(B);
                code[pc++]=0x89; code[pc++]=MODRM(rb,ra);
            }
            continue;
        }
    }
    return (funcp)code;
}
