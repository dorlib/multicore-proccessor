#size of sub-mat, B = 8
add $r2, $zero, $zero, 0			#pc:0,R2 = temp
add $r3, $zero, $imm, 0x100			#pc:1,address of second mat
add $r4, $zero, $imm, 0x200			#pc:2,address of result mat
add $r5, $zero, $imm,16				#pc:3, R5 = 16, size of mat, N
add $r6, $zero, $zero, 0			#pc:4, R6 = temp
add $r7, $zero, $zero, 0			#pc:5, R7 = 0, i
add $r8, $zero, $zero, 0			#pc:6, R8 = 0, j
add $r9, $zero, $zero, 0			#pc:7, R9 = 0, k
add $r10, $zero, $zero, 0			#pc:8, R10 = 0, jj
add $r11, $zero, $zero, 0			#pc:9, R11 = 0, kk
add $r12, $zero, $zero, 0			#pc:10, R12 = 0, address inside first mat 
add $r13, $zero, $zero, 0			#pc:11, R13 = 0, calculation result, r
add $r14, $zero, $zero, 0			#pc:12, R14 = 0, temp
add $r15, $zero, $zero, 0			#pc:13, R15 = 0, address inside second mat 
bge $imm, $r10, $r5, 67				#pc:14, if jj >= N, exit jj (main) loop
add $zero, $zero, $zero, 0			#pc:15, nop
add $r11, $zero, $zero, $zero		#pc:16, kk = 0
bge $imm, $r11, $r5, 64				#pc:17, if kk >= N, exit kk loop	this was 16
add $zero, $zero, $zero, 0			#pc:18, nop
add $r7, $zero, $zero, 0			#pc:19, i = 0
bge $imm, $r7, $r5, 61				#pc:20, if i >= N, exit i loop	this was 18
add $zero, $zero, $zero, 0			#pc:21, nop
add $r8, $zero, $r10, 0				#pc:22, j = jj
add $r14, $r10, $imm, 8				#pc:23, temp = jj + B    j loop jumps here	this was 20
blt $imm, $r14, $r5, 27				#pc:24, if temp < N, skip next instruction
add $zero, $zero, $zero, 0			#pc:25, nop
add $r14, $zero, $r5, 0				#pc:26, temp = N
bge $imm, $r8, $r14, 58				#pc:27, if min(jj + B, N) <= j, exit j loop	this was 23
add $zero, $zero, $zero, 0			#pc:28, nop
add $r13, $zero, $zero, $zero		#pc:29, r = 0
add $r9, $r11, $zero, 0				#pc:30, k = kk
add $r14, $r11, $imm, 8				#pc:31, temp = kk + B    k loop jumps here	this was 26
blt $imm, $r14, $r5, 35				#pc:32, if temp < N, skip next instruction
add $zero, $zero, $zero, 0			#pc:33, nop
add $r14, $zero, $r5, 0				#pc:34, temp = N
bge $imm, $r9, $r14, 49				#pc:35, if min(kk + B, N) <= k, exit k loop	this was 29
add $zero, $zero, $zero, 0			#pc:36, nop
mul $r12, $r7, $r5, 0				#pc:37, address1 = i*N
add $r12, $r12, $r9, 0				#pc:38, address1 = i*N + k = address1 in memory
mul $r15, $r9, $r5, 0				#pc:39, address2 = k*N
add $r15, $r15, $r8, 0				#pc:40, address2 = k*N + j
add $r15, $r15, $r3, 0				#pc:41, address2 = address2 in memory
lw $r2, $r12, $zero, 0				#pc:42, R2 = MEM[i*N + k] = Y[i][k]
lw $r14, $r15, $zero, 0				#pc:43, R14 = MEM[k*N + j] = Z[k][j]
mul $r6, $r2, $r14, 0				#pc:44, R6 = R2 * R14 = Y[i][k] * Z[k][j]
add $r13, $r13, $r6, 0				#pc:45, r = r + Y[i][k] * Z[k][j]
add $r9, $r9, $imm, 1				#pc:46, k++
beq $imm, $zero, $zero, 31			#pc:47, jump to start of k loop
add $zero, $zero, $zero, 0			#pc:48, nop
mul $r2, $r7, $r5, 0				#pc:49, R2 = i * N	this was 41
add $r2, $r2, $r8, 0				#pc:50, R2 = i * N + j
add $r2, $r2, $r4, 0				#pc:51, R2 = address of X[i][j] in memory
lw $r6, $r2, $zero, 0				#pc:52, R6 = MEM[i*N + j] = X[i][j]
add $r6, $r6, $r13, 0				#pc:53, R6 = R6 + r
sw $r6, $r2, $zero, 0				#pc:54, X[i][j] = R6 = X[i][j] + r
add $r8, $r8, $imm, 1				#pc:55, j++
beq $imm, $zero, $zero, 23			#pc:56, jump to start of j loop
add $zero, $zero, $zero, 0			#pc:57, nop
add $r7, $r7, $imm, 1				#pc:58, i++	this was 49
beq $imm, $zero, $zero, 20			#pc:59, jump to start of i loop
add $zero, $zero, $zero, 0			#pc:60, nop
add $r11, $r11, $imm, 8				#pc:61, kk =  kk + B	this was 51
beq $imm, $zero, $zero, 17			#pc:62, jump to start of kk loop
add $zero, $zero, $zero, 0			#pc:63, nop
add $r10, $r10, $imm, 8				#pc:64, jj =  jj + B	this was 53
beq $imm, $zero, $zero, 14			#pc:65, jump to start of jj loop
add $zero, $zero, $zero, 0			#pc:66, nop
# 2026: For 512-word cache with 8-word blocks, conflict addresses are 512 apart
# Result matrix at 0x200-0x2FF conflicts with addresses 0x000-0x0FF (matrix A)
# We reload matrix A blocks to force writeback of result matrix blocks
lw $r2, $imm, $zero, 0				#pc:67, create conflict for result block at 0x200 (load from 0x000)
lw $r2, $imm, $zero, 8				#pc:68, create conflict for result block at 0x208 (load from 0x008)
lw $r2, $imm, $zero, 16				#pc:69, create conflict for result block at 0x210 (load from 0x010)
lw $r2, $imm, $zero, 24				#pc:70, create conflict for result block at 0x218 (load from 0x018)
lw $r2, $imm, $zero, 32				#pc:71, create conflict for result block at 0x220 (load from 0x020)
lw $r2, $imm, $zero, 40				#pc:72, create conflict for result block at 0x228 (load from 0x028)
lw $r2, $imm, $zero, 48				#pc:73, create conflict for result block at 0x230 (load from 0x030)
lw $r2, $imm, $zero, 56				#pc:74, create conflict for result block at 0x238 (load from 0x038)
lw $r2, $imm, $zero, 64				#pc:75, create conflict for result block at 0x240 (load from 0x040)
lw $r2, $imm, $zero, 72				#pc:76, create conflict for result block at 0x248 (load from 0x048)
lw $r2, $imm, $zero, 80				#pc:77, create conflict for result block at 0x250 (load from 0x050)
lw $r2, $imm, $zero, 88				#pc:78, create conflict for result block at 0x258 (load from 0x058)
lw $r2, $imm, $zero, 96				#pc:79, create conflict for result block at 0x260 (load from 0x060)
lw $r2, $imm, $zero, 104			#pc:80, create conflict for result block at 0x268 (load from 0x068)
lw $r2, $imm, $zero, 112			#pc:81, create conflict for result block at 0x270 (load from 0x070)
lw $r2, $imm, $zero, 120			#pc:82, create conflict for result block at 0x278 (load from 0x078)
lw $r2, $imm, $zero, 128			#pc:83, create conflict for result block at 0x280 (load from 0x080)
lw $r2, $imm, $zero, 136			#pc:84, create conflict for result block at 0x288 (load from 0x088)
lw $r2, $imm, $zero, 144			#pc:85, create conflict for result block at 0x290 (load from 0x090)
lw $r2, $imm, $zero, 152			#pc:86, create conflict for result block at 0x298 (load from 0x098)
lw $r2, $imm, $zero, 160			#pc:87, create conflict for result block at 0x2A0 (load from 0x0A0)
lw $r2, $imm, $zero, 168			#pc:88, create conflict for result block at 0x2A8 (load from 0x0A8)
lw $r2, $imm, $zero, 176			#pc:89, create conflict for result block at 0x2B0 (load from 0x0B0)
lw $r2, $imm, $zero, 184			#pc:90, create conflict for result block at 0x2B8 (load from 0x0B8)
lw $r2, $imm, $zero, 192			#pc:91, create conflict for result block at 0x2C0 (load from 0x0C0)
lw $r2, $imm, $zero, 200			#pc:92, create conflict for result block at 0x2C8 (load from 0x0C8)
lw $r2, $imm, $zero, 208			#pc:93, create conflict for result block at 0x2D0 (load from 0x0D0)
lw $r2, $imm, $zero, 216			#pc:94, create conflict for result block at 0x2D8 (load from 0x0D8)
lw $r2, $imm, $zero, 224			#pc:95, create conflict for result block at 0x2E0 (load from 0x0E0)
lw $r2, $imm, $zero, 232			#pc:96, create conflict for result block at 0x2E8 (load from 0x0E8)
lw $r2, $imm, $zero, 240			#pc:97, create conflict for result block at 0x2F0 (load from 0x0F0)
lw $r2, $imm, $zero, 248			#pc:98, create conflict for result block at 0x2F8 (load from 0x0F8)
halt $zero, $zero, $zero, 0			#pc:99, HALT execution