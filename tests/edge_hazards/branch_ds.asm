add $r2, $zero, $imm, 0
add $r3, $zero, $imm, 100
beq $imm, $zero, $zero, 5
add $r2, $r2, $imm, 1
add $r3, $r3, $r2, 0
sw $r3, $zero, $imm, 0
lw $r4, $zero, $imm, 512
halt $zero, $zero, $zero, 0
