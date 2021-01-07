#include "sim_pipe.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

//#define DEBUG

using namespace std;

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "IF_ID", "ID_EX", "EXE_MEM", "MEM_WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP"};
unsigned gp_register[NUM_GP_REGISTERS] = {UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
	UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
	UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
	UNDEFINED, UNDEFINED};
/* converts integer into array of unsigned char - little indian */
inline void int2char(unsigned value, unsigned char *buffer){
	memcpy(buffer, &value, sizeof value);
}
/* converts array of char into integer - little indian */
inline unsigned char2int(unsigned char *buffer){
	unsigned d;
	memcpy(&d, buffer, sizeof d);
	return d;
}
/* implements the ALU operations */
unsigned alu(unsigned opcode, unsigned a, unsigned b, unsigned imm, unsigned npc){
	switch(opcode){
		case ADD:
			return (a+b);
		case ADDI:
			return(a+imm);
		case SUB:
			return(a-b);
		case SUBI:
			return(a-imm);
		case XOR:
			return(a ^ b);
		case LW:
		case SW:
			return(a + imm);
		case BEQZ:
		case BNEZ:
		case BGTZ:
		case BGEZ:
		case BLTZ:
		case BLEZ:
		case JUMP:
			return(npc+imm);
		default: return (-1);
	}
}
/* loads the assembly program in file "filename" in instruction memory at the specified address */
void sim_pipe::load_program(const char *filename, unsigned base_address){

  /* initializing the base instruction address */
	instr_base_address = base_address;
	pc = base_address;
	reset();

  /* creating a map with the valid opcodes and with the valid labels */
  map<string, opcode_t> opcodes; //for opcodes
  map<string, unsigned> labels;  //for branches
  for (int i=0; i<NUM_OPCODES; i++){
		opcodes[string(instr_names[i])]=(opcode_t)i;
	}

  /* opening the assembly file */
  ifstream fin(filename, ios::in | ios::binary);
  if (!fin.is_open()){
    cerr << "error: open file " << filename << " failed!" << endl;
    exit(-1);
  }

  /* parsing the assembly file line by line */
  string line;
  unsigned instruction_nr = 0;

  while (getline(fin,line)){
		// set the instruction field
		char *str = const_cast<char*>(line.c_str());
	  // tokenize the instruction
		char *token = strtok (str," \t");
		map<string, opcode_t>::iterator search = opcodes.find(token);
	  if (search == opcodes.end()){
			// this is a label for a branch - extract it and save it in the labels map
			string label = string(token).substr(0, string(token).length() - 1);
			labels[label]=instruction_nr;
	    // move to next token, which must be the instruction opcode
			token = strtok (NULL, " \t");
			search = opcodes.find(token);
			if (search == opcodes.end()){
				cout << "ERROR: invalid opcode: " << token << " !" << endl;
			}
		}

		instr_memory[instruction_nr].opcode = search->second;
		//reading remaining parameters
		char *par1;
		char *par2;
		char *par3;
		switch(instr_memory[instruction_nr].opcode){
			case ADD:
			case SUB:
			case XOR:
				par1 = strtok (NULL, " \t");
				par2 = strtok (NULL, " \t");
				par3 = strtok (NULL, " \t");
				instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
				instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
				instr_memory[instruction_nr].src2 = atoi(strtok(par3, "R"));
				break;

			case ADDI:
			case SUBI:
				par1 = strtok (NULL, " \t");
				par2 = strtok (NULL, " \t");
				par3 = strtok (NULL, " \t");
				instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
				instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
				instr_memory[instruction_nr].immediate = strtoul (par3, NULL, 0);
				break;

			case LW:
				par1 = strtok (NULL, " \t");
				par2 = strtok (NULL, " \t");
				instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
				instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
				instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
				break;

			case SW:
				par1 = strtok (NULL, " \t");
				par2 = strtok (NULL, " \t");
				instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
				instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
				instr_memory[instruction_nr].src2 = atoi(strtok(NULL, "R"));
				break;

			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
				par1 = strtok (NULL, " \t");
				par2 = strtok (NULL, " \t");
				instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
				instr_memory[instruction_nr].label = par2;
				break;

			case JUMP:
				par2 = strtok (NULL, " \t");
				instr_memory[instruction_nr].label = par2;
			default: break;
		}
		/* increment instruction number before moving to next line */
		instruction_nr++;
	}
   //reconstructing the labels of the branch operations
   int i = 0;
   while(true){
   	instruction_t instr = instr_memory[i];
		if (instr.opcode == EOP) break;
		if (instr.opcode == BLTZ || instr.opcode == BNEZ || instr.opcode == BGTZ || instr.opcode == BEQZ || instr.opcode == BGEZ || instr.opcode == BLEZ || instr.opcode == JUMP){
			instr_memory[i].immediate = (labels[instr.label] - i - 1) << 2;
		}
    i++;
  }
}
/* writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness) */
void sim_pipe::write_memory(unsigned address,unsigned value){
	int2char(value,data_memory+address);
}
/* prints the content of the data memory within the specified address range */
void sim_pipe::print_memory(unsigned start_address, unsigned end_address){
	cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	for (unsigned i=start_address; i<end_address; i++){
		if (i%4 == 0){
			cout << "0x" << hex << setw(8) << setfill('0') << i << ": ";
		}
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3){
			cout << endl;
		}
	}
}
/* prints the values of the registers */
void sim_pipe::print_registers(){
  cout << "Special purpose registers:" << endl;
  unsigned i, s;
  for (s=0; s<NUM_STAGES; s++){
    cout << "Stage: " << stage_names[s] << endl;
    for (i=0; i< NUM_SP_REGISTERS; i++){
      if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED){
				cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
			}
		}
	}
  cout << "General purpose registers:" << endl;
  for (i=0; i< NUM_GP_REGISTERS; i++){
    if (get_gp_register(i)!=(int)UNDEFINED){
			cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
		}
	}
}
/* initializes the pipeline simulator */
sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
	data_memory_size = mem_size;
	data_memory_latency = mem_latency;
	data_memory = new unsigned char[data_memory_size];
	reset();
}
/* deallocates the pipeline simulator */
sim_pipe::~sim_pipe(){
	delete [] data_memory;
}


/* body of the simulator */
void sim_pipe::run(unsigned cycles){
	while(ALWAYS){
		if(pipeline_register[MEM_WB].IR.opcode != EOP){
			clock_cycle_count++;
			WB_stage();
			MEM_stage();
			EXE_stage();
			ID_stage();
			IF_stage();
			one_time = 0;
		}
		else{
			break;
		}
		cycles--;
		if(cycles == 0){
			break;
		}
	}
}

void sim_pipe::IF_stage(void){
		pipeline_register[IF_ID].IR = instr_memory[(pc-instr_base_address)>>2];
		if(pipeline_register[IF_ID].IR.opcode != EOP){
			if((pipeline_register[EXE_MEM].COND == 0) | (pipeline_register[EXE_MEM].COND == UNDEFINED)){
				pipeline_register[IF_ID].NPC = pc + 4;
				pc = pipeline_register[IF_ID].NPC;
			}
			else{
				pipeline_register[IF_ID].NPC = pipeline_register[EXE_MEM].ALU_OUTPUT;
				pc = pipeline_register[EXE_MEM].ALU_OUTPUT;
			}
		}
	}
void sim_pipe::ID_stage(void){
	/*if(pipeline_register[ID_EXE].IR.dest != UNDEFINED){
		if((pipeline_register[IF_ID].IR.src1 == pipeline_register[EXE_MEM].IR.dest) | (pipeline_register[IF_ID].IR.src2 == pipeline_register[EXE_MEM].IR.dest)){
			stalls= stalls + 2;
			one_time = 1;
			tmp = pipeline_register[IF_ID];
			set_tmp = 1;
			pipeline_register[IF_ID].IR.opcode = EOP;
			IF_stalls = 2;
		}
		else{
			if((pipeline_register[IF_ID].IR.src1 == pipeline_register[MEM_WB].IR.dest) | (pipeline_register[IF_ID].IR.src2 == pipeline_register[MEM_WB].IR.dest)){
				stalls++;
				one_time = 1;
				tmp = pipeline_register[IF_ID];
				set_tmp = 1;
				pipeline_register[IF_ID].IR.opcode = EOP;
				IF_stalls = 1;
			}
		}
	}
		if((set_tmp == 1) & (IF_stalls == 0)){
			pipeline_register[IF_ID] = tmp;
		}*/
	switch(pipeline_register[IF_ID].IR.opcode){
				case ADD:
				case SUB:
				case XOR:
					pipeline_register[ID_EXE].A = gp_register[pipeline_register[IF_ID].IR.src1];
					pipeline_register[ID_EXE].B = gp_register[pipeline_register[IF_ID].IR.src2];
					pipeline_register[ID_EXE].IMM = UNDEFINED;
					break;

				case ADDI:
				case SUBI:
					pipeline_register[ID_EXE].A = gp_register[pipeline_register[IF_ID].IR.src1];
					pipeline_register[ID_EXE].B = UNDEFINED;
					pipeline_register[ID_EXE].IMM = pipeline_register[IF_ID].IR.immediate;
					break;

				case LW:
					pipeline_register[ID_EXE].A = gp_register[pipeline_register[IF_ID].IR.src1];
					pipeline_register[ID_EXE].B = UNDEFINED;
					pipeline_register[ID_EXE].IMM = pipeline_register[IF_ID].IR.immediate;
					break;

				case SW:
					pipeline_register[ID_EXE].A = gp_register[pipeline_register[IF_ID].IR.src2];
					pipeline_register[ID_EXE].B = gp_register[pipeline_register[IF_ID].IR.src1];
					pipeline_register[ID_EXE].IMM = pipeline_register[IF_ID].IR.immediate;
					pipeline_register[ID_EXE].IR.dest = UNDEFINED;
					break;

				case BEQZ:
				case BNEZ:
				case BLTZ:
				case BGTZ:
				case BLEZ:
				case BGEZ:
					break;

				case JUMP:
					break;
				case EOP:
				case NOP:
					pipeline_register[IF_ID].IR.dest = UNDEFINED;
					pipeline_register[IF_ID].IR.src1 = UNDEFINED;
					pipeline_register[IF_ID].IR.src2 = UNDEFINED;
					pipeline_register[IF_ID].IR.immediate = UNDEFINED;
					pipeline_register[IF_ID].IMM = UNDEFINED;
					pipeline_register[IF_ID].A = UNDEFINED;
					pipeline_register[IF_ID].B = UNDEFINED;
					pipeline_register[IF_ID].IMM = UNDEFINED;
					pipeline_register[IF_ID].ALU_OUTPUT= UNDEFINED;
					pipeline_register[IF_ID].LMD = UNDEFINED;
				case NONE:
					break;
				default: break;
		}
	if(one_time == 0){
	pipeline_register[ID_EXE].NPC = pipeline_register[IF_ID].NPC;
	pipeline_register[ID_EXE].IR = pipeline_register[IF_ID].IR;
}
}

void sim_pipe::EXE_stage(void){
		switch(pipeline_register[ID_EXE].IR.opcode){
			case ADD:
			case SUB:
			case XOR:
			case ADDI:
			case SUBI:
			case LW:
			case SW:
				pipeline_register[EXE_MEM].ALU_OUTPUT = alu(pipeline_register[ID_EXE].IR.opcode,
																										pipeline_register[ID_EXE].A,
																										pipeline_register[ID_EXE].B,
																										pipeline_register[ID_EXE].IMM,
																										pipeline_register[ID_EXE].NPC);
				pipeline_register[EXE_MEM].COND = 0;
				break;

			case BEQZ:
			case BNEZ:
			case BLTZ:
			case BGTZ:
			case BLEZ:
			case BGEZ:
				break;

			case JUMP:
				break;
			case EOP:
			case NOP:
				pipeline_register[ID_EXE].IR.dest = UNDEFINED;
				pipeline_register[ID_EXE].IR.src1 = UNDEFINED;
				pipeline_register[ID_EXE].IR.src2 = UNDEFINED;
				pipeline_register[ID_EXE].IR.immediate = UNDEFINED;
				pipeline_register[ID_EXE].IMM = UNDEFINED;
				pipeline_register[ID_EXE].A = UNDEFINED;
				pipeline_register[ID_EXE].B = UNDEFINED;
				pipeline_register[ID_EXE].IMM = UNDEFINED;
				pipeline_register[ID_EXE].ALU_OUTPUT= UNDEFINED;
			//	pipeline_register[EXE_MEM].ALU_OUTPUT = pipeline_register[ID_EXE].ALU_OUTPUT;
				pipeline_register[ID_EXE].LMD = UNDEFINED;
			case NONE:
				break;
		}
	pipeline_register[EXE_MEM].B = pipeline_register[ID_EXE].B;
	pipeline_register[EXE_MEM].IR = pipeline_register[ID_EXE].IR;
}
void sim_pipe::MEM_stage(void){
/*	if(IF_stalls == 1){
		pipeline_register[EXE_MEM].IR.opcode = EOP;
	}*/
	if((pipeline_register[EXE_MEM].IR.opcode != NONE) | (pipeline_register[MEM_WB].IR.dest != UNDEFINED)){
		/*if(pipeline_register[EXE_MEM].IR.opcode != 0){
			pipeline_register[EXE_MEM].IR.opcode = EOP;
		}*/
		switch(pipeline_register[EXE_MEM].IR.opcode){
			case LW:
			if(pipeline_register[EXE_MEM].ALU_OUTPUT != UNDEFINED){
					pipeline_register[MEM_WB].LMD = char2int(data_memory+pipeline_register[EXE_MEM].ALU_OUTPUT);
				}
				break;
			case SW:
				write_memory(pipeline_register[EXE_MEM].ALU_OUTPUT,pipeline_register[EXE_MEM].B);
				pipeline_register[MEM_WB].LMD = UNDEFINED;
				break;
			case EOP:
			case NOP:
				pipeline_register[EXE_MEM].IR.dest = UNDEFINED;
				pipeline_register[EXE_MEM].IR.src1 = UNDEFINED;
				pipeline_register[EXE_MEM].IR.src2 = UNDEFINED;
				pipeline_register[EXE_MEM].IR.immediate = UNDEFINED;
				pipeline_register[EXE_MEM].A = UNDEFINED;
				pipeline_register[EXE_MEM].B = UNDEFINED;
				pipeline_register[EXE_MEM].IMM = UNDEFINED;
				pipeline_register[EXE_MEM].ALU_OUTPUT= UNDEFINED;
			default:
				pipeline_register[MEM_WB].LMD = UNDEFINED;
				break;
		}
	}
			pipeline_register[MEM_WB].ALU_OUTPUT = pipeline_register[EXE_MEM].ALU_OUTPUT;
		pipeline_register[MEM_WB].IR = pipeline_register[EXE_MEM].IR;
}
void sim_pipe::WB_stage(void){
	switch(pipeline_register[MEM_WB].IR.opcode){
		case ADD:
		case SUB:
		case XOR:
		case ADDI:
		case SUBI:
			gp_register[pipeline_register[MEM_WB].IR.dest] = pipeline_register[MEM_WB].ALU_OUTPUT;
			instr_exec++;
			break;

		case LW:
			gp_register[pipeline_register[MEM_WB].IR.dest] = pipeline_register[MEM_WB].LMD;
			instr_exec++;
			break;

		case SW:
			instr_exec++;
			break;

		case BEQZ:
		case BNEZ:
		case BLTZ:
		case BGTZ:
		case BLEZ:
		case BGEZ:
			break;

		case JUMP:
			break;
		case EOP:
		case NOP:
			pipeline_register[MEM_WB].IR.dest = UNDEFINED;
			pipeline_register[MEM_WB].IR.src1 = UNDEFINED;
			pipeline_register[MEM_WB].IR.src2 = UNDEFINED;
			pipeline_register[MEM_WB].IR.immediate = UNDEFINED;
			pipeline_register[MEM_WB].A = UNDEFINED;
			pipeline_register[MEM_WB].B = UNDEFINED;
			pipeline_register[MEM_WB].IMM = UNDEFINED;
			pipeline_register[MEM_WB].ALU_OUTPUT= UNDEFINED;
			break;
		case NONE:
			break;
	}
}

/* reset the state of the pipeline simulator */
void sim_pipe::reset(){
	for(int k=0; k<NUM_GP_REGISTERS; k++){
		gp_register[k] = UNDEFINED;
	}

	for(int j=0; j<NUM_STAGES-1; j++){
		pipeline_register[j].NPC = UNDEFINED;
		pipeline_register[j].IR.opcode = NONE;
		pipeline_register[j].IR.dest = UNDEFINED;
		pipeline_register[j].IR.src1 = UNDEFINED;
		pipeline_register[j].IR.src2 = UNDEFINED;
		pipeline_register[j].IR.immediate = UNDEFINED;
		pipeline_register[j].A = UNDEFINED;
		pipeline_register[j].B = UNDEFINED;
		pipeline_register[j].IMM = UNDEFINED;
		pipeline_register[j].COND = UNDEFINED;
		pipeline_register[j].ALU_OUTPUT = UNDEFINED;
		pipeline_register[j].LMD = UNDEFINED;
	}
}
//return value of special purpose register
unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
	switch(s){
		case IF:
			switch(reg){
				case PC: return pc;
				case NPC: return UNDEFINED;
				case A: return UNDEFINED;
				case B: return UNDEFINED;
				case IMM: return UNDEFINED;
				case ALU_OUTPUT: return UNDEFINED;
				case LMD: return UNDEFINED;
				default: break;
			}
		case ID:
			switch(reg){
				case NPC: return pipeline_register[IF_ID].NPC;
				case A: return pipeline_register[IF_ID].A;
				case B: return pipeline_register[IF_ID].B;
				case IMM: return pipeline_register[IF_ID].IMM;
				case ALU_OUTPUT: return pipeline_register[IF_ID].ALU_OUTPUT;
				case LMD: return pipeline_register[IF_ID].LMD;
				default: break;
			}
		case EXE:
			switch(reg){
				case NPC: return pipeline_register[ID_EXE].NPC;
				case A: return pipeline_register[ID_EXE].A;
				case B: return pipeline_register[ID_EXE].B;
				case IMM: return pipeline_register[ID_EXE].IMM;
				case ALU_OUTPUT: return pipeline_register[ID_EXE].ALU_OUTPUT;
				case LMD: return pipeline_register[ID_EXE].LMD;
				default: break;
			}
		case MEM:
			switch(reg){
				case NPC: return pipeline_register[EXE_MEM].NPC;
				case A: return pipeline_register[EXE_MEM].A;
				case B: return pipeline_register[EXE_MEM].B;
				case IMM: return pipeline_register[EXE_MEM].IMM;
				case ALU_OUTPUT: return pipeline_register[EXE_MEM].ALU_OUTPUT;
				case LMD: return pipeline_register[EXE_MEM].LMD;
				default: break;
			}
		case WB:
			switch(reg){
				case NPC: return pipeline_register[MEM_WB].NPC;
				case A: return pipeline_register[MEM_WB].A;
				case B: return pipeline_register[MEM_WB].B;
				case IMM: return pipeline_register[MEM_WB].IMM;
				case ALU_OUTPUT: return pipeline_register[MEM_WB].ALU_OUTPUT;
				case LMD: return pipeline_register[MEM_WB].LMD;
				default: break;
			}
	}
	return UNDEFINED;
}
//returns value of general purpose register
int sim_pipe::get_gp_register(unsigned reg){
	return gp_register[reg]; //please modify
}
void sim_pipe::set_gp_register(unsigned reg, int value){
	gp_register[reg] = value;
}
float sim_pipe::get_IPC(){
  return (instr_exec / clock_cycle_count); //please modify
}
unsigned sim_pipe::get_instructions_executed(){
  return instr_exec; //please modify
}
unsigned sim_pipe::get_stalls(){
  return stalls; //please modify
}
unsigned sim_pipe::get_clock_cycles(){
  return clock_cycle_count;
}
