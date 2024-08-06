.global _boot
.text

_boot:
	addi x20, x0, 16     /* x = x20 */
       addi x21, x0, 8      /* y = x21 */
       addi x22, x0, 1      /* vx = x22 */
       addi x23, x0, 1      /* vy = x23 */
       addi x24, x0, 5      /* lh = x24 */
       addi x25, x0, 5      /* rh = x25 */
while:       
       add x30, x20, x22    /* next_x = x + vx */
       add x31, x21, x23    /* next_y = y + vy */
       jal ra, check_col    /* update vx, vy in case of collisions */

       add x10, x0, x20     /* x10 = old_x */
       add x11, x0, x21     /* x11 = old_y */
       add x20, x20, x22    /* x = x + vx */
       add x21, x21, x23    /* y = y + vy */

       add x16, x0, x20     /* x16 = new_x */
       add x17, x0, x21     /* x17 = new_y */
       jal ra, draw_ball
       
       addi x10, x0, 31
       beq x20, x0, reset   /* if (x == 0 || x == 31) goto reset */
       beq x20, x10, reset  
       
back0:
       add x10, x0, x24     /* x10 = lh */
       add x11, x0, x25     /* x11 = rh */
       jal ra, draw_bars
       
       addi x11, x0, 9
       lui x12, 0x9
       lb x10, 0(x12)       /* x10 = mem[0x9000] */
       blt x0, x10, label1  /* if 0 < x10 goto label1 */
       blt x10, x0, label2  /* if x10 < 0 goto label2 */
back1:
       lb x10, 1(x12)       /* x10 = mem[0x9000 + 1] */
	blt x0, x10, label3  /* if 0 < x10 goto label3 */
       blt x10, x0, label4  /* if x10 < 0 goto label4 */
back2:
       jal x0, while
 
label1:
 	bge x24, x11, back1  /* if lh >= 9 goto back1 */
 	add x24, x24, x10    /* lh += 1 */
 	jal x0, back1
label2:
 	beq x24, x0, back1   /* if lh == 0 goto back1 */
       add x24, x24, x10    /* lh += -1 */
	jal x0, back1
label3:
 	bge x25, x11, back2  /* if rh >= 9 goto back2 */
 	add x25, x25, x10    /* rh += 1 */
 	jal x0, back2
label4:
 	beq x25, x0, back2   /* if rh == 0 goto back2 */
       add x25, x25, x10    /* rh += -1 */
	jal x0, back2
 
reset:
 	slli x21, x21, 5            /* y *= 32 */
       lui x10, 0x10
       add x10, x10, x20
       add x10, x10, x21
       sb x0, 0(x10)               /* mem[0x10000 + x + y * 32] = 0 */
 	addi x20, x0, 16            /* x = 16 */
       addi x21, x0, 8             /* y = 8 */
       sub x22, x0, x22            /* vx = -vx */
       sub x23, x0, x23            /* vy = -vy */
       jal x0, back0
        
check_col:
       addi x10, x0, 31            /* x10 = 31 */
       blt x30, x10, next1         /* if next_x < 31 goto next1 */ 
       add x10, x0, x25            /* x10 = rh */
 	bge x21, x10, col_right     /* if y >= rh goto col_right */   
      	jal x0, next1               /* goto next1 */
col_right:
       addi x10, x25, 7            /* x10 = rh + 7 */
       bge x21, x10, next1         /* if y >= rh + 7 goto next1 */
       addi x22, x0, -1            /* vx = -1 */
next1:
	blt x0, x30, next2          /* if 0 < next_x goto next2 */
	add x10, x0, x24            /* x10 = lh */
 	bge x21, x10, col_left      /* if y >= lh goto col_left */
	jal x0, next2               /* goto next2 */
col_left:
	addi x10, x24, 7            /* x10 = lh + 7 */
       bge x21, x10, next2         /* if y >= lh + 7 goto next2 */
       addi x22, x0, 1             /* vx = 1 */
next2:
	addi x10, x0, 16            /* x10 = 16 */
       blt x31, x10, next3         /* if next_y < 16 goto next3 */
       addi x23, x0, -1            /* vy = -1 */
next3:   
       bge x31, x0, next4          /* if next_y >= 0 goto next4 */
       addi x23, x0, 1             /* vy = 1 */
next4:
       ret
 
draw_ball:
 	slli x11, x11, 5     /* old_y *= 32 */
       slli x17, x17, 5     /* new_y *= 32 */
       lui x12, 0x10        /* old_addr = 0x10000 */
       lui x14, 0x10        /* new_addr = 0x10000 */
       add x12, x12, x10    /* old_addr += old_x */
       add x12, x12, x11    /* old_addr += old_y * 32 */
       add x14, x14, x16    /* new_addr += new_x */
       add x14, x14, x17    /* new_addr += new_y * 32 */
       addi x13, x0, 0x00
       addi x15, x0, 0xff
       sb x13, 0(x12)       /* mem[old_addr] = 0 */
       sb x15, 0(x14)       /* mem[new_addr] = 0xff */
       ret

draw_bars:
	slli x10, x10, 5     /* lh *= 32 */
       slli x11, x11, 5     /* rh *= 32 */
       lui x12, 0x10
       lui x13, 0x10
       add x12, x12, x10
       addi x12, x12, 0     /* x12 = 0x10000 + 0 + lh * 32 */     
       add x13, x13, x11
       addi x13, x13, 31    /* x13 = 0x10000 + 31 + rh * 32 */
       addi x14, x12, -32 
       addi x15, x13, -32
                            /* clear over paddles */
       sb x0, 0(x14)        /* mem[x12 - 32] = 0 */
       sb x0, 0(x15)        /* mem[x13 - 32] = 0 */
       addi x14, x0, 6      /* i = 6 */
       addi x15, x0, -1
loop2:
      	sb x15, 0(x12)       /* mem[x12] = 0xff */
       sb x15, 0(x13)       /* mem[x13] = 0xff */
       addi x14,x14, -1     /* i-- */
       addi x12, x12, 32    /* x12 += 32 */
       addi x13, x13, 32    /* x13 += 32 */
       bne x14, x15, loop2  /* if i != -1 goto loop2 */
                            /* clear under paddles */
       sb x0, 0(x12)        /* mem[x12] = 0 */
       sb x0, 0(x13)        /* mem[x13] = 0 */
       ret