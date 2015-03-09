#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAMSIZE 65536 /* 64 kilobytes */
#define ROMSIZE 64

#define DEBUG 0

/* Reserved memory pointers */
#define AX 0xFFFA
#define BX 0xFFFB
#define CX 0xFFFC
#define DX 0xFFFD
#define CF 0xFFFE
#define NF 0xFFFF

/* Assembler instructions */
#define NUL 0x00
#define MOV 0x01
#define ADD 0x02
#define SUB 0x03
#define MUL 0x04
#define DIV 0x05
#define JMP 0x06
#define CMP 0x07
#define JE 0x08
#define JNE 0x09
#define JG 0x0A
#define JL 0x0B
#define INT 0x0C
#define MOVP 0x0D

const unsigned char rom[ROMSIZE] = {
	MOV, 0xFF, 0xFD, 0x00, 0x31, MOVP, 0xFF, 0xFB,
	0xFF, 0xFD, MOV, 0xFF, 0xFF, 0x00, 0x32, MOV,
	0xFF, 0xFA, 0x00, 0x33, INT, ADD, 0xFF, 0xFD,
	0x00, 0x32, CMP, 0xFF, 0xFD, 0x00, 0x30, JNE,
	0x00, 0x05, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x3F, 0x34, 0x01, 0x03, 'H', 'E', 'L', 'L',
	'O', ' ', 'W', 'O', 'R', 'L', 'D', 0x0
}; /* 64 bytes */

unsigned char *ram;

unsigned char getRAM(int offset){ /* 2-byte offset */
	if(offset >= 0 && offset < RAMSIZE) return ram[offset];
	else return 0x00;
}

void setRAM(int offset, unsigned char value){ /* 2-byte offset, 1-byte value */
	if(offset >= 0 && offset < RAMSIZE){
		ram[offset] = value;
	}
}

char *opcodeName(int offset){
	char *tmp = (char *) malloc(sizeof(char)*64);
	switch(getRAM(offset)){
		default: sprintf(tmp, "%02x", getRAM(offset)); break;
		case NUL: sprintf(tmp, "NUL"); break;
		case MOV: sprintf(tmp, "MOV %04x %04x", (getRAM(offset+1)) << 8 + getRAM(offset+2), (getRAM(offset+3) << 8) + getRAM(offset+4)); break;
		case ADD: sprintf(tmp, "ADD"); break;
		case SUB: sprintf(tmp, "SUB"); break;
		case MUL: sprintf(tmp, "MUL"); break;
		case DIV: sprintf(tmp, "DIV"); break;
		case JMP: sprintf(tmp, "JMP"); break;
		case CMP: sprintf(tmp, "CMP"); break;
		case JE: sprintf(tmp, "JE"); break;
		case JNE: sprintf(tmp, "JNE"); break;
		case JG: sprintf(tmp, "JG"); break;
		case JL: sprintf(tmp, "JL"); break;
		case INT: sprintf(tmp, "INT"); break;
		case MOVP: sprintf(tmp, "MOVP"); break;
	}
	return tmp;
}

void int_video(){
	switch(getRAM(AX)){
		default: break;
		case 0x01: /* Initialize textmode video - 0xB000 pointer */
			for(int i=0xB000; i<0xB7D0; i++) setRAM(i, 0x00);
			break;
		case 0x02: /* Deinitialize textmode video */
			break;
		case 0x03: /* Put char on monitor */ // DEBUG
			printf("%c", getRAM(BX));
			break;
	}
}

void interrupt(){
	switch(getRAM(NF)){
		default: break;
		case 0x01: /* Video */
			int_video();
			break;
	}
}

void execute(int offset){
	bool running = true;
	int step = 0;
	while(running){
//		if(step % 10 == 9) return;
		step++;
		unsigned char v1, v2;
		if(DEBUG) printf("[STEP %d | OFFSET %04x]\t%s\n", step, offset, opcodeName(offset));
		switch( getRAM(offset) ){
			default: running = false; break; /* Instruction: 0x00 NULL | Args: none*/
			case 0x01: /* Instruction: 0x01 MOV | Args: 2-bit destination, 2-bit source | Copies memory */
				setRAM( (getRAM(offset+1) << 8) + getRAM(offset+2), getRAM((getRAM(offset+3) << 8) + getRAM(offset+4)) );
				offset+=5;
				break;
			case 0x02: /* Instruction: 0x02 ADD | Args: 2-bit destination, 2-bit source | Adds a value */
				setRAM( (getRAM(offset+1) << 8) + getRAM(offset+2), getRAM((getRAM(offset+1) << 8) + getRAM(offset+2)) + getRAM((getRAM(offset+3) << 8) + getRAM(offset+4)) );
				offset+=5;
				break;
			case 0x03: /* Instruction: 0x03 SUB | Args: 2-bit destination, 2-bit source | Substracts a value */
				setRAM( (getRAM(offset+1) << 8) + getRAM(offset+2), getRAM((getRAM(offset+1) << 8) + getRAM(offset+2)) - getRAM((getRAM(offset+3) << 8) + getRAM(offset+4)) );
				offset+=5;
				break;
			case 0x04: /* Instruction: 0x04 MUL | Args: 2-bit destination, 2-bit source | Multiplies a value */
				setRAM( (getRAM(offset+1) << 8) + getRAM(offset+2), getRAM((getRAM(offset+1) << 8) + getRAM(offset+2)) * getRAM((getRAM(offset+3) << 8) + getRAM(offset+4)) );
				offset+=5;
				break;
			case 0x05: /* Instruction: 0x05 DIV | Args: 2-bit destination, 2-bit source | Divides a value */
				setRAM( (getRAM(offset+1) << 8) + getRAM(offset+2), getRAM((getRAM(offset+1) << 8) + getRAM(offset+2)) / getRAM((getRAM(offset+3) << 8) + getRAM(offset+4)) );
				offset+=5;
				break;
			case 0x06: /* Instruction: 0x06 JMP | Args: 2-bit destination | Jumps execution offset to specified value */
				offset = (getRAM(offset+1) << 8) + getRAM(offset + 2);
				break;
			case 0x07: /* Instruction: 0x07 CMP | Args: 2-bit pointerA, 2-bit pointerB | Compares two values in memory */
				v1 = getRAM((getRAM(offset+1) << 8) + getRAM(offset+2));
				v2 = getRAM((getRAM(offset+3) << 8) + getRAM(offset+4));
				if(v1 == v2) setRAM(CF, 0x01);
				else if(v1 > v2) setRAM(CF, 0x02);
				else if(v1 < v2) setRAM(CF, 0x03);
				else setRAM(CF, 0x00);
				offset+=5;
				break;
			case 0x08: /* Instruction: 0x08 JE | Args: 2-bit pointer | Jumps if equal */
				if(getRAM(CF) == 0x01) offset = (getRAM(offset+1) << 8) + getRAM(offset+2);
				else offset+=3;
				break;
			case 0x09: /* Instruction: 0x09 JNE | Args: 2-bit pointer | Jumps if not equal */
				if(getRAM(CF) != 0x01) offset = (getRAM(offset+1) << 8) + getRAM(offset+2);
				else offset+=3;
				break;
			case JG: /* Args: 2-bit pointer | Jumps if value of pointerA > value of pointerB */
				if(getRAM(CF) == 0x02) offset = (getRAM(offset+1) << 8) + getRAM(offset+2);
				else offset+=3;
				break;
			case JL: /* Args: 2-bit pointer | Jumps if value of pointerB < value of pointerA */
				if(getRAM(CF) == 0x03) offset = (getRAM(offset+1) << 8) + getRAM(offset+2);
				else offset+=3;
				break;
			case INT: /* No args, NF needs to be set | Executes interrupt */
				interrupt();
				offset+=1;
				break;
			case MOVP: /* Args: 2-bit destination, 2-bit pointer to pointer [xD] | Moves pointer */
				int dest = (getRAM(offset+1) << 8) + getRAM(offset+2);
				int p = (getRAM(offset+3) << 8) + getRAM(offset+4);
				int source = getRAM(getRAM(p));
				setRAM(dest, source);
				offset+=5;
				break;
		}
	}
	printf("\n[INFO] Stopping code execution at %04x. Steps done: %d\n", offset, step);
}

void memmap(int start, int end){
	printf("[MEMORY MAP]\n");
	for(int i = start; i < end; i+=8){
		printf(" [%04x:%04x]\t", i, i+7);
		for(int j = i; j < i + 8; j++){
			printf("%02x ", getRAM(j));
		}
		printf("\n");
	}
	printf("\n");
}

void regdump(){
	printf("[REGISTERS]\n");
	printf("AX: %02x | BX: %02x | CX: %02x\n", getRAM(AX), getRAM(BX), getRAM(CX));
	printf("DX: %02x | CF: %02x | NF: %02x\n", getRAM(DX), getRAM(CF), getRAM(NF));
}

int main(int argc, char *argv[]){
	/* Load ROM to RAM */
	ram = (unsigned char *)malloc(RAMSIZE);
	ram = (unsigned char *)memset((void *) ram, 0, RAMSIZE);
	if(argc > 1){
		printf("[INFO] Loading ROM from file: %s\n", argv[1]);
		FILE *fp = fopen(argv[1], "rb+");
		int c = fgetc(fp);
		int i = 0;
		while(c != EOF){
			ram[i] = c;
			i++;
			c = fgetc(fp);
		}
		fclose(fp);
	} else{
		printf("[INFO] Loading ROM (hardcoded)\n");
		memcpy((void *)ram, rom, ROMSIZE);
	}

	printf("[INFO] Starting code at 0x00\n");
	/* Execute code at 0x00 RAM offset */
	execute(0x00);

	memmap(0x00, 0x3F);
	regdump();

	printf("\n");
}


