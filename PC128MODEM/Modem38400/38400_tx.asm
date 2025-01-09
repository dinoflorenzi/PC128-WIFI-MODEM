 pshs x,y,d
 ldx #$6000
 ldy #$4000
 orcc #$50
 lda $a7ce
 anda #$fb
 sta $a7ce 
 lda #$80
 sta $a7cc
 lda $a7ce
 ora #$04
 sta $a7ce
 ldb #8
 pshs b        
loop2
 ldb #$00     ;2  _______
 stb $a7cc   ;5
 lda ,x+         ;4 
loop              ;             _______
 anda #$01  ;2                      |
 beq low       ;3     
 ldb #$ff       ;2    21
 bra ret          ;3
low
 ldb #00        ;2    18
ret
 lsra               ;2_________
 stb $a7cc    ;5
 dec ,s             ;6
 bne loop      ;3            ______|
 ldd #$08ff   ;3      21
 sta ,s             ;4_________
 stb $a7cc    ;5
 leay -1,y      ;4
 bne loop2     ;3
exit
 andcc #$af
 puls x,y,d
 rts