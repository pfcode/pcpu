#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 1
#define T_BYTE 0x01
#define T_TEXT 0x02

/* Opcodes */
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

#define VAR 0xAA /* That is not an OPCODE, but an assembler internal symbol! */

bool startsWith(char *s, const char *p){
	int i = 0;
	while(p[i] && s[i]){
		if(p[i] == '\0') return true;
		if(s[i] != p[i]) return false;
		i++;
	}
	return true;
}

typedef struct{
	unsigned int address;
	unsigned char value;
} t_define;

typedef struct{
	unsigned int value;
	t_define *define;
	void *variable;
} t_argument;

typedef struct{
	unsigned char opcode;
	int args;
	t_argument *arguments;
} t_instruction;

typedef struct{
	char name[64];
	unsigned int address;
	t_instruction *pseudoOpcode;
} t_variable;

t_define **defines;
int definesAmount;
t_instruction **instructions;
t_variable **variables;
int varsAmount;

unsigned char *output;
int offset = 0;

void pushByte(unsigned char v){
	output = (unsigned char *) realloc((void *)output, offset + 1);
	output[offset] = v;
	offset++;
}

int parseSymbol(char *s){
	if(strcmp(s, "AX") == 0) return 0xfffa;
	else if(strcmp(s, "BX") == 0) return 0xfffb;
	else if(strcmp(s, "CX") == 0) return 0xfffc;
	else if(strcmp(s, "DX") == 0) return 0xfffd;
	else if(strcmp(s, "CF") == 0) return 0xfffe;
	else if(strcmp(s, "NF") == 0) return 0xffff;
	else return 0;
}

void parseVariable(t_argument *arg, char *s){
	if(!arg) return;
	for(int i = 0; i < varsAmount; i++){
		if(variables && variables[i] && strcmp(variables[i]->name, s) == 0){
			arg->value = variables[i]->address;
			arg->define = NULL;
			arg->variable = (void *) variables[i];
			break;
		}
	}
}

t_instruction *parseLine(char *s){
	t_instruction* i = (t_instruction *) malloc(sizeof(t_instruction));
	// Example line: MOV 0020 #0000
	// '#' prefix is a constant value, not a pointer

	char opcode[16];
	char args[8][16];
	int startArg = 0;
	t_variable* var;

	int parsedElements = sscanf(s, "%s %s %s %s %s %s %s %s %s", opcode, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
	// MOV instruction, 2 16-bit arguments
	if(startsWith(s, "MOV")){ i->opcode = MOV; i->args = 2; }
	else if(startsWith(s, "ADD")){ i->opcode = ADD; i->args = 2; }
	else if(startsWith(s, "SUB")){ i->opcode = SUB; i->args = 2; }
	else if(startsWith(s, "MUL")){ i->opcode = MUL; i->args = 2; }
	else if(startsWith(s, "DIV")){ i->opcode = DIV; i->args = 2; }
	else if(startsWith(s, "JMP")){ i->opcode = JMP; i->args = 1; }
	else if(startsWith(s, "CMP")){ i->opcode = CMP; i->args = 2; }
	else if(startsWith(s, "JE")){ i->opcode = JE; i->args = 1; }
	else if(startsWith(s, "JNE")){ i->opcode = JNE; i->args = 1; }
	else if(startsWith(s, "JG")){ i->opcode = JG; i->args = 1; }
	else if(startsWith(s, "JL")){ i->opcode = JL; i->args = 1; }
	else if(startsWith(s, "INT")){ i->opcode = INT; i->args = 0; }
	else if(startsWith(s, "MOVP")){ i->opcode = MOVP; i->args = 2; }
	else if(startsWith(s, "VAR")){
		i->opcode = VAR;
		i->args = 2;
		varsAmount++;
		if(!variables) (t_variable **) malloc(sizeof(t_variable*));
		variables = (t_variable **) realloc((void *) variables, sizeof(t_variable *)*varsAmount);
		variables[varsAmount - 1] = (t_variable *) malloc(sizeof(t_variable));
		var = variables[varsAmount - 1];
		sscanf(args[0], ".%s", var->name);
		startArg = 1;
	}
	// Parse 2 arguments, space/tab delimited
	// If starts with a '#', create define
	i->arguments = (t_argument *) malloc(sizeof(t_argument)*i->args);
	for(int j = startArg; j < i->args; j++){
		i->arguments[j].define = NULL;
		int n = sscanf(args[j], "0x%x", &i->arguments[j].value);
		if(n == 0){ // argument is not an address
			// Try to parse as a define
			int constant = 0;
			char symbol[64];
			n = sscanf(args[j], "#0x%x", &constant); // Try to parse as hex number
			if(n == 0) n = sscanf(args[j], "#'%c'", &constant); // Try to parse as ASCII char
			if(n == 0) n = sscanf(args[j], "#%d", &constant); // Try to parse as decimal number
			if(n == 0){ // argument is not a define
				n = sscanf(args[j], ".%s", symbol);
				if(n == 0){
					// Try to parse as a symbol (register, flag, etc)
					n = sscanf(args[j], "%s", symbol);
					if(n > 0){
						// On symbol read
						i->arguments[j].value = parseSymbol(symbol);
					}
				} else{
					// Try to parse as a variable
					parseVariable(&i->arguments[j], symbol);
				}
			} else{
				// Create new define
				definesAmount++;
				if(!defines) (t_define **) malloc(sizeof(t_define *)*definesAmount);
				defines = (t_define **) realloc((void *) defines, sizeof(t_define *)*definesAmount);
				defines[definesAmount - 1] = (t_define *) malloc(sizeof(t_define));
				i->arguments[j].define = defines[definesAmount - 1];
				i->arguments[j].define->value = constant;
			}
		}
	}
	if(i->opcode == VAR) var->pseudoOpcode = i;
	// Other instructions here
	return i;
}

bool parseCode(char *s){
	char *line = strtok(s, "\n");
	int lines = 0;
	int instAmount = 0;
	while(line){
		lines++;
		if(DEBUG) printf("[PROCESSING LINE] %s\n", line);
		if(line[0] != ';' && strlen(line) > 0){
			t_instruction* i = parseLine(line);
			if(!i){
				printf("Error: Cannot parse line %d\n", lines);
				return false;
			}
			if(i->opcode != VAR){
				instAmount++;
				instructions = (t_instruction **) realloc((void *) instructions, sizeof(t_instruction) * instAmount);
				instructions[instAmount - 1] = i;
			}
		}
		line = strtok(NULL, "\n");
	}
	return true;
}

/* Size of executive binary section:  */
int calcExecSize(){
	int size = 0, j = 0;
	while(instructions && instructions[j]){
		t_instruction* i = instructions[j];
		size += 1 + i->args*2;
		j++;
	}
	return size;
}

/* Size of constants binary section: */
int calcConstSize(){
	int size = 0, j = 0;
	while(defines && defines[j]){
		size+=1;
		j++;
	}
	return size;
}

unsigned char *createBinary(){
	unsigned char *output;
	int j = 0;
	int currentSize = 0;
	int execSize = calcExecSize();
	int constSize = calcConstSize();
	int constOffset = execSize + 1; // Last instruction should be NULL
	/* Allocate adresses for defines */
	for(int i = 0; i < definesAmount; i++){
		defines[i]->address = constOffset + i;
	}
	/* Apply addresses to the variables */
	for(int i = 0; i < varsAmount; i++){
		t_variable *var = variables[i];
		t_instruction *pseudoOpcode = var->pseudoOpcode;
		if(pseudoOpcode->opcode == VAR){
			printf("%d", pseudoOpcode->arguments[1].define->address);
			if(pseudoOpcode->arguments[1].define){
				var->address = pseudoOpcode->arguments[1].define->address;
			} else{
				var->address = pseudoOpcode->arguments[1].value;
			}
		}
	}
	while(instructions && instructions[j]){
		t_instruction* i = instructions[j];
		// Iterate through arguments and set constants addresses to the values
		for(int k = 0; k < i->args; k++){
			if(i->arguments[k].variable){
				i->arguments[k].value = ((t_variable*)(i->arguments[k].variable))->address;
			}
			else if(i->arguments[k].define){
				i->arguments[k].value = i->arguments[k].define->address;
			}
		}
		// Push bytes to the output
		output = (unsigned char *) realloc((void *) output, currentSize + sizeof(unsigned char) * (i->args * 2 + 1));
		output[currentSize] = i->opcode;
		currentSize++;
		for(int k=0; k < i->args; k++){
			output[currentSize + k*2] = (i->arguments[k].value >> 8);
			output[currentSize + k*2 + 1] = (unsigned char)(i->arguments[k].value);
		}
		currentSize += i->args*2;
		j++;
	}
	currentSize++; // Last instruction - NULL
	output = (unsigned char *) realloc((void *) output, currentSize);
	output[currentSize - 1] = 0;
	for(int i = 0; i < definesAmount; i++){
		output = (unsigned char *) realloc((void *) output, currentSize + 1);
		output[currentSize] = defines[i]->value;
		currentSize++;
	}
	return output;
}

void memmap(int start, int end, unsigned char* output){
        printf("[MEMORY MAP]\n");
        for(int i = start; i < end; i+=8){
                printf(" [%04x:%04x]\t", i, i+7);
                for(int j = i; j < i + 8; j++){
			if(output[j]) printf("%02x ", output[j]);
			else printf(":: ");
                }
                printf("\n");
        }
        printf("\n");
}

int main(int argc, char *argv[]){
	if(argc <= 1){
		printf("pCPU Assembler\n");
		printf("Usage: %s [source] [binary]\n", argv[0]);
		return 0;
	}
	FILE *fp = fopen(argv[1], "r");
	if(!fp){
		printf("Error: Cannot open file: %s\n", argv[1]);
		return 1;
	}
	int size = 0;
	char *s = (char *) malloc(1);
	char c;
	c = fgetc(fp);
	while(c != EOF){
		size++;
		s = (char*)realloc(s, size);
		s[size - 1] = c;
		c = fgetc(fp);
	}

	if(!parseCode(s)){
		printf("Fatal: Code parsing failed.\n");
	}
	unsigned char* binary = createBinary();

	memmap(0, 0x50, binary);

	// Create binary file
	FILE *fp2 = fopen(argv[2], "wb+");
	int d = calcExecSize() + calcConstSize() + 1;
	for(int i = 0; i < d; i++){
		fputc(binary[i], fp2);
	}

	printf("[STATS]\tExec stack: %d bytes\tConstants stack: %d bytes\n", calcExecSize(), calcConstSize());

	fclose(fp);
	fclose(fp2);
}
