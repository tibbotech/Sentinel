#include "p16F1824.inc"
#include "macros.inc"

	    list

; CONFIG1
; __config 0x3FC4
 __CONFIG _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _PWRTE_OFF & _MCLRE_ON & _CP_OFF & _CPD_OFF & _BOREN_ON & _CLKOUTEN_OFF & _IESO_ON & _FCMEN_ON
; CONFIG2
; __config 0x3FFF
 __CONFIG _CONFIG2, _WRT_OFF & _PLLEN_ON & _STVREN_ON & _BORV_LO & _LVP_ON


#define	    I2C_ADDR	    .6	    ; Our I2C slave address
#define	    IBIT_DLY	    .10	    ; Delay in μs after transmitting each bit
#define	    IBYTE_DLY	    .50	    ; Delay in μs after transmitting each byte


; <cmd> <channel> <command_data>
; 
	    CBLOCK 0x01
	    Start
	    Flush
	    NoReset
	    ReadRq
	    ENDC
 
	    UDATA
	    
; 
; Uninitialized data section
;
xmitbuf	    res .16

	    UDATA_SHR
;	    
; These registers will be placed into common shared memory area (0x70...0x7F) to
; prevent excess 'banksel' instructions
;	    
tmp	    res 1		    ; general purpose temporary register
tmp_send    res 1
cnt	    res 1		    ; low 8 bits of general purpose counter
cnt_hi	    res 1		    ; high 8 bits of general purpose counter
bit	    res 1		    ; bit counter
chnl	    res 1		    ; selected channel # for CALL's
msg_chnl    res 1		    ; 
read_ptr    res 1		    ; buffer read pointer
write_ptr   res 1		    ; buffer write pointer
flags	    res	1		    ; Misc. flags
	    
	    IDATA		    
;
; Initialized data section
;
fw_name	    db "I2C1WG.0216.01", 0  ; Frimware ID: I2C-1W{IRE}-G{ATE}-02(feb)-16(2016).(serial)
	    
	    ORG 0xF000		    ; EEPROM start address for PIC16Fxxx MCUs

interbit_delay	    de .10
interbyte_delay	    de .50
i2c_addr	    de	I2C_ADDR	    
 
	    
RES_VECT    CODE 0x0000		    ; processor reset vector
	    goto START              ; go to program entry point

	    
INTR_VECT   CODE 0x0004		    ; Processor Interrupt vector
	    goto INTR		    ; go to ISR


MAIN_PROG   CODE		    ; let linker place main program

START
   
pulse	    MACRO
	    banksel TRISA
	    bcf	    TRISA, TRISA5
	    banksel PORTA
	    bcf	    PORTA, RA5
	    udelay  .50
	    banksel PORTA
	    bsf	    PORTA, RA5
	    ENDM

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
	    
	    clrf    msg_chnl

; Enable interrupts
;
	    banksel PIE1
	    bsf	    PIE1, SSP1IE

	    banksel INTCON
	    bsf	    INTCON, PEIE
	    bsf	    INTCON, GIE

; Make RA5 output line
;	    
	    banksel TRISA
	    bcf	    TRISA, TRISA5
	    banksel PORTA
	    bsf	    PORTA, RA5

	    clrf    read_ptr
	    clrf    write_ptr
	    clrf    msg_chnl
	    clrf    flags
	    clrf    tmp
	    
main_loop   btfss   flags, ReadRq   ; Check if 'READ REQUEST' bit is set
	    goto    chk_write	    ; if no, skip reading
	    
	    ow_xmit 0		    ; Set pin as output pin
	    ow_low  0		    ; Pull line low
	    udelay  .10		    ; 10 μs delay
;	    ow_high 0
	    ow_recv 0		    ; Set pin as input
	    movfw   msg_chnl	    ; Load channel #
	    call    ONEW_RECVBYTE   ; Perform read
	    bcf	    flags, ReadRq   ; Clear read request bit
	    call    I2C_WRITE_BACK  ; Send readed byte back to master
	    goto    main_loop
	    
chk_write   movfw   write_ptr
	    subwf   read_ptr, W
	    btfsc   STATUS, C	    ; xwr(w) < write_ptr(f)
	    goto    main_loop	    ; Waiting for data to write or requests for read
	    
; Write pointer not null - we have something to write to 1-Wire bus	    
;
	    movfw   read_ptr
	    btfss   STATUS, Z
	    goto    simple_bytes

;	    btfsc   flags, NoReset  ; Restart don't need reset pulse
;	    goto    no_reset
	    
; Perform 1-Wire reset for zero byte
;	    
	    movfw   msg_chnl	    ; Load channel #
	    call    ONEW_RESET	    ; Initiate reset pulse
	    udelay  .250	    ; Perform necessary delays as stated
	    udelay  .250	    ; in 1-Wire protocol specification

no_reset	    
	    bcf	    flags, NoReset
	    
simple_bytes
	    movlw   LOW xmitbuf
	    addwf   read_ptr, W
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH xmitbuf
	    banksel FSR0H
	    movwf   FSR0H
	    banksel xmitbuf
	    movfw   INDF0
	    movwf   tmp_send
	    incf    read_ptr, W	    ; Increment read pointer
	    andlw   b'00001111'	    ; Wrap around buffer size
	    movwf   read_ptr

	    movfw   msg_chnl	    ; Load channel #
	    ow_xmit 0
	    call    ONEW_SENDBYTE   ; Send byte
	    ow_recv 0
	    udelay  .50		    ; Perform inter-byte delay

	    goto    main_loop

	    
I2C_WRITE_BACK
	    nop
	    clrw
	    return
	    
; Perform 1-wire master reset routine
;	    
; On entry, WREG contains channel number 0...3 to reset
; Returns hardware presence flag: if any of the hardware is connected,
; 'C' flag of STATUS reg is set (presence pulse detected). 'C' is clear
; if bus is free of devices
;	    
ONEW_RESET  andlw   b'00000011'
	    brw
	    goto    _reset0
	    goto    _reset1
	    goto    _reset2

_reset3
	    ow_xmit 3		    ; Take control over line
	    ow_low 3		    ; Set line to LOW
	    udelay .250		    ; 500 μs delay
	    udelay .250		    ; (480-960 μs allowed by standard)
	    ow_recv 3		    ; Allow line to float
    	    udelay .100		    ; Device must pull line low after 15-60 μs 
				    ; interval and hold it low 60-240 μs
				    ; This is called 'Presence Pulse'
	    bcf STATUS, C
	    banksel PORTC	    ; Sample line
	    btfsc PORTC, RC2	    ; RC2
	    bsf STATUS, C
	    return

_reset0	    
	    ow_xmit 0		    ; Take control over line
	    ow_low 0		    ; Set line to LOW
	    udelay .250		    ; 500 μs delay
	    udelay .250		    ; (480-960 μs allowed by standard)
	    ow_recv 0		    ; Allow line to float
    	    udelay .100		    ; Device must pull line low after 15-60 μs 
				    ; interval and hold it low 60-240 μs
				    ; This is called 'Presence Pulse'
	    bcf STATUS, C
	    banksel PORTA	    ; Sample line
	    btfsc PORTA, RA5	    ; RA5
	    bsf STATUS, C
	    return
	    
_reset1
	    ow_xmit 1		    ; Take control over line
	    ow_low 1		    ; Set line to LOW
	    udelay .250		    ; 500 μs delay
	    udelay .250		    ; (480-960 μs allowed by standard)
	    ow_recv 1		    ; Allow line to float
    	    udelay .100		    ; Device must pull line low after 15-60 μs 
				    ; interval and hold it low 60-240 μs
				    ; This is called 'Presence Pulse'
	    bcf STATUS, C
	    banksel PORTC	    ; Sample line
	    btfsc PORTC, RC3	    ; RC3
	    bsf STATUS, C
	    return

_reset2
	    ow_xmit 2		    ; Take control over line
	    ow_low 2		    ; Set line to LOW
	    udelay .250		    ; 500 μs delay
	    udelay .250		    ; (480-960 μs allowed by standard)
	    ow_recv 2		    ; Allow line to float
    	    udelay .100		    ; Device must pull line low after 15-60 μs 
				    ; interval and hold it low 60-240 μs
				    ; This is called 'Presence Pulse'
	    bcf STATUS, C
	    banksel PORTA	    ; Sample line
	    btfsc PORTA, RA2	    ; RA2
	    bsf STATUS, C
	    return

	    
; Sends one bit, specified by C flag in STATUS reg to 1-Wire channel,
; specified by WREG (0...3)
; No error checking done
;	    
; Timings for sending '0's and '1's: (60μs total length + 10μs inter-bit period)
;
; 0: LOW, 60μs delay, HIGH, 10μs delay
; 1: LOW, 6μs delay, HIGH, 64μs delay
;	    
ONEW_SENDBIT
;	    banksel STATUS
	    brw			    ; PC = PC+1+W
	    goto _send_bit_0	    ; CH 0 - IO pins RA4, RC5
	    goto _send_bit_1	    ; CH 1 - IO pins RC3, RC4
	    goto _send_bit_2	    ; CH 2 - IO pin RA2
	    
_send_bit_3
	    btfsc STATUS, C	    
	    goto _send_one_3
	    
	    ow_low 3
	    udelay .60		    
	    ow_high 3
	    udelay .10		    
	    return

_send_one_3
	    ow_low 3
	    udelay .6		    
	    ow_high 3
	    udelay .64		    
	    return

_send_bit_0
	    btfsc STATUS, C
	    goto _send_one_0

	    ow_low 0
	    udelay .60
	    ow_high 0
	    udelay .10
	    return

_send_one_0
	    ow_low 0
	    udelay .6
	    ow_high 0
	    udelay .64
	    return

_send_bit_1
	    btfsc STATUS, C
	    goto _send_one_1
	    
	    ow_low 1
	    udelay .60
	    ow_high 1
	    udelay .10
	    return

_send_one_1
	    ow_low 1
	    udelay .6
	    ow_high 1
	    udelay .64
	    return

_send_bit_2
	    btfsc STATUS, C
	    goto _send_one_2
	    
	    ow_low 2
	    udelay .60
	    ow_high 2
	    udelay .10
	    return

_send_one_2
	    ow_low 2
	    udelay .6	
	    ow_high 2
	    udelay .64	
	    return
	    
	    
; Sends one byte to selected 1-wire channel
; Byte in tmp, channel in WREG
;	    
; ow_xmit must be called before transmitting sequence of bytes on selected channel
; ow_recv must be called after to allow slave device to answer master
;	    
ONEW_SENDBYTE
	    andlw b'00000011'	    ; Mask excess bits of WREG to prevent invalid indexes
	    movwf chnl		    ; Save channel number for future use
	    movlw .8		    ; Bit count = 8
	    movwf bit		    ; Save bit count in counter
_sndbit	    movfw chnl		    ; Load channel number into WREG
	    rrf tmp_send, F	    ; Shift rightmost (less significant) bit into 'C'
	    call ONEW_SENDBIT	    ; 'C' = bit value, WREG = channel number
	    decfsz bit, F	    ; Decrement bit count
	    goto _sndbit	    ; Loop, if not zero
	    return
	    

; Samples one bit from specified 1-wire channel
; On entry WREG must contain 1-wire channel number 0...3
; On exit carry flag contains sampled bit
;	    
ONEW_RECVBIT
	    brw
	    goto _recv_bit_0
	    goto _recv_bit_1
	    goto _recv_bit_2

_recv_bit_3
	    ow_xmit 3
	    ow_low 3
	    udelay .2
	    ow_high 3
	    ow_recv 3
	    udelay .8
	    bcf STATUS, C
	    banksel PORTC
	    btfsc PORTC, RC2
	    bsf STATUS, C
	    udelay .80			; Carry bit unaffected
	    return

_recv_bit_0
	    ow_xmit 0
	    ow_low 0
	    udelay .2
	    ow_high 0
	    ow_recv 0
	    udelay .8
	    bcf STATUS, C
	    banksel PORTA
	    btfsc PORTA, RA4
	    bsf STATUS, C
	    udelay .80			; Carry bit unaffected
	    return
	    
_recv_bit_1
	    ow_xmit 1
	    ow_low 1
	    udelay .2
	    ow_high 1
	    ow_recv 1
	    udelay .8
	    bcf STATUS, C
	    banksel PORTC
	    btfsc PORTC, RC3
	    bsf STATUS, C
	    udelay .80			; Carry bit unaffected
	    return
	    
_recv_bit_2
	    ow_xmit 2
	    ow_low 2
	    udelay .2
	    ow_high 2
	    ow_recv 2
	    udelay .8
	    bcf STATUS, C
	    banksel PORTA
	    btfsc PORTA, RA2
	    bsf STATUS, C
	    udelay .80			; Carry bit unaffected
	    return

    
	    
; Receives one byte from slave device
;	    
; On input WREG must contain 1-Wire channel number 0...3
; On output WREG contains byte read from specified channel
;
ONEW_RECVBYTE
	    andlw   b'00000011'		; Mask bits except 0,1
	    movwf   chnl		; Save channel ID
	    movlw   .8			; Bit count to receive
	    movwf   bit			; ... save in counter
	    clrf    tmp			; Clear bit accumulation register
	    INT_DISABLE			; Disable interrupts
recv_bits   movfw   chnl		; Load channel #
	    call    ONEW_RECVBIT	; Receive one bit
	    rlf	    tmp, F		; Bits in order: low to high
	    decfsz  bit, F		; Decrement bit count...
	    goto    recv_bits		; ... and repeat, if needed
	    INT_ENABLE			; Enable interrupts
	    movfw   tmp			; Load result into WREG
	    return
	    

; Samples specified bus
; Returns C=0 when bus is low and C=1 when bus is high
;	    
SAMPLE_BUS  bcf	    STATUS, C
	    brw
	    goto    _smpl_c0
	    goto    _smpl_c1
	    goto    _smpl_c2
; Sample channel 3
	    banksel PORTC
	    btfsc   PORTC, RC2
	    bsf	    STATUS, C
	    goto    _sbitret
; Sample channel 0	    
_smpl_c0
	    banksel PORTA
	    btfsc   PORTA, RA4
	    bsf	    STATUS, C
	    goto    _sbitret
; Sample channel 1	    
_smpl_c1    
	    banksel PORTC
    	    btfsc   PORTC, RC3
	    bsf	    STATUS, C
	    goto    _sbitret
; Sample channel 2
_smpl_c2
	    banksel PORTA
	    btfsc   PORTA, RA2
	    bsf	    STATUS, C	    
_sbitret    return

	    
DELAY_1MS   
	    return

	    
; This is byte recieve subroutine for fucking Chinese version of 1-wire (called
; Single Wire)
;	    
; Specifications for this pain-in-the-ass protocol you can see in AM2301 sensor
; documentation.
;	    
; In short:
;	    
; Any transmitted bit starts with bus low level period with length about of 45-60 μs
; Then, 0's denoted by 25-32 μs high bus level peak, and 1's by 75-85 μs peak
;	    
ONEW_RECVBYTE_FCV
	    andlw   b'00000011'		; Mask bits except 0,1
	    movwf   chnl		; Save channel ID
	    movlw   .8			; Bit count to receive
	    movwf   bit			; ... save in counter
	    clrf    tmp			; Clear bit accumulation register
;	    
; We begin to sample bus to catch pulldown by slave device
;
; TODO: It is possible to hang inside this cycle forever. To prevent this
; we need to implement some sort of time counter
;	    
_islow_fcv  movfw   chnl		; Load channel #
	    call    SAMPLE_BUS
	    btfsc   STATUS, S
	    goto    _islow_fcv
;	    
; Found low level latch. Now sample bus until it goes high and determine 
; length of sampled area
;	    
	    INT_DISABLE			; Disable interrupts

	    movlw   .60
	    movwf   cnt_hi		; Max 60 μs of low bus level
_low32ms    movfw   chnl
	    call    SAMPLE_BUS		; Sample selected bus
	    btfsc   STATUS, C		
	    goto    _lowchk
	    decfsz  cnt_hi, F
	    goto    _low32ms
;	    
; Counter counted to zero: this indicates that latch length is ms exceeded
;	    
	    bsf	    STATUS, C		; Indicate error condition
	    INT_ENABLE
	    return

; Compare value remaining in counter with margins
;	    
_lowchk
	    INT_ENABLE
	    return
	    
	    
; Load configuration data from EEPROM
;	    
LOAD_CONFIG
	    return
	    

; TODO: FIXME!!!
;
; Routine in this form won't work at all because direct writes to EEPROM
; is prohibited by hardware.
;	    
DEFAULT_CONFIG
	    movlw IBIT_DLY
	    banksel interbit_delay
	    movwf interbit_delay
	    movlw IBYTE_DLY
	    banksel interbyte_delay
	    movwf interbyte_delay
	    movlw I2C_ADDR
	    banksel i2c_addr
	    movwf i2c_addr
	    return
	    
	    
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
	    movlw   b'00011110'	    ; Two high bits of address don't care
	    ;movlw   b'11111111'	    ; Two high bits of address don't care
	    movwf   SSP1MSK
	    
	    ;banksel PIR1
	    ;bcf	    PIR1, SSP1IF
	    
	    banksel SSP1CON1
	    movlw   0x2E	    ; I2C Slave mode, 7-bit address with Start and Stop bit interrupts enabled
	    ;movlw   b'00100110'
	    movwf   SSP1CON1

	    banksel SSP1CON2
	    bcf	    SSP1CON2, SEN   ; Enable clock stretching for slave
	    bsf	    SSP1CON2, ACKDT
	    bsf	    SSP1CON2, GCEN  ; General call enable
	    
	    banksel SSP1CON3
	    bcf	    SSP1CON3, AHEN  ; Address hold disabled
	    bcf	    SSP1CON3, DHEN  ; Data hold enabled
	    bcf	    SSP1CON3, PCIE  ; Stop condition interrupt enabled
	    bcf	    SSP1CON3, SCIE  ; Start condition interrupt enabled
	    bsf	    SSP1CON3, BOEN  ; 
	    
	    banksel PIR1
	    bcf	    PIR1, SSP1IF 

	    banksel PORTC
	    clrf    PORTC
        
	    banksel LATC
	    clrf    LATC
	    
	    banksel ANSELC
	    movlw   b'0001100'
	    movwf   ANSELC
        
	    banksel TRISC
	    movlw   b'00111111'
	    movwf   TRISC

	    return
	    
	    
; Interrupt service routine code
;
INTR	    				    
	    banksel PIR1	    ; Clear SSP1IF flag, allowing to receive
	    bcf	    PIR1, SSP1IF    ; next portion of data from I2C

	    banksel SSP1STAT
	    btfss   SSP1STAT, S	    ; Start bit set?
	    goto    check_stop	    ; If no, continue
	    
	    btfsc   flags, Start    ; Is we already in 'Start' condition?
	    goto    restart	    ; If yes, go to restart condition handler 
	    
	    bsf	    flags, Start    ; Set start condition flag for restart detection
	    goto    do_rdwr
	    
; Restart condition detected.
; We need to flush write queue (if any) and prevent generation of reset/presence 
; detection pulse
; 
restart
	    bsf	    flags, NoReset  ; Disable RESET pulse
	    bsf	    flags, Flush    ; And flush all need-to-write data
	    goto    do_rdwr
	    
; Stop bit processing code unimplemented yet
;
check_stop
	    banksel SSP1STAT
	    btfss   SSP1STAT, P	    ; Stop bit set?
	    goto    do_rdwr	    ; No, pass along
	    
	    clrf    flags	    ; Clear all flags, preparing to new I2C cycle
	    clrf    read_ptr	    ; Clear read pointer
	    clrf    write_ptr	    ; Clear write pointer
	    goto    intr_end	    ;

; D/~A indicates last byte status: data (bit is set) or address (bit is clear)
; When address is received, examine R/~W bit to determine read or write mode
; requesteded by master
;
do_rdwr	    
	    banksel SSP1STAT
	    btfss   SSP1STAT, BF    ; Is receive buffer full?
	    goto    intr_end

byte_ready
	    banksel SSP1BUF
	    movfw   SSP1BUF
	    movwf   tmp		    ; Save current received byte value, clear BF flag
	    
	    banksel SSP1STAT
	    btfsc   SSP1STAT, D_NOT_A 
	    goto    is_read_req
   
; Address byte encodes 1-Wire channel number in two high bits
;
	    movwf   msg_chnl
	    rlf	    msg_chnl, F
	    rlf	    msg_chnl, F
	    rlf	    msg_chnl, W
	    andlw   b'00000011'	    ; One bit in C flag, other is MSB. Mask out excess bits
	    movwf   msg_chnl	    ; Save in msg_chnl
	    
	    clrf    msg_chnl	    ; TODO: FIXME!!!!

; On address byte arrival clear both buffer's R and W pointers, but only if 
; 'Restart' condition flag is NOT set, because restart must flush remaining
; data from buffer
;
;	    btfsc   flags, NoReset
;	    goto    intr_end
	    
	    clrf    read_ptr	    ; On address byte arrival
	    clrf    write_ptr	    ; clear read and write pointers
	    goto    intr_end	    ;

is_read_req
	    banksel SSP1STAT
	    btfss   SSP1STAT, R_NOT_W
	    goto    write_req

	    bsf	    flags, ReadRq    ; Set 'Read Request' flag
	    goto    intr_end
	    
; Read the message byte(s). High two bits of the message byte contains
; 1-Wire channel number, e.g. pin # (0...3), mapped to RC5, RC3, RA2, RC2
;
write_req   
	    movlw   LOW xmitbuf
	    addwf   write_ptr, W
	    banksel FSR0L
	    movwf   FSR0L
	    movlw   HIGH xmitbuf
	    banksel FSR0H
	    movwf   FSR0H
	    ;clrw
	    ;addwfc  FSR0H, F
	    bankisel xmitbuf
	    movfw   tmp
	    movwf   INDF0
	    incf    write_ptr, W
	    andlw   b'00001111'
	    movwf   write_ptr
	    
; Check, if 'Flush' flag is set then don't release CLK line, hold it low,
; thus preventing master to send us more I2C data
;	    
intr_end    
	    banksel SSP1CON
	    bsf	    SSP1CON, CKP

	    retfie		    ; Return from ISR to main program

	    
	    END