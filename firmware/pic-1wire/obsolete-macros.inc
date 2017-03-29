	    CODE
;
; Delay subroutine. 
; Upon entry, 'cnt' must contains delay value, in us
; 5 nop's
;
_delay	    
;	    banksel cnt
	    movwf cnt 
dloop	    nop			    ; 1 cycle
	    goto $+1		    ; 2 cycles
	    goto $+1		    ; 2 cycles
	    decfsz cnt, 1
	    goto dloop
dretn	    return

	    
udelay	    MACRO us
	    movlw us
	    call _delay
	    ENDM
	    

sync	    MACRO
	    banksel LATC
	    bsf	    LATC, LATC2
;	    movlw .255
;	    banksel cnt
;	    movwf cnt
;sync_loop   decfsz cnt, 1
;	    goto sync_loop
	    udelay .100
	    banksel LATC
	    bcf	    LATC, LATC2
	    ENDM

	    
ow_xmit	    MACRO chan
	    if chan == 0 
		; channel 0 on pins RA4, RC5
		banksel TRISA
		bcf TRISA, TRISA4
		banksel TRISC
		bcf TRISC, TRISC5
            endif
	    if chan == 1
		; channel 1 on pins RC3, RC4
		banksel TRISC
		bcf TRISC, TRISC3
		bcf TRISC, TRISC4
            endif
	    if chan == 2
		; channel 2 on pin RA2
		banksel TRISA
		bcf TRISA, TRISA2
	    endif
	    if chan == 3
		; channel 3 on pin RC2
		banksel TRISC
		bcf TRISC, TRISC2
	    endif
	    ENDM
	    
	    
ow_recv	    MACRO chan
	    if chan == 0
		banksel TRISA
		bsf TRISA, TRISA4
		banksel TRISC
		bsf TRISC, TRISC5
	    endif
	    if chan == 1
		banksel TRISC
		bsf TRISC, TRISC3
		bsf TRISC, TRISC4
	    endif
	    if chan == 2
		banksel TRISA
		bsf TRISA, TRISA2
	    endif
	    if chan == 3
		banksel TRISC
		bsf TRISC, TRISC2
	    endif
	    ENDM

	    
ow_high	    MACRO chan
	    if chan == 0
		banksel LATA
		bsf LATA, LATA4
		banksel LATC
		bsf LATC, LATC5
	    endif
	    if chan == 1
		banksel LATC
		bsf LATC, LATC3
		bsf LATC, LATC4
	    endif
	    if chan == 2
		banksel LATA
		bsf LATA, LATA2
	    endif
	    if chan == 3
		banksel LATC
		bsf LATC, LATC2
	    endif
	    ENDM

	    
ow_low	    MACRO chan
	    if chan == 0
		banksel LATA
		bcf LATA, LATA4
		banksel LATC
		bcf LATC, LATC5
	    endif
	    if chan == 1
		banksel LATC
		bcf LATC, LATC3
		bcf LATC, LATC4
	    endif
	    if chan == 2
		banksel LATA
		bcf LATA, LATA2
	    endif
	    if chan == 3
		banksel LATC
		bcf LATC, LATC2
	    endif
	    ENDM


ow_write    MACRO chnl, byte_to_send
	    movlw byte_to_send
	    movwf tmp
	    movlw chnl
	    call ONEW_SENDBYTE
	    ENDM

	    
ow_read	    MACRO chnl
	    movlw chnl
	    call ONEW_RECVBYTE
	    ENDM

ow_reset    MACRO chnl, no_pulse
	    movlw chnl
	    call ONEW_RESET
	    btfsc STATUS, C
	    goto no_pulse
	    ENDM
	    
	    
INT_DISABLE MACRO
	    banksel INTCON
	    bcf	    INTCON, GIE
	    ENDM

	    
INT_ENABLE  MACRO
	    banksel INTCON
	    bsf	    INTCON, GIE
	    ENDM