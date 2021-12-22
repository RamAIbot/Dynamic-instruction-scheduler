#ifndef SIM_PROC_H
#define SIM_PROC_H

#define TOTAL_ARCH_REGISTERS 67

#include <vector>
#include <limits.h>
using namespace std;

typedef unsigned long int u_ll;
typedef unsigned int u_int;

typedef struct proc_params{
    u_ll rob_size;
    u_ll iq_size;
    u_ll width;
}proc_params;

// Put additional data structures here as per your requirement
typedef struct reorder_buffer_entries{
    bool valid;
    long int value;
    long int dest;
    bool ready;
    bool exception;
    bool miss_prediction;
    u_ll pc;

    reorder_buffer_entries(bool valid,long int value,long int dest,bool ready,bool exception,bool miss_prediction, u_ll pc)
    {
        this->valid = valid;
        this->value = value;
        this->dest = dest;
        this->ready = ready;
        this->exception = exception;
        this->miss_prediction = miss_prediction;
        this->pc = pc;
    }

}reorder_buffer;

typedef struct issue_queue_entries{
    bool valid;
    long int dest_tag;
    bool rs1_ready;
    long int rs1_tag_or_value;
    bool rs2_ready;
    long int rs2_tag_or_value;
    long int counter;
    u_ll opcode;
    int optype;
    int cycles;
    u_ll instruction_num;

    issue_queue_entries(bool valid,long int dest_tag,bool rs1_ready,long int rs1_tag_or_value,bool rs2_ready,long int rs2_tag_or_value,long int counter,u_ll opcode,int optype,int cycles,u_ll instruction_num)
    {
        this->valid = valid;
        this->dest_tag = dest_tag;
        this->rs1_ready = rs1_ready;
        this->rs1_tag_or_value = rs1_tag_or_value;
        this->rs2_ready = rs2_ready;
        this->rs2_tag_or_value = rs2_tag_or_value;
        this->counter = counter;
        this->opcode = opcode;
        this->optype = optype;
        this->cycles = cycles;
        this->instruction_num = instruction_num;
    }

}issue_queue;

typedef struct rename_map_table_entries{
    bool valid;
    long int rob_tag;

    rename_map_table_entries(bool valid,long int rob_tag)
    {
        this->valid = valid;
        this->rob_tag = rob_tag;
        
    }
}rename_map_table_entry;

typedef struct register_entries
{
    u_ll opcode;
    long long int instruction_num;
    int optype;
    int cycles;
    long int rd,rs1,rs2;
    bool is_rob_src1,is_rob_src2;
    bool is_src1_rdy,is_src2_rdy;

    register_entries(long long int instruction_num,u_ll opcode,int optype,int cycles,long int rd,long int rs1,long int rs2,bool is_rob_src1,bool is_rob_src2,bool is_src1_rdy,bool is_src2_rdy)
    {
        this->instruction_num = instruction_num;
        this->opcode = opcode;
        this->optype = optype;
        this->cycles = cycles;
        this->rd = rd;
        this->rs1 = rs1;
        this->rs2 = rs2;
        this->is_rob_src1 = is_rob_src1;
        this->is_rob_src2 = is_rob_src2;
        this->is_src1_rdy = is_src1_rdy;
        this->is_src2_rdy = is_src2_rdy;
    }

}registers;


typedef struct DE_reg
{
    bool full_flag;
    bool empty_flag;
    unsigned int tot_size;
    unsigned int current_size;
    registers *DE_registers;
}DE_reg;


typedef struct result_storage
{
    unsigned int fu;
    pair<u_ll,u_ll> src;
    unsigned int dst;
    pair<u_ll,u_ll> FE,DE,RN,RR,DI,IS,EX,WB,RT;

    result_storage(unsigned int fu,pair<u_ll,u_ll> src,unsigned int dst,pair<u_ll,u_ll> FE,pair<u_ll,u_ll> DE,pair<u_ll,u_ll> RN,pair<u_ll,u_ll> RR,pair<u_ll,u_ll> DI,pair<u_ll,u_ll> IS,pair<u_ll,u_ll> EX,pair<u_ll,u_ll> WB,pair<u_ll,u_ll> RT)
    {
    this->fu = fu;
    this->src = src;
    this->dst = dst;
    this->FE = FE;
    this->DE = DE;
    this->RN = RN;
    this->RR = RR;
    this->DI = DI;
    this->IS = IS;
    this->EX = EX;
    this->WB = WB;
    this->RT = RT;
    }

}results;

//data structures declaration
// vector<reorder_buffer> reorder_buffer_table;
// vector<issue_queue> issue_queue_table;
// vector<rename_map_table_entry> rename_map_table;

reorder_buffer *reorder_buffer_table;
issue_queue *issue_queue_table;
rename_map_table_entry *rename_map_table;
vector<results> results_table;

u_ll head_pointer,tail_pointer;
u_ll iq_pointer;
u_ll exec_pointer;
u_ll WB_pointer;

//Registers
//vector<registers> DE;
DE_reg DE;
DE_reg RN;
DE_reg RR;
DE_reg DI;
DE_reg execute_list;
DE_reg WB;



//Functions
bool Advance_Cycle();


//variables for simulator
u_ll sim_cycles;
int num_instr_in_pipe;
bool trace_depleted;
int width_of_scalar;
u_int rob_size_;
u_int iq_size_;
u_ll program_counter;
int instruction_count_val;
u_ll dynamic_instruction_count;


#endif
