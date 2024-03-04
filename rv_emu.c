#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "rv_emu.h"



void unsupported(char *s, uint32_t val) {
    printf("%s: %d\n", s, val);
    exit(-1);
}

void rv_init(struct rv_state *rsp, uint32_t *func,
             uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    int i;

    // Zero out registers
    for (i = 0; i < NREGS; i += 1) {
        rsp->regs[i] = 0;
    }

    // Zero out the stack
    for (i = 0; i < STACK_SIZE; i += 1) {
        rsp->stack[i] = 0;
    }

    // Initialize the Program Counter
    rsp->pc = (uint64_t) func;

    // Initialize the Link Register to a sentinel value
    rsp->regs[RA] = 0;

    // Initialize Stack Pointer to the logical bottom of the stack
    rsp->regs[SP] = (uint64_t) &rsp->stack[STACK_SIZE];

    // Initialize the first 4 arguments in emulated r0-r3
    rsp->regs[A0] = a0;
    rsp->regs[A1] = a1;
    rsp->regs[A2] = a2;
    rsp->regs[A3] = a3;
}


void emu_r_type(struct rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t rs2 = (iw >> 20) & 0b11111;
    uint32_t funct3 = (iw >> 12) & 0b111;
    uint32_t funct7 = (iw >> 25) & 0b1111111;

    if (funct3 == 0b000) {
        if (funct7 == 0b0000000) {
            rsp->regs[rd] = rsp->regs[rs1] + rsp->regs[rs2];
        } else if (funct7 == 0b0100000) {
            rsp->regs[rd] = rsp->regs[rs1] - rsp->regs[rs2];
        } else if(funct7 == 0b0000001) {
        	rsp->regs[rd] = rsp ->regs[rs1] * rsp->regs[rs2];
        	
        }else {
            unsupported("Unsupported R-type instruction", funct3);
        }
        rsp->pc += 4; // Next instruction
    }

 }

void emu_jalr(struct rv_state *rsp, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b1111;  // Will be ra (aka x1)
    uint64_t val = rsp->regs[rs1];  // Value of regs[1]

    rsp->pc = val;  // PC = return address
}


void emu_i_type(struct rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    uint32_t rs1 = (iw >> 15) & 0b11111;
    int32_t imm = (iw >> 20) | ((iw & 0x800) ? 0xFFFFF000 : 0);
    uint32_t opcode = (iw >> 12) & 0b111;

	
switch (opcode) {
        case 0b101: // srli
            rsp->regs[rd] = rsp->regs[rs1] >> imm;
            break;
        case 0b001: // addi
        	rsp->regs[rd] = rsp->regs[rs1] + imm;
        	break;
        case 0b011: // li
        	rsp->regs[rd] = imm;
        	break;
        default:
            unsupported("Unsupported I-type instruction", opcode);
            break;
    }

         rsp->pc += 4; 
}


void emu_s_type(struct rv_state *rsp, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t rs2 = (iw >> 20) & 0b11111;
    int32_t imm = ((iw >> 25) << 5) | ((iw >> 7) & 0b11111);
    uint32_t opcode = (iw >> 12) & 0b111;

    if (opcode == 0b000) {
        rsp->regs[rs1] = rsp->regs[rs2] + imm;
    } else {
        printf("Unsupported S-type instruction: %d\n", opcode);
        exit(-1);
    }

     rsp->pc += 4; 
}

void emu_b_type(struct rv_state *rsp, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b11111;
    uint32_t rs2 = (iw >> 20) & 0b11111;
    int32_t imm = ((iw >> 31) << 12) | (((iw >> 7) & 0x1) << 11) | 
    			  (((iw >> 25) & 0x3F) << 5) | (((iw >> 8) & 0xF) << 1);
    uint32_t opcode = (iw >> 12) & 0b111;
switch (opcode) {
        case 0b000: // BEQ
            if (rsp->regs[rs1] == rsp->regs[rs2]) {
                rsp->pc += imm;
            }
            break;
        case 0b001: // BNE
            if (rsp->regs[rs1] != rsp->regs[rs2]) {
                rsp->pc += imm;
            }
            break;
        case 0b100: // BLT
            if ((int32_t)rsp->regs[rs1] < (int32_t)rsp->regs[rs2]) {
                rsp->pc += imm;
            }
            break;
        case 0b101: // BGE
            if ((int32_t)rsp->regs[rs1] >= (int32_t)rsp->regs[rs2]) {
                rsp->pc += imm;
            }
            break;
        default:
            printf("Unsupported B-type instruction: %d\n", opcode);
            exit(-1);
    }

     rsp->pc += 4; 
}

void emu_u_type(struct rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    uint32_t imm = iw & 0xFFFFF000;
    uint32_t opcode = (iw >> 12) & 0b111;

    if (opcode == 0b011) {
        rsp->regs[rd] = imm;
    } else {
        printf("Unsupported U-type instruction: %d\n", opcode);
        exit(-1);
    }
     rsp->pc += 4; 
}


void emu_j_type(struct rv_state *rsp, uint32_t iw) {
    uint32_t rd = (iw >> 7) & 0b11111;
    int32_t imm = ((iw >> 31) << 20) | ((iw >> 12) & 0xFF) | 
    			  ((iw >> 20) & 0x1) | (((iw >> 21) & 0x3FF) << 1);
    uint32_t opcode = (iw >> 12) & 0b111;

    if (opcode == 0b000) {
        rsp->regs[rd] = rsp->pc + 4;
        rsp->pc += imm;
    } else {
        printf("Unsupported J-type instruction: %d\n", opcode);
        exit(-1);
    }
     rsp->pc += 4; 
}



void rv_one(struct rv_state *rsp) {

    // Get an instruction word from the current Program Counter    
    uint32_t iw = *(uint32_t*) rsp->pc;

    /* could also think of that ^^^ as:
        uint32_t *pc = (uint32_t*) rsp->pc;
        uint32_t iw = *pc;
    */

   uint32_t opcode = iw & 0b1111111;
    switch (opcode) {
        case 0b0110011:
            // R-type instructions have two register operands
            emu_r_type(rsp, iw);
            break;
        case 0b0000011:
            // I-type instructions have one register operand and an immediate value
            emu_i_type(rsp, iw);
            break;
        case 0b0100011:
            // S-type instructions have two register operands and an immediate value
            emu_s_type(rsp, iw);
            break;
        case 0b1100011:
            // B-type instructions have two register operands and a PC-relative immediate value
            emu_b_type(rsp, iw);
            break;
        case 0b1100111:
            emu_jalr(rsp, iw); // Handle JALR instruction
            break;
        case 0b0010011:
            emu_i_type(rsp, iw); // Handle I-type instructions (including LUI)
            break;
        case 0b0110111:
            // U-type instructions have one register operand and an immediate value
            emu_u_type(rsp, iw);
            break;
        case 0b1101111:
            // J-type instructions have one register operand and a PC-relative immediate value
            emu_j_type(rsp, iw);
            break;
        default:
            unsupported("Unknown opcode: ", opcode);
    }
}


int rv_emulate(struct rv_state *rsp) {
    while (rsp->pc != 0) {
       rv_one(rsp);
    }
    
    return (int) rsp->regs[A0];
}
