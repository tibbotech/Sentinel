; Transparent 1-WIRE to I2C bridge firmware for PIC 16F1824 microcontroller 
; based tibbits
;	    
; (c) 2016 Tibbo Technology, Inc. All Rights Reserved.
;	    
; Developed and maintained by Alexey (skuf) Skoufyin, alexey.skoufyin@tibbo.com
;	    
; Version 1.0 (01.06.2016)
;	    
; Notes:
;   - Supported modes for 1-Wire is RESET-WRITE/RESET-WRITE-READ/RESET-READ only
;   - Supported modes for Single-Channel is RESET-READ only
;
; Known bugs and issues:
;   - Channel #3 on RC2 pin works incorrectly
;
#include "p16F1824.inc"

	    
	    nolist
	    

; CONFIG1
; __config 0x3FC4
 __CONFIG _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _PWRTE_OFF & _MCLRE_ON & _CP_OFF & _CPD_OFF & _BOREN_ON & _CLKOUTEN_OFF & _IESO_ON & _FCMEN_ON
; CONFIG2
; __config 0x3FFF
 __CONFIG _CONFIG2, _WRT_OFF & _PLLEN_ON & _STVREN_ON & _BORV_LO & _LVP_ON


#define	    I2C_ADDR	    .6	    ; Our I2C slave address. Hardcoded by now


; Bit flags, used internally in code
; 
	    CBLOCK 0
	    ReadRq		    ; Master requested a byte from us
	    Read2Rq		    ; Master initiates FCV read sequence
	    WriteRq		    ; Master sends byte to 1-Wire bus
	    ResetRq		    ; Bus reset request
	    Resetted		    ; Bus resetted after last I2C START. Used in RESTART detection
	    ByteSent		    ; Slave is sending bytes to master. Check SSP1CON2[ACKSTAT]
	    CmdByte		    ; First byte sent by master is command byte
	    ENDC
 

OW_NONE	    EQU	    0x00	    ; Unused
OW_READ_ROM EQU	    0x33	    ; Read connected device ROM address (8 bytes)
OW_SKIP_ROM EQU	    0xCC	    ; Disable device addressing on bus
OW_MATCH_ROM EQU    0x55	    ; Select specific device on bus. 8 bytes of device ROM address must follow command
OW_UNUSED_1 EQU	    0xAA	    ; Unused
OW_SEARCH_ROM EQU   0xF0	    ; Initiate SEARCH ROM routine
OW_SEARCH_INIT EQU   0xF1	    ; Continue SEARCH ROM routine
OW_UNUSED_2 EQU	    0x0F	    ; Unused
OW_UNUSED_3 EQU	    0xFF	    ; Unused

 
 ; Please don't change order of these constants. Add new constants
 ; before IND_None to maintain code compatibility, or adjust jumptables
 ; accordingly
 ;
	    CBLOCK 0
	    IND_SkipROM		    ; = 0
	    IND_ReadROM		    ; = 1
	    IND_MatchROM	    ; = 2
	    IND_SearchROM	    ; = 3
	    IND_SearchINIT	    ; = 4
	    IND_Unused
	    IND_Unknown
	    ENDC
	    
	    
	    UDATA
	    
recvbuf	    res .32		    ; Receive buffer
	    
	    UDATA_SHR
;	    
; These registers will be placed into common shared memory area (0x70...0x7F) to
; prevent excess 'banksel' instructions
;	    
cnt	    res 1		    ; low 8 bits of general purpose counter
cnt_hi	    res 1		    ; high 8 bits of general purpose counter
bit	    res 1		    ; bit counter
tmp_int	    res 1		    ; interrupt temporary register
chnl_int    res 1		    ; interrupt channel register
tmp_sr	    res 1		    ; send/recv temporary register
chnl_sr	    res 1		    ; selected channel # for CALL's
flags	    res	1		    ; Misc. flags
rptr	    res 1		    ; Read pointer
wptr	    res 1		    ; Write pointer
	    
; These registers can be shared, because it used only in SEARCH_ROM subroutine	    
;	    
fork	    res 1	    
prev	    res 1
next	    res 1
newfork	    res 1
	    
	    IDATA		    
;
; Initialized data section
;
fw_name	    db "1W03.009"	    ; Frimware ID (1Wmm.Vvv) m=month, V=major version, v=minor version

;
; Code section begins here
;
RES_VECT    CODE 0x0000		    ; processor reset vector
	    goto START              ; go to program entry point

	    
INTR_VECT   CODE 0x0004		    ; Processor Interrupt vector
	    goto INTR		    ; go to ISR


MAIN_PROG   CODE


; Delay routine, tuned for 32 Mhz + PLL oscillator settings. If you change
; clock speed of PIC, please re-tune internal cycle body to achieve correct
; delays
;   
_delay	    movwf cnt 
dloop	    nop			    ; 1 cycle
	    goto $+1		    ; 2 cycles
	    goto $+1		    ; 2 cycles
	    decfsz cnt, 1
	    goto dloop
dretn	    return


; Macro to simplify things
;
udelay	    MACRO us
	    movlw us
	    call _delay
	    ENDM

; Issue debug pulse on RA5 PIC pin, to easy things when debugging with
; oscilloscope
;
dbg_pulse   MACRO
	    banksel TRISA
	    bcf	    TRISA, TRISA5
	    banksel PORTA
	    bsf	    PORTA, RA5
	    udelay  .10
	    banksel PORTA
	    bcf	    PORTA, RA5
	    udelay  .10
	    ENDM


pulse_high  MACRO
	    banksel PORTA
	    bsf	    PORTA, RA5
	    ENDM

	    
pulse_low   MACRO
	    banksel PORTA
	    bcf	    PORTA, RA5
	    ENDM
	    
    
; Sets specified line to LOW state
;	    
ONEW_LOW    movfw   chnl_sr
	    brw
	    goto    _owlow_0
	    goto    _owlow_1
	    goto    _owlow_2
owlow_3	    
	    banksel LATC
	    bcf	    LATC, LATC2
	    return

_owlow_0
	    banksel LATA
	    bcf	    LATA, LATA4
	    banksel LATC
	    bcf	    LATC, LATC5
	    return

_owlow_1
	    banksel LATC
	    bcf	    LATC, LATC3
	    bcf	    LATC, LATC4
	    return

_owlow_2	  
	    banksel LATA
	    bcf	    LATA, LATA2
	    return
	    

; Sets specified line to HIGH state	    
;	    
ONEW_HIGH   movfw   chnl_sr
	    brw
	    goto    _owhi_0
	    goto    _owhi_1
	    goto    _owhi_2
_owhi_3
	    banksel LATC
	    bsf	    LATC, LATC2
	    return
_owhi_0	    
	    banksel LATA
	    bsf	    LATA, LATA4
	    banksel LATC
	    bsf	    LATC, LATC5
	    return
_owhi_1	    
	    banksel LATC
	    bsf	    LATC, LATC3
	    bsf	    LATC, LATC4
	    return
_owhi_2	    
	    banksel LATA
	    bsf	    LATA, LATA2
	    return


; Takes control over line
;	    
ONEW_XMIT   movfw   chnl_sr
	    brw
	    goto    _owxmit_0
	    goto    _owxmit_1
	    goto    _owxmit_2
_owxmit_3	    
	    banksel TRISC
	    bcf TRISC, TRISC2
	    return
_owxmit_0	    
	    banksel TRISA
	    bcf TRISA, TRISA4
	    banksel TRISC
	    bcf TRISC, TRISC5
	    return
_owxmit_1	    
	    banksel TRISC
	    bcf TRISC, TRISC3
	    bcf TRISC, TRISC4
	    return
_owxmit_2	    
	    banksel TRISA
	    bcf TRISA, TRISA2
	    return


; Releases control over line
;
ONEW_RECV   movfw   chnl_sr
	    brw
	    goto    _owrecv_0
	    goto    _owrecv_1
	    goto    _owrecv_2

_owrecv_3	    
	    banksel TRISC
	    bsf TRISC, TRISC2
	    return
	    
_owrecv_0	    
	    banksel TRISA
	    bsf TRISA, TRISA4
	    banksel TRISC
	    bsf TRISC, TRISC5
	    return
	    
_owrecv_1	    
	    banksel TRISC
	    bsf TRISC, TRISC3
	    bsf TRISC, TRISC4
	    return
	    
_owrecv_2	    
	    banksel TRISA
	    bsf TRISA, TRISA2
	    return

	    
; Sample single bit from 1-Wire bus
;
ONEW_RECVB  call    ONEW_XMIT	    ; Take control over line
	    call    ONEW_LOW	    ; Pull it low...
	    udelay  .2		    ; ... for 2-5 us
	    call    ONEW_RECV	    ; Release line
	    udelay  .8		    ; 8 us delay
;	    goto    ONEW_SAMPLE
	    
;	    pulse_low
;	    nop
;	    nop
;	    nop
;	    nop
;	    nop
;	    nop
;	    pulse_high
	    
; Samples specified line
; Returns C=0 when bus is low and C=1 when bus is high
;	    
ONEW_SAMPLE movfw   chnl_sr
	    bcf	    STATUS, C
	    brw
	    goto    _smpl_c0
	    goto    _smpl_c1
	    goto    _smpl_c2

; Sample channel 3
	    banksel PORTC
	    btfsc   PORTC, RC2
	    bsf	    STATUS, C
	    return
	    
; Sample channel 0	    
_smpl_c0
	    banksel PORTC
	    btfsc   PORTC, RC5
	    bsf	    STATUS, C
	    return
	    
; Sample channel 1	    
_smpl_c1    
	    banksel PORTC
    	    btfsc   PORTC, RC4
	    bsf	    STATUS, C
	    return
	    
; Sample channel 2
_smpl_c2
	    banksel PORTA
	    btfsc   PORTA, RA2
	    bsf	    STATUS, C
	    return

	    
; Send one bit to 1-Wire bus
; Do not forget to release bus after you finish with sending of all bits
;	    
ONEW_SENDB  call    ONEW_LOW	    
	    btfsc   STATUS, C
	    goto    _sendb_one
	    udelay .60		    
	    call    ONEW_HIGH
	    udelay .3
	    return
	    
_sendb_one  udelay  .2
	    call    ONEW_HIGH
	    udelay  .61
	    return
	    
; Sends one byte to selected 1-wire channel
;	    
; Byte in 'tmp_sr' register, channel in 'chnl_sr' register
; Required delay values taken from 1-Wire specification PDF	    
;	    
ONEW_SENDBYTE
	    call    ONEW_XMIT	    ; Take control over line
	    movlw   .8		    ; Bit count = 8
	    movwf   bit		    ; Save bit count in counter
_sndbit	    rrf	    tmp_sr, F	    ; Shift rightmost (less significant) bit into 'C'
	    call    ONEW_SENDB
	    decfsz  bit, F	    ; Decrement bit count
	    goto    _sndbit	    ; Loop, if not zero
	    call    ONEW_RECV	    ; Release line
	    return


; Receives one byte from slave device
;	    
; On input 'chnl_sr' must contain 1-Wire channel number 0...3
; On output 'tmp_sr' and WREG contains byte read from specified channel
;
ONEW_RECVBYTE
	    movlw   .8		    ; Bit count to receive
	    movwf   bit		    ; ... save in counter
	    clrf    tmp_sr	    ; Clear bit accumulation register
recv_bits   call    ONEW_RECVB	    ; Get one bit from bus
	    rrf	    tmp_sr, F	    ; Bits in order: low to high
	    udelay  .60		    ; Delay until next time slot starts
	    decfsz  bit, F	    ; Decrement bit count...
	    goto    recv_bits	    ; ... and repeat, if needed
	    movfw   tmp_sr	    ; Load result into WREG
	    return
	    

; Get byte from circular queue
;	    
GET_BYTE    movlw   LOW recvbuf
	    addwf   rptr, W
	    movwf   FSR0L
	    movlw   HIGH recvbuf
	    movwf   FSR0H
	    clrw
	    addwfc  FSR0H, F
	    movfw   INDF0
	    movwf   tmp_sr
	    incf    rptr, W
	    andlw   b'00001111'
	    movwf   rptr
	    return

	    
DETECT_LOW  call    ONEW_SAMPLE
	    btfss   STATUS, C
	    goto    low_ok
	    decfsz  cnt_hi
	    goto    DETECT_LOW
low_ok	    return

	    
DETECT_HI   call    ONEW_SAMPLE
	    btfsc   STATUS, C
	    goto    hi_ok
	    decfsz  cnt_hi
	    goto    DETECT_HI
hi_ok	    return
	    

; Initiate read sequence for f#%$d Chinese Single-Wire gauges and read all available bits
; into internal buffer for further sending to master
;
; Channel number contained in 'chnl_int' and set by ISR, so this procedure must 
; be called immediately after interrupt
;	    
READ_CFV    clrf    tmp_sr	    ; Clear accum
	    clrf    rptr	    ; and buffer
	    clrf    wptr	    ; pointers
	    
	    movlw   LOW recvbuf
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH recvbuf
	    banksel FSR0H
	    movwf   FSR0H	
	    
	    movfw   chnl_int	    ; Copy channel ID 
	    movwf   chnl_sr	    ; 
	    
	    call    ONEW_XMIT	    ; Take control over line
	    call    ONEW_LOW	    ; Set line to LOW
	    udelay .250		    ; 500 μs delay
	    udelay .250		    ; (480-960 μs allowed by standard)
	    call    ONEW_RECV	    ; Allow line to float
    	    udelay .130		    ; Device must hold line low for 75-85 μs...
	    
cfv_loop    movlw   .50		    ; 85 us + 1 cycle
	    movwf   cnt_hi
	    call    DETECT_LOW
	    movf    cnt_hi, F
	    btfsc   STATUS, Z
	    goto    cfv_end	    ; Low level not detected in required interval, exit

cfv_bitst   movlw   .8
	    movwf   bit		    ; Set bit counter

cfv_bitloop movlw   .30		    ; LOW level period always precedes any bit
	    movwf   cnt_hi	    ; transmission an lies in 48-55 μs range
	    call    DETECT_HI	    ;
	    movf    cnt_hi, F	    ;
	    btfsc   STATUS, Z	    ;
	    goto    read_cfv_end    ; If reach maximum length of 55 μs + 1 cycle, exit

	    movlw   .60		    ; HIGH level period can lasts from 22 to 30 μs
	    movwf   cnt_hi	    ; for logical '0' to 75-80 μs for logical '1'
	    call    DETECT_LOW	    ; Out-of-range values interpreted as end of bit
	    movf    cnt_hi, F	    ; stream.
	    btfsc   STATUS, Z	    ;
	    goto    read_cfv_end    ;

; Compare remaining time with 35 cycles
; If it is larger than 35 cycles, then we got a 1, else 0
;	    
	    movlw   .35		    ; if more than 35 cycles remaining then
	    subwf   cnt_hi, W	    ; carry bit will contain 1, else 0 (1)
	    rlf	    tmp_sr, F	    ; Shift carry bit into current byte
	    decfsz  bit, F	    ; Decrement bit counter
	    goto    cfv_bitloop	    ; ... and continue if we have more bits to process
	    
	    movlw   0xFF	    ; Invert all bits, because carry flag is
	    xorwf   tmp_sr, W	    ; inverted after compare (1)
	    movwi   FSR0++
	    goto    cfv_bitst	    ; and continue
	    
read_cfv_end
	    movlw   .8		    ; If we have incomplete byte
	    subwf   bit, W	    ; then we need to right-align bits in it
	    btfsc   STATUS, Z	    ; to gain correct result
	    goto    cfv_end	    ; else just return
	    
; We need to align bits in tmp_sr register
;	    
read_cfv_el rlf	    tmp_sr, F	    ; 
	    decfsz  bit, F	    ;
	    goto    read_cfv_el	    ;
	    movlw   0xFF
	    xorwf   tmp_sr, W
	    movwi   FSR0++
	    
cfv_end	    movlw   LOW FSR0L
	    banksel FSR0L
	    subwf   FSR0L, W
	    movwf   wptr
	    clrf    rptr
	    
	    return


; Convert 1-Wire command code into command table index	    
;
WHICH_CMD   movlw   OW_SKIP_ROM
	    subwf   tmp_sr, W
	    btfsc   STATUS, Z
	    retlw   IND_SkipROM
	    movlw   OW_MATCH_ROM
	    subwf   tmp_sr, W
	    btfsc   STATUS, Z
	    retlw   IND_MatchROM
	    movlw   OW_READ_ROM
	    subwf   tmp_sr, W
	    btfsc   STATUS, Z
	    retlw   IND_ReadROM
	    movlw   OW_SEARCH_ROM
	    subwf   tmp_sr, W
	    btfsc   STATUS, Z
	    retlw   IND_SearchROM
	    movlw   OW_SEARCH_INIT
	    subwf   tmp_sr, W
	    btfsc   STATUS, Z
	    retlw   IND_SearchINIT
	    retlw   IND_Unknown


; This is command handler routine. Replace stub goto's with goto's to actual
; command handlers. PLEASE keep this jumptable in sync with command indexes!
;	    
DO_COMMAND  call    WHICH_CMD
	    brw
	    return		    ; IND_SkipROM
	    goto    cmd_readrom	    ; IND_ReadROM
	    return		    ; IND_MatchROM
	    goto    cmd_searchrom   ; IND_SearchROM
	    goto    cmd_searchinit   ; IND_SearchINIT
; --------------------------------------------------------------------------	    
	    return		    ; IND_Unused
	    return		    ; IND_Unknown


; After issuing READ_ROM command device sends 64 bits of his address:
; 8 bits of device class, 48 bits of serial # and 8 bits of CRC
;
cmd_readrom movlw   LOW recvbuf
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH recvbuf
	    banksel FSR0H
	    movwf   FSR0H

	    movlw   .8
	    movwf   cnt_hi
	    
	    pulse_high
	    
rom_byte    call    ONEW_RECVBYTE
	    movfw   tmp_sr
	    movwi   FSR0++
	    decfsz  cnt_hi
	    goto    rom_byte
	    
	    pulse_low
	    
	    clrf    rptr
	    movlw   .8
	    movwf   wptr
	    
	    return


; Initialize device enumerator to default state
;	    
ENUM_RESET  movlw   LOW recvbuf
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH recvbuf
	    banksel FSR0H
	    movwf   FSR0H
	    clrw
	    movwi   0[FSR0]
	    movwi   1[FSR0]
	    movwi   2[FSR0]
	    movwi   3[FSR0]
	    movwi   4[FSR0]
	    movwi   5[FSR0]
	    movwi   6[FSR0]
	    movwi   7[FSR0]
	    movlw   .65
	    movwf   fork
	    clrf    rptr
	    movlw   .8
	    movwf   wptr
	    return


; Enumerate devices on specified port.
; Master must issue RESET/SEARCH_ROM commands until receiving address of all 0xFF
;	    
; (I think about using values 0xDEADBEEF 0xBAADF00D as invalid address value ;)
;	    
SEARCH_ROM  movf    fork, F
	    btfsc   STATUS, Z
	    goto    exit_none

	    movlw   .8		    ; bp = 8
	    movwf   bit		    ;

	    movlw   LOW recvbuf
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH recvbuf
	    banksel FSR0H
	    movwf   FSR0H	
	    
	    moviw   [FSR0]
	    movwf   prev	    ;
	    
	    clrf    next	    ; next = 0
	    clrf    newfork	    ; newfork = 0
	    movlw   .1		    ; p = 1
	    movwf   cnt_hi	    ;

; 
; Sample ~0 and ~1 bits from bus. No need to clear tmp_sr
;	    
enum_loop   call    ONEW_RECVB	    ;
	    rlf	    tmp_sr, F	    ; Original bit is bit #1
	    udelay  .60		    ; Delay until next time slot starts
	    
	    call    ONEW_RECVB	    ;
	    rlf	    tmp_sr, F	    ; Complement bit is bit #0
	    udelay  .60		    ; Delay until next time slot starts

	    movlw   b'00000011'
	    andwf   tmp_sr, W
	    brw
	    goto    bits_00
	    goto    bits_01
	    goto    bits_10
	    goto    enum_end
;
; '00' is conflict place
;	    
bits_00	    
	    movfw   cnt_hi	    ; if(p < onewire_enum_fork_bit) {
	    subwf   fork, W
	    btfsc   STATUS, Z	    ; p == onewire_enum_fork_bit?
	    goto    set_hibit
	    btfss   STATUS, C	    ; C set if w <= f (p < onewire_enum_fork_bit)
	    goto    save_place
;	    
; p < onewire_enum_fork_bit
;
	    btfsc   prev, 0	    ; if(prev & 1)
	    goto    set_hibit	    ;	    next |= 0x80
save_place  movfw   cnt_hi	    ; else
	    movwf   newfork	    ;	    newfork = p
	    goto    end_if
;
; '01' = 0
;	    
bits_01	    bcf	    next, 7
	    goto    end_if
;
; '10' = 1
;
bits_10	    
set_hibit   bsf	    next, 7	    ; }
	    
end_if	    call    ONEW_XMIT	    ; Take control over line
	    rlf	    next, W	    ; Shift high bit into carry flag
	    call    ONEW_SENDB
	    call    ONEW_RECV
	    udelay  .10
	    
	    decfsz  bit, F	    ; if(--bp == 0) {
	    goto    more_bits

	    movfw   next
	    movwi   [FSR0]	    ; *pprev = next
	    clrf    next	    ; next = 0
	    
	    movlw   .64		    ; if p >= 64 exit for(;;)
	    subwf   cnt_hi, W	    
	    btfsc   STATUS, C
	    goto    enum_end
	    
	    moviw   ++FSR0	    ; ++pprev; prev = *pprev;
	    movwf   prev
	    
	    movlw   .8		    ; bp = 8
	    movwf   bit
	    goto    for_end
	    
more_bits   movlw   .64		    ; if p >= 64 exit for(;;)
	    subwf   cnt_hi, W	    
	    btfsc   STATUS, C
	    goto    enum_end

	    rrf	    next, F	    ; next >>= 1
	    rrf	    prev, F	    ; prev >>= 1
	    
for_end	    incf    cnt_hi, F	    ; p++
	    goto    enum_loop	    ; } // for(;;)

exit_none   movlw   LOW recvbuf
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH recvbuf
	    banksel FSR0H
	    movwf   FSR0H	    
	    movlw   0xFF
	    movwi   0[FSR0]
	    movwi   1[FSR0]
	    movwi   2[FSR0]
	    movwi   3[FSR0]
	    movwi   4[FSR0]
	    movwi   5[FSR0]
	    movwi   6[FSR0]
	    movwi   7[FSR0]
	    goto    enum_ret
	    
; Copy discovered 1-wire address to send buffer
;	    
enum_end    movfw   newfork	    ; onewire_enum_fork_bit = newfork
	    movwf   fork
	    
enum_ret    clrf    rptr
	    movlw   .8
	    movwf   wptr
	    return	    

cmd_searchinit	    
	    call    ENUM_RESET
	    return
	    
cmd_searchrom
	    pulse_high
	    call    SEARCH_ROM
	    pulse_low
	    return
	    
;
; ******** MAIN ENTRY POINT ********
;
START
   
; 32 MHZ: PLL Enable +  8MHZ + CLOCL SR C= CONFIG1
;
	    movlw   0xF0
	    banksel OSCCON
	    movwf   OSCCON

; Initial configuration and initialization
;
	    call    SETUP_I2C
	    
; Enable weak pull-up resistors for 1-wire output lines RC5, RC4, RA2, RC2
;
	    movlw   b'00111100' ; WPUC2,3,4,5
	    banksel WPUC
	    movwf   WPUC
	    movlw   b'00010100'	; WPUA2,4
	    banksel WPUA
	    movwf   WPUA
	    banksel OPTION_REG
	    bcf	    OPTION_REG, NOT_WPUEN
	    
; Enable interrupts
;
	    banksel PIE1
	    bsf	    PIE1, SSP1IE

	    banksel INTCON
	    bsf	    INTCON, PEIE
	    bsf	    INTCON, GIE

; Make RA5 output line
; RA5 is used to PIC - CPU synchronization instead of Clock Stretching
;
	    banksel TRISA
	    bcf	    TRISA, TRISA5
	    
	    banksel PORTA
	    bsf	    PORTA, RA5
	    
	    clrf    chnl_int
	    clrf    tmp_int
	    clrf    chnl_sr
	    clrf    tmp_sr
	    clrf    flags
	    clrf    rptr
	    clrf    wptr

	    pulse_low

; Main program loop
;
main_loop
	    
chk_reset   btfss   flags, ResetRq
	    goto    chk_rd2

	    bcf	    flags, ResetRq

	    btfsc   flags, Resetted
	    goto    main_loop

	    bsf	    flags, Resetted
	    
	    movfw   chnl_int
	    movwf   chnl_sr

	    pulse_high
	    
	    call    ONEW_XMIT	    ; Take control over line
	    call    ONEW_LOW	    ; Set line to LOW
	    udelay .250		    ; 500 μs delay
	    udelay .250		    ; (480-960 μs allowed by standard)
	    call    ONEW_RECV	    ; Allow line to float
    	    udelay .100		    ; Device must pull line low after 15-60 μs 
				    ; interval and hold it low 60-240 μs
				    ; This is called 'Presence Pulse'
	    call    ONEW_SAMPLE	    ; Sample line to determine device presence pulse
	    btfsc   STATUS, C	    ; ACK or NACK depending on presence pulse
	    goto    no_reply
	    
	    udelay  .250	    ; Perform necessary delays as stated
	    udelay  .250	    ; in 1-Wire protocol specification
	    banksel SSP1CON2
	    bcf	    SSP1CON2, ACKDT ; ACKNOWLEDGE
	    goto    reset_cont
	    
no_reply
	    banksel SSP1CON2
	    bsf	    SSP1CON2, ACKDT ; NOT ACKNOWLEDGE
	    
reset_cont	    
	    banksel SSP1CON
	    bsf	    SSP1CON, CKP    ; Release SCL line to allow master to continue
	    
	    pulse_low

	    goto    continue

	    
chk_rd2	    btfss   flags, Read2Rq
	    goto    chk_read
	
	    bcf	    flags, Read2Rq

	    movfw   chnl_int
	    movwf   chnl_sr

	    pulse_high
	    call    READ_CFV
	    pulse_low
	    
	    goto    continue

	    
chk_read    btfss   flags, ReadRq   ; Check if 'READ REQUEST' bit is set
	    goto    chk_write	    ; if no, skip reading

	    movfw   chnl_int	    ; Select channel
	    movwf   chnl_sr

	    movf    wptr, F
	    btfsc   STATUS, Z	    ; Z != 0?
	    goto    buf_empty	    ; No, proceed sending byte from tmp_sr
	    
	    movfw   wptr
	    subwf   rptr, W
	    btfsc   STATUS, Z
	    goto    buf_empty

	    call    GET_BYTE
	    goto    i2c_send

buf_empty   
	    pulse_high		    ; Set master wait flag
	    
	    call    ONEW_XMIT	    ; Take control over line
	    call    ONEW_LOW	    ; Issue LOW pulse
	    udelay  .8		    ; 5-10 μs delay
	    call    ONEW_RECVBYTE   ; Perform read

	    pulse_low		    ; Clear master wait flag

i2c_send	    
	    movfw   tmp_sr
	    banksel SSP1BUF
	    movwf   SSP1BUF

	    banksel SSP1CON
	    bsf	    SSP1CON, CKP    ; Release SCL line to allow master to continue

	    movlw   .55
	    movwf   tmp_sr
	    movwf   tmp_int
	    
	    bcf	    flags, ReadRq
	    bsf	    flags, ByteSent ; Singal IRQ handler to check ACKSTAT flag

	    goto    continue
	    
chk_write   btfss   flags, WriteRq
	    goto    continue
	    
	    pulse_high

	    movfw   chnl_int
	    movwf   chnl_sr
	    
	    movfw   tmp_int
	    movwf   tmp_sr
	    
	    call    ONEW_SENDBYTE   ; Send byte
	    bcf	    flags, WriteRq

	    pulse_low
	    
	    btfss   flags, CmdByte
	    goto    main_loop
	    
	    movfw   tmp_int
	    movwf   tmp_sr
	    call    DO_COMMAND
	    bcf	    flags, CmdByte

continue    goto    main_loop
	    
	    
; I2C slave initalization
;
SETUP_I2C
	    movlw   0x00
	    banksel SSP1BUF
	    movwf   SSP1BUF

	    banksel SSP1STAT
	    movlw   0x00
	    movwf   SSP1STAT
	    
	    movlw   I2C_ADDR	    ; NODE_ADDR, must be taken from EEPROM
	    banksel SSP1ADD
	    movwf   SSP1ADD
	    
	    banksel SSP1MSK
	    movlw   b'00011110'	    ; Three high bits of address don't care
	    movwf   SSP1MSK
	    
	    banksel SSP1CON1
	    movlw   0x2E	    ; I2C Slave mode, 7-bit address with Start and Stop bit interrupts enabled
	    movwf   SSP1CON1

	    banksel SSP1CON2
	    bcf	    SSP1CON2, SEN   ; Enable clock stretching for slave
	    bsf	    SSP1CON2, ACKDT
	    bsf	    SSP1CON2, GCEN  ; General call enable
	    
	    banksel SSP1CON3
	    bcf	    SSP1CON3, AHEN  ; Address hold disabled
	    bcf	    SSP1CON3, DHEN  ; Data hold disabled
	    bcf	    SSP1CON3, PCIE  ; Stop condition interrupt enabled
	    bcf	    SSP1CON3, SCIE  ; Start condition interrupt enabled
	    bsf	    SSP1CON3, BOEN  ; 
	    
	    banksel PIR1
	    bcf	    PIR1, SSP1IF 

	    banksel PORTC
	    clrf    PORTC
	    banksel LATC
	    clrf    LATC
	    banksel ANSELA
	    clrf    ANSELA
	    banksel ANSELC
	    clrf    ANSELC
        
	    banksel TRISC
	    movlw   b'00111111'
	    movwf   TRISC

	    banksel APFCON0
	    movlw   b'11101000'
	    movwf   APFCON0
	    
	    banksel APFCON1
	    movlw   b'00001111'
	    movwf   APFCON1
	    
	    return
	    
	    
; Interrupt service routine code
;
INTR	    				    
	    banksel PIR1	    ; Clear SSP1IF flag, allowing to receive
	    bcf	    PIR1, SSP1IF    ; next portion of data from I2C

	    btfss   flags, ByteSent ; Was byte sent previously?
	    goto    not_sent
	    
	    banksel SSP1CON2
	    btfsc   SSP1CON2, ACKSTAT	; Parent ~acknowledges last byte?
	    goto    eob		    ; Nope. No more data required to send
	    bsf	    flags, ReadRq   ; Or yes? Then just set read-request flag
	    retfie
	    
eob	    bcf	    flags, ByteSent ; No more data to send
	    goto    intr_end
	    
not_sent
	    banksel SSP1STAT
	    btfss   SSP1STAT, S	    ; Start bit set?
	    goto    check_stop	    ; If no, continue

; FIXME: Aw. Nothing to do.	    
;	    bsf	    flags, ResetRq 
	    bsf	    flags, CmdByte  ; First byte flag
	    goto    do_rdwr
	    
; HACK: Stop bit processing code works incorrectly and was removed
;
check_stop
	    banksel SSP1STAT
	    btfss   SSP1STAT, P	    ; Stop bit set?
	    goto    do_rdwr	    ; No, pass along

	    clrf    flags	    ; Clear all flags, preparing to new I2C cycle
	    goto    intr_end
	    
; D/~A indicates last byte status: data (bit is set) or address (bit is clear)
; When address is received, examine R/~W bit to determine read or write mode
; requested by master
;
do_rdwr	    
	    banksel SSP1STAT
	    btfss   SSP1STAT, BF    ; Is receive buffer full?
	    goto    intr_end

; SSP1BUF contains byte of address or data received from master
;
	    banksel SSP1BUF
	    movfw   SSP1BUF
	    movwf   tmp_int	    ; Save current received byte value, clear BF flag
	    
	    banksel SSP1STAT
	    btfsc   SSP1STAT, D_NOT_A 
	    goto    is_read_data_req
   
; Address byte encodes 1-Wire channel number in two high bits
;
	    movwf   chnl_int	    ;
	    rlf	    chnl_int, F	    ;
	    rlf	    chnl_int, F	    ;
	    rlf	    chnl_int, W	    ;
	    andlw   b'00000011'	    ; One bit in C flag, other is MSB. Mask out excess bits
	    movwf   chnl_int	    ; Save in chnl_int

	    btfss   tmp_int, 5	    ; Bit #5 is 'FCV' flag. It valid only for read requests
	    goto    read_ord	    ; and intiates special bus handling routine
	    
	    bsf	    flags, Read2Rq  ; We need Single-Wire read to buffer
	    bsf	    flags, ReadRq   ; then send data from buffer to master
	    bcf	    flags, CmdByte  ; 'Command byte' is irrelevant here
	    retfie		    ; Don't touch CKP flag!
	    
read_ord
	    banksel SSP1STAT
	    btfss   SSP1STAT, R_NOT_W
	    goto    addr_write_req
	    bsf	    flags, ReadRq   ; 
	    bcf	    flags, CmdByte  ; Read request clears 'next byte is command byte' flag
	    retfie		    ; CKP will be set after byte is received from 1-Wire
	    
addr_write_req   
	    bsf	    flags, ResetRq  ; Bus need reset pulse before writes can occur
;	    goto    intr_end
	    retfie		    ; We releace Clock line after presence pulse detection
	    
    
is_read_data_req 
	    banksel SSP1STAT
	    btfss   SSP1STAT, R_NOT_W
	    goto    data_write_req
	    bsf	    flags, ReadRq   ; Set 'Read Request' flag
	    bcf	    flags, CmdByte  ; Read request automatically clears 'cmdbyte' flag
	    retfie
	    
; Read the message byte(s). High two bits of the message byte contains
; 1-Wire channel number, e.g. pin # (0...3), mapped to RC5, RC3, RA2, RC2
;
data_write_req
	    bsf	    flags, WriteRq  ; 

intr_end	    
	    banksel SSP1CON
	    bsf	    SSP1CON, CKP    ; Release SCL line to allow master to continue
	    retfie		    ; Return from ISR to main program


	    END
	    