 pshs x,y,d
 orcc #$50
 ldx #$6000  ;start receive mem address
start
 ldy #$ffff      ;4
loop
 lda $a7cc     ;5 
 anda #$40   ;2
 beq exit        ;3
 leay -1,y        ;4
 bne loop      ;3
 bra exit2       ;3
exit
 ldb #8           ; 2
rit
 lda  $a7cc      ; 5 read port
 lsla            ; 2
 lsla            ; 2
 ror  ,x         ; 6 shift in bit
 nop ; 2
 nop  ; 2
 nop                 ; 2
 decb                ; 2
 bne     rit         ; 3   / 26
 leax 1,x      ; 4
 bra start    ; 3
exit2
 andcc #$af
 puls x,y,d
 rts