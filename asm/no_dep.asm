LW	R1 0(R0)
ADD	R2 R3 R4
ADDI	R3 R3 10
SUB	R4 R4 R1
SW  R2 0(R0)
ADD	R1 R2 R3
SW	R4 4(R0)
ADD	R5 R5 R6
LW	R6 4(R0)
EOP
