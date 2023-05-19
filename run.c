/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    *
//*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/


instruction* get_inst_info(uint32_t pc) { 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

void FORWARD_HANDLER(){
    if(CURRENT_STATE.MEM_WB_REG_WRITE &&
            CURRENT_STATE.MEM_WB_DEST && CURRENT_STATE.MEM_WB_DEST == CURRENT_STATE.ID_EX_REG_RS){
            CURRENT_STATE.MEM_WB_FORWARD_REG1 = CURRENT_STATE.ID_EX_REG_RS;
            CURRENT_STATE.MEM_WB_FORWARD_VALUE1 = CURRENT_STATE.MEM_WB_MEM_TO_REG ? CURRENT_STATE.MEM_WB_MEM_OUT : CURRENT_STATE.MEM_WB_ALU_OUT;
    }else{
        CURRENT_STATE.MEM_WB_FORWARD_REG1 = 0;
    }
    if(CURRENT_STATE.MEM_WB_REG_WRITE && 
        CURRENT_STATE.MEM_WB_DEST && CURRENT_STATE.MEM_WB_DEST == CURRENT_STATE.ID_EX_REG_RT){
            CURRENT_STATE.MEM_WB_FORWARD_REG2 = CURRENT_STATE.ID_EX_REG_RT;
            CURRENT_STATE.MEM_WB_FORWARD_VALUE2 = CURRENT_STATE.MEM_WB_MEM_TO_REG ? CURRENT_STATE.MEM_WB_MEM_OUT : CURRENT_STATE.MEM_WB_ALU_OUT;
    }else{
        CURRENT_STATE.MEM_WB_FORWARD_REG2 = 0;
    }
    
    if(CURRENT_STATE.EX_MEM_REG_WRITE && 
        CURRENT_STATE.EX_MEM_DEST && CURRENT_STATE.EX_MEM_DEST == CURRENT_STATE.ID_EX_REG_RT){
            CURRENT_STATE.EX_MEM_FORWARD_REG2 = CURRENT_STATE.ID_EX_REG_RT;
            CURRENT_STATE.EX_MEM_FORWARD_VALUE2 = CURRENT_STATE.EX_MEM_ALU_OUT;
    }else{
        CURRENT_STATE.EX_MEM_FORWARD_REG2 = 0;
    }
    if(CURRENT_STATE.EX_MEM_REG_WRITE && 
        CURRENT_STATE.EX_MEM_DEST && CURRENT_STATE.EX_MEM_DEST == CURRENT_STATE.ID_EX_REG_RS){
            CURRENT_STATE.EX_MEM_FORWARD_REG1 = CURRENT_STATE.ID_EX_REG_RS;
            CURRENT_STATE.EX_MEM_FORWARD_VALUE1 = CURRENT_STATE.EX_MEM_ALU_OUT;
    }else{
        CURRENT_STATE.EX_MEM_FORWARD_REG1 = 0;
    }
}
void NOOP(){
    CURRENT_STATE.ID_EX_REG_WRITE = 0;
    CURRENT_STATE.ID_EX_MEM_TO_REG = 0;
    CURRENT_STATE.ID_EX_MEM_WRITE = 0;
    CURRENT_STATE.ID_EX_MEM_READ = 0;
    CURRENT_STATE.ID_EX_BRANCH = 0;
    CURRENT_STATE.ID_EX_DEST = 0;
}

void IF_Stage(){
    /*FETCH STAGE*/
    CURRENT_STATE.IF_ID_NOOP = 0;
    if(CURRENT_STATE.PC - MEM_TEXT_START <= 4*NUM_INST){ 
        if(CURRENT_STATE.MEM_WB_BR_TAKE){
            CURRENT_STATE.IF_ID_NOOP = 1;
            CURRENT_STATE.PIPE[0] = 0;
            CURRENT_STATE.PC = CURRENT_STATE.BRANCH_PC;
            CURRENT_STATE.BRANCH_PC = 0;
        }else if(CURRENT_STATE.JUMP){
            CURRENT_STATE.IF_ID_NOOP = 1;
            CURRENT_STATE.PC = CURRENT_STATE.JUMP_PC;
        }
        else{CURRENT_STATE.PC+=4;}
        
    }
}
void ID_Stage(){
    /*DECODE STAGE FOR FETCHED INSTRUCTION, GENERATES CONTROL SIGNALS
    IMPORTANT SIGNALS TO BE SET TO 0 BY EVERYTIME NEW INST IS FED, UNLESS CONDITION SATISFIES LATER:
        1. ID_EX_REG_WRITE, 2. ID_EX_MEM_TO_REG, 3.ID_EX_MEM_WRITE, 4. ID_EX_MEM_READ, 5.ID_EX_BRANCH;
    DATA TO BE PROPAGATED FOR NEXT STAGE:
        1. ID_EX_REG1, ID_EX_REG2, ID_EX_IMM
    */
    CURRENT_STATE.ID_EX_REG_WRITE = 0;
    CURRENT_STATE.ID_EX_MEM_TO_REG = 0;
    CURRENT_STATE.ID_EX_MEM_WRITE = 0;
    CURRENT_STATE.ID_EX_MEM_READ = 0;
    CURRENT_STATE.ID_EX_BRANCH = 0;
    if(CURRENT_STATE.IF_ID_NOOP || CURRENT_STATE.MEM_WB_BR_TAKE){
        CURRENT_STATE.PIPE[1] = 0; 
        return;
    }
    instruction* instr = get_inst_info(CURRENT_STATE.IF_ID_NPC);
    CURRENT_STATE.ID_EX_REG_RS = RS(instr);
    CURRENT_STATE.ID_EX_REG_RT = 0;
    CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[CURRENT_STATE.ID_EX_REG_RS];
    CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[CURRENT_STATE.ID_EX_REG_RT];
    CURRENT_STATE.ID_EX_SHAMT = SHAMT(instr);
    CURRENT_STATE.ID_EX_IMM = IMM(instr);
    if(!OPCODE(instr) && FUNC(instr)!=0x8){ /*CASE FOR R-TYPE INST*/
        CURRENT_STATE.ID_EX_DEST = RD(instr);
        CURRENT_STATE.ID_EX_REG_RT = RT(instr);
        CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[CURRENT_STATE.ID_EX_REG_RT];
        CURRENT_STATE.ID_EX_ALU_SRC = 1;
        CURRENT_STATE.ID_EX_REG_WRITE = 1;
        switch(FUNC(instr)){
            case(0x21): CURRENT_STATE.ID_EX_ALU_OP = 1;
                        break;
            case(0x24): CURRENT_STATE.ID_EX_ALU_OP = 2;
                        break;
            case(0x27): CURRENT_STATE.ID_EX_ALU_OP = 4;
                        break;
            case(0x25): CURRENT_STATE.ID_EX_ALU_OP = 3;
                        break;
            case(0x2b): CURRENT_STATE.ID_EX_ALU_OP = 7;
                        break;
            case(0x00): CURRENT_STATE.ID_EX_ALU_OP = 5;
                        break;
            case(0x02): CURRENT_STATE.ID_EX_ALU_OP = 6;
                        break;
            case(0x23): CURRENT_STATE.ID_EX_ALU_OP = 0;
                        break;
            
        }
    }else if(OPCODE(instr) > 3){ /*I-TYPE HERE REG2 DOESNT MATTER*/
        CURRENT_STATE.ID_EX_DEST = RT(instr);
        CURRENT_STATE.ID_EX_ALU_SRC = 0;
        CURRENT_STATE.ID_EX_REG_WRITE = 1;
        switch(OPCODE(instr)){
            case(0x9):  CURRENT_STATE.ID_EX_ALU_OP = 1;
                        break;
            case(0xc):  CURRENT_STATE.ID_EX_ALU_OP = 2;
                        break;
            case(0xf):  CURRENT_STATE.ID_EX_ALU_OP = 8;
                        break;
            case(0xd):  CURRENT_STATE.ID_EX_ALU_OP = 3;
                        break;
            case(0x4):  CURRENT_STATE.ID_EX_BRANCH = 1;
                        CURRENT_STATE.ID_EX_REG_WRITE = 0;
                        CURRENT_STATE.ID_EX_ALU_OP = 0;
                        CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[RT(instr)];
                        break;
            case(0x5):  CURRENT_STATE.ID_EX_BRANCH = 1;
                        CURRENT_STATE.ID_EX_REG_WRITE = 0;
                        CURRENT_STATE.ID_EX_ALU_OP = 1;
                        CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[RT(instr)];
                        break;
            case(0x23): CURRENT_STATE.ID_EX_ALU_OP = 1;
                        CURRENT_STATE.ID_EX_MEM_READ = 1;
                        CURRENT_STATE.ID_EX_MEM_TO_REG = 1;
                        break;
            case(0xb):  CURRENT_STATE.ID_EX_ALU_OP = 7;
                        break;
            case(0x2b): CURRENT_STATE.ID_EX_ALU_OP = 1;  /*CASE FOR SW HAVE TO EXPLICITELY SET REG_WRITE = 0*/
                        CURRENT_STATE.ID_EX_REG_WRITE = 0;
                        CURRENT_STATE.ID_EX_MEM_WRITE = 1;
                        break;
        }
    }else{
        switch(OPCODE(instr)){
            case(0x2):
                CURRENT_STATE.JUMP_PC = 4*TARGET(instr);
                CURRENT_STATE.JUMP = 1;
                break;
            case(0x3):
                CURRENT_STATE.REGS[31] = CURRENT_STATE.IF_ID_NPC+4;
                CURRENT_STATE.JUMP_PC = 4*TARGET(instr);
                CURRENT_STATE.JUMP = 1;
                break;
            case(0x0):
                CURRENT_STATE.JUMP_PC = CURRENT_STATE.ID_EX_REG1;
                CURRENT_STATE.JUMP = 1;
                break;
        }
        
    }  

}

void EX_Stage(){
    if(CURRENT_STATE.EX_MEM_FORWARD_REG1 && CURRENT_STATE.ID_EX_REG_RS &&
        CURRENT_STATE.EX_MEM_FORWARD_REG1 == CURRENT_STATE.ID_EX_REG_RS) CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_FORWARD_VALUE1;
    else if(CURRENT_STATE.MEM_WB_FORWARD_REG1 && CURRENT_STATE.ID_EX_REG_RS &&
        CURRENT_STATE.MEM_WB_FORWARD_REG1 == CURRENT_STATE.ID_EX_REG_RS) CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_FORWARD_VALUE1;

    if(CURRENT_STATE.EX_MEM_FORWARD_REG2 && CURRENT_STATE.ID_EX_ALU_SRC &&
        CURRENT_STATE.EX_MEM_FORWARD_REG2 == CURRENT_STATE.ID_EX_REG_RT) CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_FORWARD_VALUE2;
    else if(CURRENT_STATE.MEM_WB_FORWARD_REG2 && CURRENT_STATE.ID_EX_ALU_SRC &&
        CURRENT_STATE.MEM_WB_FORWARD_REG2 == CURRENT_STATE.ID_EX_REG_RT) CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_FORWARD_VALUE2; 
    CURRENT_STATE.EX_MEM_BR_TAKE = 0; /*DEFAULT TO 0 */
    if(CURRENT_STATE.ID_EX_BRANCH){
        if(!CURRENT_STATE.ID_EX_ALU_OP){
            CURRENT_STATE.EX_MEM_BR_TAKE = !(CURRENT_STATE.ID_EX_REG1 - CURRENT_STATE.ID_EX_REG2);
        }else{
            CURRENT_STATE.EX_MEM_BR_TAKE = !!(CURRENT_STATE.ID_EX_REG1 - CURRENT_STATE.ID_EX_REG2);
        }
    }else{
        uint32_t OPERAND2 = CURRENT_STATE.ID_EX_ALU_SRC ? CURRENT_STATE.ID_EX_REG2 : SIGN_EX(CURRENT_STATE.ID_EX_IMM);

        
        switch(CURRENT_STATE.ID_EX_ALU_OP){
            case(0): CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 - OPERAND2; 
                        break;
            case(1): CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 + OPERAND2; 
                        break;
            case(2): CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 & OPERAND2; 
                        break;
            case(3): CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 | OPERAND2; 
                        break;
            case(4): CURRENT_STATE.EX_MEM_ALU_OUT = ~(CURRENT_STATE.ID_EX_REG1 | OPERAND2); 
                        break;
            case(5): OPERAND2 = CURRENT_STATE.ID_EX_SHAMT;
                     CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG2 << OPERAND2; 
                        break;
            case(6): OPERAND2 = CURRENT_STATE.ID_EX_SHAMT;
                     CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG2 >> OPERAND2; 
                        break;
            case(7): CURRENT_STATE.EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_REG1 < OPERAND2; 
                        break;
            case(8): CURRENT_STATE.EX_MEM_ALU_OUT = OPERAND2 << 16; 
                        break;
        }
    }
    CURRENT_STATE.EX_MEM_BR_TARGET = (CURRENT_STATE.ID_EX_NPC+4)+(SIGN_EX(CURRENT_STATE.ID_EX_IMM) << 2);  
    CURRENT_STATE.EX_MEM_W_VALUE = CURRENT_STATE.ID_EX_REG1;

    CURRENT_STATE.EX_MEM_REG_WRITE = CURRENT_STATE.ID_EX_REG_WRITE;
    CURRENT_STATE.EX_MEM_MEM_TO_REG = CURRENT_STATE.ID_EX_MEM_TO_REG;
    CURRENT_STATE.EX_MEM_DEST = CURRENT_STATE.ID_EX_DEST;
    CURRENT_STATE.EX_MEM_MEM_READ = CURRENT_STATE.ID_EX_MEM_READ;
    CURRENT_STATE.EX_MEM_MEM_WRITE = CURRENT_STATE.ID_EX_MEM_WRITE;

    if(CURRENT_STATE.MEM_WB_BR_TAKE){
        CURRENT_STATE.PIPE[2] = 0;
        CURRENT_STATE.EX_MEM_REG_WRITE = 0;
        CURRENT_STATE.EX_MEM_MEM_TO_REG = 0;
        CURRENT_STATE.EX_MEM_MEM_WRITE = 0;
        CURRENT_STATE.EX_MEM_MEM_READ = 0;
        CURRENT_STATE.EX_MEM_BR_TAKE = 0;
    }
}
void MEM_Stage(){
    if(CURRENT_STATE.EX_MEM_MEM_WRITE){
        mem_write_32(CURRENT_STATE.EX_MEM_ALU_OUT, CURRENT_STATE.EX_MEM_W_VALUE);
    }
    else if(CURRENT_STATE.EX_MEM_MEM_READ){
        CURRENT_STATE.MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.EX_MEM_ALU_OUT);
    }
    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
    CURRENT_STATE.MEM_WB_BR_TAKE = CURRENT_STATE.EX_MEM_BR_TAKE;
    CURRENT_STATE.MEM_WB_MEM_TO_REG = CURRENT_STATE.EX_MEM_MEM_TO_REG;
    CURRENT_STATE.MEM_WB_REG_WRITE = CURRENT_STATE.EX_MEM_REG_WRITE;
    CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;
    CURRENT_STATE.BRANCH_PC = CURRENT_STATE.EX_MEM_BR_TARGET;
    
}
void WB_Stage(){
    if(CURRENT_STATE.MEM_WB_NPC) INSTRUCTION_COUNT++;
    if(CURRENT_STATE.MEM_WB_REG_WRITE){
        CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_MEM_TO_REG ? CURRENT_STATE.MEM_WB_MEM_OUT : CURRENT_STATE.MEM_WB_ALU_OUT;
    }
    if(CURRENT_STATE.MEM_WB_NPC == (MEM_TEXT_START + 4*(NUM_INST - 1))) RUN_BIT = 0;
    
}
/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(){
    /*AT WB STAGE; TAKING NECES
    SARY VALUES AND CONTROL SIGNALS FROM MEM_WB_PIPELINE REGISTERS*/
    CURRENT_STATE.JUMP = 0;
    if(CURRENT_STATE.PIPE_START[4]){
        CURRENT_STATE.PIPE[4] = CURRENT_STATE.MEM_WB_NPC;
        WB_Stage();
    }
    if(!RUN_BIT) CURRENT_STATE.PIPE[3] = 0;
    if(RUN_BIT){

        if(CURRENT_STATE.PIPE_START[3] && CURRENT_STATE.EX_MEM_NPC < (MEM_TEXT_START + 4 * NUM_INST)) {
             if(!CURRENT_STATE.PIPE_START[4]) CURRENT_STATE.PIPE_START[4] = 1;    
            CURRENT_STATE.PIPE[3]= CURRENT_STATE.EX_MEM_NPC;
            MEM_Stage();
            CURRENT_STATE.MEM_WB_NPC = CURRENT_STATE.EX_MEM_NPC; 
        }


        if(CURRENT_STATE.PIPE_START[2] && CURRENT_STATE.ID_EX_NPC < (MEM_TEXT_START + 4 *NUM_INST)){
            if(!CURRENT_STATE.PIPE_START[3]) CURRENT_STATE.PIPE_START[3] = 1;    
            CURRENT_STATE.PIPE[2]= CURRENT_STATE.ID_EX_NPC;
            EX_Stage();
            CURRENT_STATE.EX_MEM_NPC = CURRENT_STATE.PIPE[2];
            
        }else{
            CURRENT_STATE.PIPE[2] = 0;
            CURRENT_STATE.EX_MEM_NPC = CURRENT_STATE.ID_EX_NPC;
        }

        if(CURRENT_STATE.PIPE_START[1] && CURRENT_STATE.IF_ID_NPC <  (MEM_TEXT_START + 4 * NUM_INST)){
            if(!CURRENT_STATE.PIPE_START[2]) CURRENT_STATE.PIPE_START[2] = 1;
            CURRENT_STATE.PIPE[1]= CURRENT_STATE.IF_ID_NPC;
            if(CURRENT_STATE.PIPE_STALL[1] > 0){
                NOOP();
                CURRENT_STATE.ID_EX_NPC = 0;
                CURRENT_STATE.PIPE_STALL[1]--;
            }else{
                ID_Stage();
                CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.PIPE[1];
            }
        }else{
            CURRENT_STATE.PIPE[1]=0;
            CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
        }

        if(CURRENT_STATE.PC < MEM_TEXT_START + 4*NUM_INST){
            if(!CURRENT_STATE.PIPE_START[1]) CURRENT_STATE.PIPE_START[1] = 1;
            CURRENT_STATE.PIPE[0] = CURRENT_STATE.PC;
            if(CURRENT_STATE.PIPE_STALL[0] > 0) CURRENT_STATE.PIPE_STALL[0]--;
            else{IF_Stage(); CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PIPE[0];}
        }else{
            CURRENT_STATE.PIPE[0]= 0;
            CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PC;
        }
        
        
       
        FORWARD_HANDLER();
        
        if(!CURRENT_STATE.PIPE_STALL[1] && CURRENT_STATE.ID_EX_MEM_READ && (CURRENT_STATE.ID_EX_DEST == RT(get_inst_info(CURRENT_STATE.IF_ID_NPC)) || 
            CURRENT_STATE.ID_EX_DEST == RS(get_inst_info(CURRENT_STATE.IF_ID_NPC)) )){
                CURRENT_STATE.PIPE_STALL[0] = 1;
                CURRENT_STATE.PIPE_STALL[1] = 1;
         }

    }
    


}
