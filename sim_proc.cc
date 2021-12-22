#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_proc.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/

bool verbose = false;


void initialize(int rob_size,int iq_size,int width)
{
    reorder_buffer_table = (reorder_buffer *)malloc(rob_size * sizeof(reorder_buffer));
    for(int i=0;i<rob_size;i++)
    {
        //reorder_buffer_entries(bool valid,unsigned long int value, unsigned long int dest,bool ready,bool exception,bool miss_prediction, unsigned long int pc)
        //reorder_buffer_table.push_back(reorder_buffer(false,0,0,false,false,false,0));
        reorder_buffer_table[i] = reorder_buffer(false,0,0,false,false,false,0);
    }

    issue_queue_table = (issue_queue *)malloc(iq_size * sizeof(issue_queue));
    for(int i=0;i<iq_size;i++)
    {
        //issue_queue_entries(bool valid,unsigned long int dest_tag,bool rs1_ready,unsigned long int rs1_tag_or_value,bool rs2_ready,unsigned long int rs2_tag_or_value)
        //issue_queue_table.push_back(issue_queue(false,0,false,0,false,0));
        issue_queue_table[i] = issue_queue(false,0,false,0,false,0,0,0,0,0,0);
    }

    rename_map_table = (rename_map_table_entry *)malloc(TOTAL_ARCH_REGISTERS * sizeof(rename_map_table_entry));
    for(int i=0;i<TOTAL_ARCH_REGISTERS;i++)
    {
        //rename_map_table_entries(bool valid,unsigned long int rob_tag)
        //rename_map_table.push_back(rename_map_table_entries(false,0));
        rename_map_table[i] = rename_map_table_entries(false,0);
    }

    DE.DE_registers = (registers *)malloc(width * sizeof(registers));
    RN.DE_registers = (registers *)malloc(width * sizeof(registers));
    RR.DE_registers = (registers *)malloc(width * sizeof(registers));
    DI.DE_registers = (registers *)malloc(width * sizeof(registers));
    execute_list.DE_registers = (registers *)malloc(width*5 * sizeof(registers));
    WB.DE_registers = (registers *)malloc(width*5 * sizeof(registers));

    for(int i=0;i<width;i++)
    {
        //register_entries(unsigned long int opcode,int cycles,unsigned long int rd,unsigned long int rs1,unsigned long int rs2)
        //DE.push_back(registers(0,0,0,0,0,0,false,true));
        DE.DE_registers[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);
        RN.DE_registers[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);
        RR.DE_registers[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);
        DI.DE_registers[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);
    }

    for(int i=0;i<width*5;i++)
    {
        execute_list.DE_registers[i] = registers(-1,0,0,-1,0,0,0,0,0,0,0);
        WB.DE_registers[i] = registers(-1,0,0,0,0,0,0,0,0,0,0);
    }

    DE.full_flag = false;
    DE.empty_flag = true;
    DE.tot_size = width;
    DE.current_size = 0;

    RN.full_flag = false;
    RN.empty_flag = true;
    RN.tot_size = width;
    RN.current_size = 0;

    RR.full_flag = false;
    RR.empty_flag = true;
    RR.tot_size = width;
    RR.current_size = 0;

    DI.full_flag = false;
    DI.empty_flag = true;
    DI.tot_size = width;
    DI.current_size = 0;

    execute_list.full_flag = false;
    execute_list.empty_flag = true;
    execute_list.tot_size = width*5;
    execute_list.current_size = 0;

    WB.full_flag = false;
    WB.empty_flag = true;
    WB.tot_size = width*5;
    WB.current_size = 0;

    //variable initialization
    sim_cycles = 0;
    num_instr_in_pipe = 0;
    trace_depleted = false;
    width_of_scalar = width;
    rob_size_ = rob_size;
    iq_size_ = iq_size;

    head_pointer = 0;
    tail_pointer = 0;
    program_counter = 0;
    iq_pointer = 0;
    instruction_count_val = 0;
    exec_pointer = 0;
    WB_pointer = 0;
    dynamic_instruction_count = 0;
    
}

void print_results()
{
    for(int i=0;i<results_table.size();i++)
    {
        printf("%d fu{%d} src{%d,%d} dst{%d} ",i,results_table[i].fu,results_table[i].src.first,results_table[i].src.second,results_table[i].dst);
        printf("FE{%d,%d} DE{%d,%d} RN{%d,%d} ",results_table[i].FE.first,results_table[i].FE.second,results_table[i].DE.first,results_table[i].DE.second,results_table[i].RN.first,results_table[i].RN.second);
        printf("RR{%d,%d} DI{%d,%d} IS{%d,%d} ",results_table[i].RR.first,results_table[i].RR.second,results_table[i].DI.first,results_table[i].DI.second,results_table[i].IS.first,results_table[i].IS.second);
        printf("EX{%d,%d} WB{%d,%d} RT{%d,%d}",results_table[i].EX.first,results_table[i].EX.second,results_table[i].WB.first,results_table[i].WB.second,results_table[i].RT.first,results_table[i].RT.second);
        printf("\n");

    }
}
void copy_reg_DE_to_RN(DE_reg *src, DE_reg *tar)
{
    for(int i=0;i<width_of_scalar;i++)
    {
        //register_entries(unsigned long int opcode,int cycles,unsigned long int rd,unsigned long int rs1,unsigned long int rs2)
        //DE.push_back(registers(0,0,0,0,0,0,false,true));
        //reg->DE_registers[i] = registers(0,0,0,0,0,0);
        tar->DE_registers[i].instruction_num = src->DE_registers[i].instruction_num;
        tar->DE_registers[i].cycles = src->DE_registers[i].cycles;
        tar->DE_registers[i].opcode = src->DE_registers[i].opcode;
        tar->DE_registers[i].optype = src->DE_registers[i].optype;
        tar->DE_registers[i].rd = src->DE_registers[i].rd;
        tar->DE_registers[i].rs1 = src->DE_registers[i].rs1;
        tar->DE_registers[i].rs2 = src->DE_registers[i].rs2;
        tar->DE_registers[i].is_rob_src1 = src->DE_registers[i].is_rob_src1;
        tar->DE_registers[i].is_rob_src2 = src->DE_registers[i].is_rob_src2;
        tar->DE_registers[i].is_src1_rdy = src->DE_registers[i].is_src1_rdy;
        tar->DE_registers[i].is_src2_rdy = src->DE_registers[i].is_src2_rdy;

        src->current_size -= 1;

        if(tar->DE_registers[i].instruction_num != -1 && results_table[tar->DE_registers[i].instruction_num].DE.first == 0)
        {
           // printf("\n HERE1\n");
            results_table[tar->DE_registers[i].instruction_num].DE = make_pair(sim_cycles,1);
        }
        else if(tar->DE_registers[i].instruction_num != -1)
        {
            //printf("\n HERE2\n");
            results_table[tar->DE_registers[i].instruction_num].DE.second += 1;
        }
        

        src->DE_registers[i].instruction_num = -1;
        src->DE_registers[i].cycles = -1;
        src->DE_registers[i].opcode = 0;
        src->DE_registers[i].optype = 0;
        src->DE_registers[i].rd = 0;
        src->DE_registers[i].rs1 = 0;
        src->DE_registers[i].rs2 = 0;
        src->DE_registers[i].is_rob_src2 = 0;
        src->DE_registers[i].is_rob_src1 = 0;
        src->DE_registers[i].is_src1_rdy = false;
        src->DE_registers[i].is_src2_rdy = false;
        
    }
}

void print_contents_reg(DE_reg &DE)
{
    printf("\n");
    for(int i=0;i<DE.tot_size;i++)
    {
        printf("%d ",DE.DE_registers[i].instruction_num);
        printf("%d ",DE.DE_registers[i].is_rob_src1);
        printf("%d ",DE.DE_registers[i].is_rob_src2);
        printf("%d ",DE.DE_registers[i].is_src1_rdy);
        printf("%d ",DE.DE_registers[i].is_src2_rdy);
        printf("%d ",DE.DE_registers[i].rd);
        printf("%d ",DE.DE_registers[i].rs1);
        printf("%d ",DE.DE_registers[i].rs2);
    }
}

void copy_reg_RR_to_DI(DE_reg *src, DE_reg *tar)
{
    for(int i=0;i<width_of_scalar;i++)
    {
        //register_entries(unsigned long int opcode,int cycles,unsigned long int rd,unsigned long int rs1,unsigned long int rs2)
        //DE.push_back(registers(0,0,0,0,0,0,false,true));
        //reg->DE_registers[i] = registers(0,0,0,0,0,0);
        if(verbose)
        {
            printf("\n RR to DI copy");
            print_contents_reg(*src);
            print_contents_reg(*tar);
        }
        tar->DE_registers[i].instruction_num = src->DE_registers[i].instruction_num;
        tar->DE_registers[i].cycles = src->DE_registers[i].cycles;
        tar->DE_registers[i].opcode = src->DE_registers[i].opcode;
        tar->DE_registers[i].optype = src->DE_registers[i].optype;
        tar->DE_registers[i].rd = src->DE_registers[i].rd;
        tar->DE_registers[i].rs1 = src->DE_registers[i].rs1;
        tar->DE_registers[i].rs2 = src->DE_registers[i].rs2;
        tar->DE_registers[i].is_rob_src1 = src->DE_registers[i].is_rob_src1;
        tar->DE_registers[i].is_rob_src2 = src->DE_registers[i].is_rob_src2;
        tar->DE_registers[i].is_src1_rdy = src->DE_registers[i].is_src1_rdy;
        tar->DE_registers[i].is_src2_rdy = src->DE_registers[i].is_src2_rdy;

       // results_table[tar->DE_registers[i].instruction_num].RR = make_pair(sim_cycles,1);
        if(RR.DE_registers[i].instruction_num != -1 && results_table[RR.DE_registers[i].instruction_num].RR.first == 0)
        {
            //printf("\n HERE");
            results_table[RR.DE_registers[i].instruction_num].RR = make_pair(sim_cycles,1);
        }
        else if(RR.DE_registers[i].instruction_num != -1)
        {
            results_table[RR.DE_registers[i].instruction_num].RR.second += 1;
        }

        //Added Nov 22 //IMPORTANT 
        if(reorder_buffer_table[src->DE_registers[i].rs1].valid && reorder_buffer_table[src->DE_registers[i].rs1].ready && tar->DE_registers[i].is_src1_rdy == false && tar->DE_registers[i].is_rob_src1 == true)
        {
            tar->DE_registers[i].is_src1_rdy = true;
        }

        if(reorder_buffer_table[src->DE_registers[i].rs2].valid && reorder_buffer_table[src->DE_registers[i].rs2].ready && tar->DE_registers[i].is_src2_rdy == false && tar->DE_registers[i].is_rob_src2 == true)
        {
            tar->DE_registers[i].is_src2_rdy = true;
        }
        //ends

        src->DE_registers[i].instruction_num = -1;
        src->DE_registers[i].cycles = -1;
        src->DE_registers[i].opcode = 0;
        src->DE_registers[i].optype = 0;
        src->DE_registers[i].rd = 0;
        src->DE_registers[i].rs1 = 0;
        src->DE_registers[i].rs2 = 0;
        src->DE_registers[i].is_rob_src2 = 0;
        src->DE_registers[i].is_rob_src1 = 0;
        src->DE_registers[i].is_src1_rdy = false;
        src->DE_registers[i].is_src2_rdy = false;
        
    }
}



void print_contents_iq()
{
    printf("\n");
    for(int i=0;i<iq_size_;i++)
    {
        printf("%d ",issue_queue_table[i].valid);
        printf("%d ",issue_queue_table[i].dest_tag);
        printf("%d ",issue_queue_table[i].rs1_ready);
        printf("%d ",issue_queue_table[i].rs1_tag_or_value);
        printf("%d ",issue_queue_table[i].rs2_ready);
        printf("%d ",issue_queue_table[i].rs2_tag_or_value);
        printf("%d ",issue_queue_table[i].counter);
        printf("\n");
    }
}

void print_contents_rmt()
{
    printf("\n");
    for(int i=0;i<TOTAL_ARCH_REGISTERS;i++)
    {
        printf("%d ",rename_map_table[i].valid);
        printf("%d ",rename_map_table[i].rob_tag);
        printf("\n");
    }
}

void print_contents_rob()
{
    printf("\n");
    for(int i=0;i<rob_size_;i++)
    {
        printf("%d ",reorder_buffer_table[i].valid);
        printf("%d ",reorder_buffer_table[i].value);
        printf("%d ",reorder_buffer_table[i].dest);
        printf("%d ",reorder_buffer_table[i].ready);
        printf("%d ",reorder_buffer_table[i].exception);
        printf("%d ",reorder_buffer_table[i].miss_prediction);
        printf("%d ",reorder_buffer_table[i].pc);
        printf("\n");
    }
}

void print_contents_execute_list()
{
    printf("\n");
    for(int i=0;i<execute_list.tot_size;i++)
    {
        printf("%d ",execute_list.DE_registers[i].cycles);
        printf("%d ",execute_list.DE_registers[i].instruction_num);
        printf("%d ",execute_list.DE_registers[i].is_rob_src1);
        printf("%d ",execute_list.DE_registers[i].is_rob_src2);
        printf("%d ",execute_list.DE_registers[i].opcode);
        printf("%d ",execute_list.DE_registers[i].optype);
        printf("%d ",execute_list.DE_registers[i].rd);
        printf("%d ",execute_list.DE_registers[i].rs1);
        printf("%d ",execute_list.DE_registers[i].rs2);
        printf("\n");
    }
}

bool Advance_Cycle()
{
    sim_cycles++;
    //printf("NUM_INSTR_PIPE_%d,%d",num_instr_in_pipe,trace_depleted);
    if((num_instr_in_pipe <= 0) && (trace_depleted == true))
    {
        //printf("\n ENDS\n");
        return false;
    }
    return true;
}

bool is_rob_full()
{
    int count = 0;
    for(int i=0;i<rob_size_;i++)
    {
        if(reorder_buffer_table[i].valid == false)
            count+=1;
        
    }
    
    if(count >= width_of_scalar)
        return false;
    else    
        return true;
}

bool is_iq_full()
{
    int count = 0;
    for(int i=0;i<iq_size_;i++)
    {
        if(issue_queue_table[i].valid == false)
            count += 1;
    }

    if(count >= width_of_scalar)
        return false;
    else
        return true;
}

bool is_iq_empty()
{
    int count = 0;
    for(int i=0;i<iq_size_;i++)
    {
        if(issue_queue_table[i].valid == false)
            count += 1;
    }

    if(count != iq_size_)
        return false;
    else
        return true;
}

int find_pos_iq()
{
    int pos = -1;
    for(int i=0;i<iq_size_;i++)
    {
        if(issue_queue_table[i].valid == false)
        {
            pos = i;
            return pos;
        }
            
    }
}

void retire()
{
    if(verbose)
        print_contents_rob();
    
    for(int k=0;k<rob_size_;k++)
    {
        if(reorder_buffer_table[k].valid == true && reorder_buffer_table[k].ready == true)
        {
            if(reorder_buffer_table[k].pc != -1 && results_table[reorder_buffer_table[k].pc].RT.first == 0)
            {
                results_table[reorder_buffer_table[k].pc].RT = make_pair(sim_cycles,1);
            }
            else if(reorder_buffer_table[k].pc != -1)
            {
                results_table[reorder_buffer_table[k].pc].RT.second += 1;
            }
        }      
    }

    for(int i=0;i<width_of_scalar;i++)
    {

        // if((reorder_buffer_table[head_pointer].valid == true) && (results_table[reorder_buffer_table[head_pointer].pc].RT.first == 0))
        // {   
        //     results_table[reorder_buffer_table[head_pointer].pc].RT = make_pair(sim_cycles,1);
        // }
        // else if((reorder_buffer_table[head_pointer].valid == true) && (reorder_buffer_table[head_pointer].ready == false))
        // {
        //     results_table[reorder_buffer_table[head_pointer].pc].RT.second += 1;
        // }
        // else
        // {
        //     printf("\n Assert \n");
        // }

        if(reorder_buffer_table[head_pointer].ready == false)
        {
            return;
        }
        else
        {
            reorder_buffer_table[head_pointer].valid = false;
            head_pointer = (head_pointer + 1) % rob_size_;
            num_instr_in_pipe -= 1;
        }

    }
}

void writeback()
{
    if(!WB.empty_flag)
    {
        for(int i=0;i<WB.tot_size;i++)
        {
            int ins_num = WB.DE_registers[i].instruction_num;
            for(int k=0;k<rob_size_;k++)
            {
                if(reorder_buffer_table[k].valid == true && reorder_buffer_table[k].ready == false && reorder_buffer_table[k].pc == ins_num)
                {
                    reorder_buffer_table[k].ready = true;

                    if(rename_map_table[reorder_buffer_table[k].dest].valid == true && rename_map_table[reorder_buffer_table[k].dest].rob_tag == k)
                    {
                        rename_map_table[reorder_buffer_table[k].dest].valid = false;
                    }

                    if(ins_num != -1)
                        results_table[ins_num].WB = make_pair(sim_cycles,1);
                    WB.current_size -= 1;
                    if(WB.current_size == 0)
                        WB.empty_flag = true;

                    WB.DE_registers[i].cycles = 0;
                    WB.DE_registers[i].instruction_num = -1;
                    WB.DE_registers[i].opcode = 0;
                    WB.DE_registers[i].optype = 0;
                    WB.DE_registers[i].rd = 0;
                    WB.DE_registers[i].rs1 = 0;
                    WB.DE_registers[i].rs2 = 0;
                }
            }
        }
    }
    return;
    
}


void execute()
{
    if(!execute_list.empty_flag)
    {
        if(verbose)
        {
            printf("\n EXECUTE LIST\n");
            print_contents_execute_list();
        }
        for(int i=0;i<execute_list.tot_size;i++)
        {
            if(execute_list.DE_registers[i].cycles > 0)
            {
                execute_list.DE_registers[i].cycles -= 1;
            }
            
            int ins_num = execute_list.DE_registers[i].instruction_num;
            if(ins_num != -1 && results_table[ins_num].EX.first == 0)
            {
                int cycle_num = (execute_list.DE_registers[i].optype == 0) ? 1 : ((execute_list.DE_registers[i].optype == 1) ? 2 : 5);
                
                results_table[ins_num].EX = make_pair(sim_cycles,cycle_num);
            }

        }

        for(int i=0;i<execute_list.tot_size;i++)
        {
            if(execute_list.DE_registers[i].cycles == 0)
            {
                WB.DE_registers[WB_pointer].cycles = execute_list.DE_registers[i].cycles;
                WB.DE_registers[WB_pointer].instruction_num = execute_list.DE_registers[i].instruction_num;
                WB.DE_registers[WB_pointer].opcode = execute_list.DE_registers[i].opcode;
                WB.DE_registers[WB_pointer].optype = execute_list.DE_registers[i].optype;
                WB.DE_registers[WB_pointer].rd = execute_list.DE_registers[i].rd;
                WB.DE_registers[WB_pointer].rs1 = execute_list.DE_registers[i].rs1;
                WB.DE_registers[WB_pointer].rs2 = execute_list.DE_registers[i].rs2;

                for(int j=0;j<iq_size_;j++)
                {
                    if(verbose)
                        printf("\nRD_%d",WB.DE_registers[WB_pointer].rd);

                    if(issue_queue_table[j].rs1_ready == false && issue_queue_table[j].rs1_tag_or_value == WB.DE_registers[WB_pointer].rd)
                    {
                        issue_queue_table[j].rs1_ready = true;
                    }


                    if(issue_queue_table[j].rs2_ready == false && issue_queue_table[j].rs2_tag_or_value == WB.DE_registers[WB_pointer].rd)
                    {
                        issue_queue_table[j].rs2_ready = true;
                    }
                }

                //Added
                for(int k=0;k<width_of_scalar;k++)
                {
                    if((RR.DE_registers[k].rs1 == WB.DE_registers[WB_pointer].rd) && (RR.DE_registers[k].is_src1_rdy == false))// && (RR.DE_registers[k].is_rob_src1 == true))
                    {
                        //printf("\n IM RR_SETTING_%d_%d\n",WB.DE_registers[WB_pointer].instruction_num,RR.DE_registers[k].instruction_num);
                        RR.DE_registers[k].is_src1_rdy = true;
                    }

                    if((RR.DE_registers[k].rs2 == WB.DE_registers[WB_pointer].rd) && (RR.DE_registers[k].is_src2_rdy == false))// && (RR.DE_registers[k].is_rob_src2 == true))
                    {
                        //printf("\n IM RR_SETTING2_%d_%d\n",WB.DE_registers[WB_pointer].instruction_num,RR.DE_registers[k].instruction_num);
                        RR.DE_registers[k].is_src2_rdy = true;
                    }


                    if((DI.DE_registers[k].rs1 == WB.DE_registers[WB_pointer].rd) && (DI.DE_registers[k].is_src1_rdy == false))// && (DI.DE_registers[k].is_rob_src1 == true))
                    {
                        //printf("\n IM SETTING_%d_%d\n",WB.DE_registers[WB_pointer].instruction_num,DI.DE_registers[k].instruction_num);
                        DI.DE_registers[k].is_src1_rdy = true;
                    }

                    if((DI.DE_registers[k].rs2 == WB.DE_registers[WB_pointer].rd) && (DI.DE_registers[k].is_src2_rdy == false))// && (DI.DE_registers[k].is_rob_src2 == true))
                    {
                        DI.DE_registers[k].is_src2_rdy = true;
                    }
                }
                //Ends

                //rename_map_table[WB.DE_registers[WB_pointer].rd].valid = false;
                // int dest_reg = WB.DE_registers[WB_pointer].rd;
                // for(int k=0;k<DI.tot_size;k++)
                // {
                //     if(DI.DE_registers[i].rs1 == dest_reg)
                //     {
                //         DI.DE_registers[i].
                //     }
                // }

                // for(int k=0;k<RR.tot_size;k++)
                // {
                    
                // }

                if(verbose)
                    printf("\n iNS_%d",execute_list.DE_registers[i].instruction_num);

                WB.current_size += 1;
                WB.empty_flag = false;
                WB_pointer = (WB_pointer + 1) % WB.tot_size;


                //added nov 22
                execute_list.DE_registers[i].cycles = -1;
                execute_list.DE_registers[i].instruction_num = -1;
                execute_list.DE_registers[i].opcode = 0;
                execute_list.DE_registers[i].optype = 0;
                execute_list.DE_registers[i].rd = 0;
                execute_list.DE_registers[i].rs1 = 0;
                execute_list.DE_registers[i].rs2 = 0;
                //ends
            }
        }
        
    }
    
    return;
    
}

void issue()
{
    if(verbose)
    {
        print_contents_iq();
        print_contents_rmt();
    }
    
    if(!is_iq_empty())
    {
        for(int x=0;x<iq_size_;x++)
        {
            int ins_num = issue_queue_table[x].instruction_num;
            if(ins_num != -1 && issue_queue_table[x].valid == true && results_table[ins_num].IS.first == 0) 
            {
                results_table[ins_num].IS = make_pair(sim_cycles,1);
            }   
            else if(ins_num != -1 && issue_queue_table[x].valid == true)
            {
                results_table[ins_num].IS.second += 1;
            }
        }
        
        for(int k=0;k<width_of_scalar;k++)
        {
            int min_val = INT_MAX;
            int ins_num = -2;
            int instr_to_invalidate = -1;
            for(int i=0;i<iq_size_;i++)
            {
                if(issue_queue_table[i].valid && (issue_queue_table[i].rs1_ready && issue_queue_table[i].rs2_ready) && (issue_queue_table[i].counter < min_val))
                {
                    
                    execute_list.DE_registers[exec_pointer].instruction_num = issue_queue_table[i].instruction_num;
                    execute_list.DE_registers[exec_pointer].cycles = issue_queue_table[i].cycles;
                    execute_list.DE_registers[exec_pointer].opcode = issue_queue_table[i].opcode;
                    execute_list.DE_registers[exec_pointer].optype = issue_queue_table[i].optype;
                    execute_list.DE_registers[exec_pointer].rd = issue_queue_table[i].dest_tag;
                    execute_list.DE_registers[exec_pointer].rs1 = issue_queue_table[i].rs1_tag_or_value;
                    execute_list.DE_registers[exec_pointer].rs2 = issue_queue_table[i].rs2_tag_or_value;
                    min_val = issue_queue_table[i].counter;
                    ins_num = issue_queue_table[i].instruction_num;
                    instr_to_invalidate = i;
                }
                
            }
            if(ins_num!=-2)
            {   
                issue_queue_table[instr_to_invalidate].valid = false;
                issue_queue_table[instr_to_invalidate].dest_tag = 0;
                issue_queue_table[instr_to_invalidate].opcode =0;
                issue_queue_table[instr_to_invalidate].optype = 0;
                issue_queue_table[instr_to_invalidate].cycles = 0;
                issue_queue_table[instr_to_invalidate].instruction_num = -1;
                exec_pointer = (exec_pointer + 1) % execute_list.tot_size;
            }
           
        }

        execute_list.empty_flag = false;
        execute_list.full_flag = false;
    } 

    return;

}

void dispatch()
{
    if(!DI.empty_flag)
    {
        if(is_iq_full())
        {
            //Do nothing
            for(int i=0;i<width_of_scalar;i++)
            {
                if(DI.DE_registers[i].instruction_num != -1 && results_table[DI.DE_registers[i].instruction_num].DI.first == 0)
                {
                    //printf("\n HERE");
                    results_table[DI.DE_registers[i].instruction_num].DI = make_pair(sim_cycles,1);
                }
                else if(DI.DE_registers[i].instruction_num != -1)
                {
                    results_table[DI.DE_registers[i].instruction_num].DI.second += 1;
                }
            }
            return;
        }
        else
        {
            if(verbose)
            {
                print_contents_rob();
            }
            
            for(int i=0;i<width_of_scalar;i++)
            {
                //int iq_pos = find_pos_iq();
                while(issue_queue_table[iq_pointer].valid == true)
                {
                    iq_pointer = (iq_pointer + 1) % iq_size_;
                }
                issue_queue_table[iq_pointer].valid = true;
                issue_queue_table[iq_pointer].dest_tag = DI.DE_registers[i].rd;
                issue_queue_table[iq_pointer].opcode = DI.DE_registers[i].opcode;
                issue_queue_table[iq_pointer].optype = DI.DE_registers[i].optype;
                issue_queue_table[iq_pointer].cycles = DI.DE_registers[i].cycles;
                issue_queue_table[iq_pointer].instruction_num = DI.DE_registers[i].instruction_num;

                int src1_val = DI.DE_registers[i].rs1;
                int src2_val = DI.DE_registers[i].rs2;
                if(verbose)
                    printf("\n SRC1_%d",src1_val);
                //results_table[DI.DE_registers[i].instruction_num].DI = make_pair(sim_cycles,1);

                
                if(DI.DE_registers[i].instruction_num != -1 && results_table[DI.DE_registers[i].instruction_num].DI.first == 0)
                {
                    //printf("\n HERE");
                    results_table[DI.DE_registers[i].instruction_num].DI = make_pair(sim_cycles,1);
                }
                else if(DI.DE_registers[i].instruction_num != -1)
                {
                    results_table[DI.DE_registers[i].instruction_num].DI.second += 1;
                }



                issue_queue_table[iq_pointer].counter = instruction_count_val; //% iq_size_;
                instruction_count_val++;

                

                if(src1_val != -1)
                {
                   // printf("\n SRC1_ROB_%d",DI.DE_registers[i].is_rob_src1);
                    if(src1_val < rob_size_ && reorder_buffer_table[src1_val].valid && DI.DE_registers[i].is_rob_src1)
                    {
                        //printf("\nNEW HERE\n");
                        issue_queue_table[iq_pointer].rs1_ready = reorder_buffer_table[src1_val].ready;
                        issue_queue_table[iq_pointer].rs1_tag_or_value = src1_val;
                    }
                    else
                    {
                        //printf("\nNEW HERE1\n");
                        issue_queue_table[iq_pointer].rs1_ready = true;
                        issue_queue_table[iq_pointer].rs1_tag_or_value = src1_val;
                    }


                     //Added
                    if(DI.DE_registers[i].is_src1_rdy)
                    {
                        //printf("\nNEW HERE2_%d\n",issue_queue_table[iq_pointer].instruction_num);
                        issue_queue_table[iq_pointer].rs1_ready = true;
                    }
                    //Ends

                }
                else
                {
                    //printf("\nNEW HERE3\n");
                    issue_queue_table[iq_pointer].rs1_ready = true;
                    issue_queue_table[iq_pointer].rs1_tag_or_value = 0;
                }

               if(src2_val != -1)
               {
                    if(src2_val < rob_size_ && reorder_buffer_table[src2_val].valid && DI.DE_registers[i].is_rob_src2)
                    {
                        issue_queue_table[iq_pointer].rs2_ready = reorder_buffer_table[src2_val].ready;
                        issue_queue_table[iq_pointer].rs2_tag_or_value = src2_val;
                    }
                    else
                    {
                        issue_queue_table[iq_pointer].rs2_ready = true;
                        issue_queue_table[iq_pointer].rs2_tag_or_value = src2_val;
                    }

                    //Added
                    if(DI.DE_registers[i].is_src2_rdy)
                    {
                        issue_queue_table[iq_pointer].rs2_ready = true;
                    }
                    //Ends
                    
               }

               else
               {
                    issue_queue_table[iq_pointer].rs2_ready = true;
                    issue_queue_table[iq_pointer].rs2_tag_or_value = 0;
               }

             

              

               iq_pointer = (iq_pointer + 1) % iq_size_;

               DI.DE_registers[i].instruction_num = -1;
               DI.DE_registers[i].cycles = -1;
               DI.DE_registers[i].opcode = 0;
               DI.DE_registers[i].optype = 0;
               DI.DE_registers[i].rd = 0;
               DI.DE_registers[i].rs1 = 0;
               DI.DE_registers[i].rs2 = 0;

               //printf("\n ISSUE QUEUE\n");
                        //print_contents_iq();
            }

            DI.empty_flag = true;
            DI.full_flag = false;
        }
    }
}

void register_read()
{
    if(!RR.empty_flag)
    {
        if(DI.full_flag)
        {
            //Do nothing
            for(int i=0;i<width_of_scalar;i++)
            {
                if(RR.DE_registers[i].instruction_num != -1 && results_table[RR.DE_registers[i].instruction_num].RR.first == 0)
                {
                    //printf("\n HERE");
                    results_table[RR.DE_registers[i].instruction_num].RR = make_pair(sim_cycles,1);
                }
                else if(RR.DE_registers[i].instruction_num != -1)
                {
                    results_table[RR.DE_registers[i].instruction_num].RR.second += 1;
                }
            }
            return;
        }
        else
        {
            copy_reg_RR_to_DI(&RR, &DI);
            RR.empty_flag = true;
            RR.full_flag = false;
            DI.empty_flag = false;
            DI.full_flag = true;
            //num_instr_in_pipe -= width_of_scalar;  
        }
    }
    
     if(verbose)
    {
        printf("\n DI contents\n");
        print_contents_reg(DI);
    }
    return;
   
}

void rename()
{
    if(!RN.empty_flag)
    {
        if(RR.full_flag || is_rob_full())
        {
            //Do nothing
            for(int i=0;i<width_of_scalar;i++)
            {
                if(RN.DE_registers[i].instruction_num != -1 && results_table[RN.DE_registers[i].instruction_num].RN.first == 0)
                {
                    //printf("\n HERE");
                    results_table[RN.DE_registers[i].instruction_num].RN = make_pair(sim_cycles,1);
                }
                else if(RN.DE_registers[i].instruction_num != -1)
                {
                    results_table[RN.DE_registers[i].instruction_num].RN.second += 1;
                }
            }
            return;
        }
        else
        {
            for(int i=0;i<width_of_scalar;i++)
            {
                
                int src1 = -1 ,src2 = -1,dest = -1;
                //IMPORTANT Rename Src first then dest or else error occurs in wakeup.
                src1 = RN.DE_registers[i].rs1;
                src2 = RN.DE_registers[i].rs2;

                RR.DE_registers[i].is_rob_src1 = false;
                RR.DE_registers[i].is_rob_src2 = false;
                RR.DE_registers[i].is_src1_rdy = false;
                RR.DE_registers[i].is_src2_rdy = false;

                if(RN.DE_registers[i].rs1!=-1)
                {
                    if(rename_map_table[RN.DE_registers[i].rs1].valid)// && reorder_buffer_table[rename_map_table[RN.DE_registers[i].rs1].rob_tag].ready == false && reorder_buffer_table[rename_map_table[RN.DE_registers[i].rs1].rob_tag].valid == true)
                    {
                        src1 = rename_map_table[RN.DE_registers[i].rs1].rob_tag;
                        RR.DE_registers[i].is_rob_src1 = true;
                    }
                }

                if(RN.DE_registers[i].rs2!=-1)
                {
                    if(rename_map_table[RN.DE_registers[i].rs2].valid) //&& reorder_buffer_table[rename_map_table[RN.DE_registers[i].rs2].rob_tag].ready == false && reorder_buffer_table[rename_map_table[RN.DE_registers[i].rs2].rob_tag].valid == true)
                    {
                        src2 = rename_map_table[RN.DE_registers[i].rs2].rob_tag;
                        RR.DE_registers[i].is_rob_src2 = true;
                    }
                }


                dest = RN.DE_registers[i].rd;
                if(RN.DE_registers[i].rd != -1)
                {
                    rename_map_table[RN.DE_registers[i].rd].valid = true;
                    rename_map_table[RN.DE_registers[i].rd].rob_tag = tail_pointer;
                    dest = tail_pointer;

                    reorder_buffer_table[tail_pointer].valid = true;
                    reorder_buffer_table[tail_pointer].dest = RN.DE_registers[i].rd;
                    reorder_buffer_table[tail_pointer].value =  -1;
                    reorder_buffer_table[tail_pointer].ready = false;
                    reorder_buffer_table[tail_pointer].exception = false;
                    reorder_buffer_table[tail_pointer].miss_prediction = false;
                    reorder_buffer_table[tail_pointer].pc = RN.DE_registers[i].instruction_num;
                    tail_pointer = (tail_pointer + 1) % rob_size_;
                }
                else
                {
                    reorder_buffer_table[tail_pointer].valid = true;
                    reorder_buffer_table[tail_pointer].dest = RN.DE_registers[i].rd;
                    reorder_buffer_table[tail_pointer].value =  -1;
                    reorder_buffer_table[tail_pointer].ready = false;
                    reorder_buffer_table[tail_pointer].exception = false;
                    reorder_buffer_table[tail_pointer].miss_prediction = false;
                    reorder_buffer_table[tail_pointer].pc = RN.DE_registers[i].instruction_num;
                    tail_pointer = (tail_pointer + 1) % rob_size_;
                }
                
                

               
                RR.DE_registers[i].instruction_num = RN.DE_registers[i].instruction_num;
                RR.DE_registers[i].cycles = RN.DE_registers[i].cycles;
                RR.DE_registers[i].opcode = RN.DE_registers[i].opcode;
                RR.DE_registers[i].optype = RN.DE_registers[i].optype;
                RR.DE_registers[i].rd = dest;
                RR.DE_registers[i].rs1 = src1;
                RR.DE_registers[i].rs2 = src2;

                //results_table[RR.DE_registers[i].instruction_num].RN = make_pair(sim_cycles,1);
                if(RN.DE_registers[i].instruction_num != -1 && results_table[RN.DE_registers[i].instruction_num].RN.first == 0)
                {
                    //printf("\n HERE");
                    results_table[RN.DE_registers[i].instruction_num].RN = make_pair(sim_cycles,1);
                }
                else if(RN.DE_registers[i].instruction_num != -1)
                {
                    results_table[RN.DE_registers[i].instruction_num].RN.second += 1;
                }

                RN.DE_registers[i].instruction_num = -1;
                RN.DE_registers[i].cycles = -1;
                RN.DE_registers[i].opcode = 0;
                RN.DE_registers[i].optype = 0;
                RN.DE_registers[i].rd = 0;
                RN.DE_registers[i].rs1 = 0;
                RN.DE_registers[i].rs2 = 0;
                RN.DE_registers[i].is_rob_src1 = false;
                RN.DE_registers[i].is_rob_src2 = false;
                RN.DE_registers[i].is_src1_rdy = false;
                RN.DE_registers[i].is_src2_rdy = false;

            }
            RN.empty_flag = true;
            RN.full_flag = false;
            RR.empty_flag = false;
            RR.full_flag = true;
            //num_instr_in_pipe -= width_of_scalar;  
        }
    } 
     if(verbose)
    {
        printf("\n RR contents\n");
        print_contents_reg(RR);
    }
    return;
}

void decode()
{
    if(!DE.empty_flag)
    {
        if(RN.full_flag)
        {
            for(int i=0;i<width_of_scalar;i++)
            {
                if(DE.DE_registers[i].instruction_num != -1 && results_table[DE.DE_registers[i].instruction_num].DE.first == 0)
                {
                    //printf("\n HERE");
                    results_table[DE.DE_registers[i].instruction_num].DE = make_pair(sim_cycles,1);
                }
                else if(DE.DE_registers[i].instruction_num != -1)
                {
                    results_table[DE.DE_registers[i].instruction_num].DE.second += 1;
                }
            }

            //program_counter--;

            return;
        }
        else
        {
            //memcpy(&RN,&DE,sizeof(registers));
            //RN.DE_registers = DE.DE_registers;
            copy_reg_DE_to_RN(&DE,&RN);
            //make_zero(&DE);
            DE.empty_flag = true;
            DE.full_flag = false;
            RN.empty_flag = false;
            RN.full_flag = true;
           // num_instr_in_pipe -= width_of_scalar; //testing remove it for full run
        }
    }
     if(verbose)
    {
        printf("\n RN contents\n");
        print_contents_reg(RN);
    }
    return;
}

void fetch(FILE **fp)
{
    if(verbose)
    {
        printf("\n Fetch stage \n");
        printf("\n DE_FF_%d",DE.full_flag);
        printf("\n DE_TOT_%d",DE.tot_size);
    }
    
    char buf[60];
    char delim[] = " ";
    if(DE.full_flag || trace_depleted)
    {
        if(verbose)
            printf("\nStalling as DE is full or trace_depleted\n");
        return;
    }

    for(int i=0;i<width_of_scalar;i++)
    {
        unsigned long int pc;
        int op_type,dest,src1,src2;
        if(fscanf(*fp, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
        {   
            if(verbose)
            {
                printf("\n%lx %d %d %d %d \n", pc, op_type, dest, src1, src2);
            }
            dynamic_instruction_count++;
            DE.DE_registers[i].instruction_num = program_counter++;
            DE.DE_registers[i].opcode = (unsigned long int)pc;
            DE.DE_registers[i].optype = op_type;
            DE.DE_registers[i].rd = dest;
            DE.DE_registers[i].rs1 = src1;
            DE.DE_registers[i].rs2 = src2;
            DE.current_size+=1;
            DE.empty_flag = false;
            if(DE.current_size == DE.tot_size)
            {
                DE.full_flag = true;
            }
            
            if(op_type == 0)
            {
                DE.DE_registers[i].cycles = 1;
            }
            else if(op_type == 1)
            {
                DE.DE_registers[i].cycles = 2;
            }
            else
            {
                DE.DE_registers[i].cycles = 5;
            }
            DE.DE_registers[i].is_rob_src1 = false;
            DE.DE_registers[i].is_rob_src2 = false;
            DE.DE_registers[i].is_src1_rdy = false;
            DE.DE_registers[i].is_src2_rdy = false;
            results_table.push_back(results(DE.DE_registers[i].optype,make_pair(src1,src2),dest,make_pair(sim_cycles,1),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0),make_pair(0,0)));

            num_instr_in_pipe += 1;
        }
        else
        {
            trace_depleted = true;
            break;
        }
    }
    
    if(verbose)
    {
        printf("\n DE contents\n");
        print_contents_reg(DE);
    }
    return;
}

void print_register_contents(registers *de,int n)
{
    for(int i=0;i<width_of_scalar;i++)
    {
        if(n == 0)
        {
            printf("\nDE [%d] \n",i);
        }
        else if(n == 1)
        {
            printf("\nRN [%d] \n",i);
        }
        
        printf("\n %u \n",de[i].opcode);
        printf("\n %d \n",de[i].optype);
        printf("\n %d \n",de[i].rd);
        printf("\n %d \n",de[i].rs1);
        printf("\n %d \n",de[i].rs2);
        printf("\n %d \n",de[i].cycles);
        printf("\n ----------- \n");
    }
}


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    unsigned long int pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    // printf("rob_size:%lu "
    //         "iq_size:%lu "
    //         "width:%lu "
    //         "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    // while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    // {
        
    //     printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly
    //     /*************************************
    //         Add calls to OOO simulator here
    //     **************************************/
    // }

   


    initialize(params.rob_size,params.iq_size,params.width);
    do
    {
        retire();
        writeback();
        execute();
        issue();
        dispatch();
        register_read();
        rename();
        decode();   
        fetch(&FP);
        
        if(verbose)
        {
            printf("\n Num_instructions: %d \n",num_instr_in_pipe);
        }
        // int u = num_instr_in_pipe - params.width;
        // if(u >= 0)
        // {
        //     num_instr_in_pipe = u;
        // }
        // else 
        // {
        //     num_instr_in_pipe = 0;
        // }
        
        if(verbose)
        {   
            //print_register_contents(DE.DE_registers,0);
            //print_register_contents(RN.DE_registers,1);
            print_results();
        }
            
        
        
    } while (Advance_Cycle());

    print_results();


    printf("# === Simulator Command =========\n");
    printf("# %s %lu %lu %lu %s\n", argv[0], params.rob_size, params.iq_size, params.width, trace_file);
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %lu \n"
            "# IQ_SIZE  = %lu \n"
            "# WIDTH    = %lu\n", params.rob_size, params.iq_size, params.width);
    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count    = %d\n",dynamic_instruction_count);
    printf("# Cycles                       = %d\n",sim_cycles);
    printf("# Instructions Per Cycle (IPC) = %.2f",((float)(dynamic_instruction_count)/ (float)(sim_cycles)));
    return 0;
}
