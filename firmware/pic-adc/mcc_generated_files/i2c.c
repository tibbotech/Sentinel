#include "i2c.h"
#include "../common.h"


#define I2C_SLAVE_ADDRESS 0x03 
#define I2C_SLAVE_MASK    0x7F


static uint8_t NameString[] = {'B','c','u','r','r',' ','v','0','0','.','0','1'}; // first symbol length // memory address 0x20


typedef enum
{
    SLAVE_NORMAL_DATA,
    SLAVE_DATA_ADDRESS,
} SLAVE_WRITE_DATA_TYPE;


volatile uint8_t I2C_slaveWriteData      = 0x55;


void I2C_StatusCallback(I2C_SLAVE_DRIVER_STATUS i2c_bus_state);


void I2C_Initialize(void)
{
    // initialize the hardware
    // R_nW write_noTX; P stopbit_notdetected; S startbit_notdetected; BF RCinprocess_TXcomplete; SMP High Speed; UA dontupdate; CKE disabled; D_nA lastbyte_address; 
    SSP1STAT = 0x00;
    // SSPEN enabled; WCOL no_collision; CKP disabled; SSPM 7 Bit Polling; SSPOV no_overflow; 
    SSP1CON1 = 0x26;
    // ACKEN disabled; GCEN disabled; PEN disabled; ACKDT acknowledge; RSEN disabled; RCEN disabled; ACKSTAT received; SEN enabled; 
    SSP1CON2 = 0x01;
    // ACKTIM ackseq; SBCDE disabled; BOEN disabled; SCIE disabled; PCIE disabled; DHEN disabled; SDAHT 100ns; AHEN disabled; 
    SSP1CON3 = 0x00;
    // SSPMSK 127; 
    SSP1MSK = (I2C_SLAVE_MASK << 1);  // adjust UI mask for R/nW bit            
    // SSPADD 3; 
    SSP1ADD = (I2C_SLAVE_ADDRESS << 1);  // adjust UI address for R/nW bit

    // clear the slave interrupt flag
    PIR1bits.SSP1IF = 0;
    // enable the master interrupt
    PIE1bits.SSP1IE = 1;

}


void I2C_ISR ( void )
{
    uint8_t     i2c_data                = 0x55;


    // NOTE: The slave driver will always acknowledge
    //       any address match.

    PIR1bits.SSP1IF = 0;        // clear the slave interrupt flag
    i2c_data        = SSP1BUF;  // read SSPBUF to clear BF
    if(1 == SSP1STATbits.R_nW)
    {
        if((1 == SSP1STATbits.D_nA) && (1 == SSP1CON2bits.ACKSTAT))
        {
            // callback routine can perform any post-read processing
            I2C_StatusCallback(I2C_SLAVE_READ_COMPLETED);
        }
        else
        {
            // callback routine should write data into SSPBUF
            I2C_StatusCallback(I2C_SLAVE_READ_REQUEST);
        }
    }
    else if(0 == SSP1STATbits.D_nA)
    {
        // this is an I2C address

        // callback routine should prepare to receive data from the master
        I2C_StatusCallback(I2C_SLAVE_WRITE_REQUEST);
    }
    else
    {
        I2C_slaveWriteData   = i2c_data;

        // callback routine should process I2C_slaveWriteData from the master
        I2C_StatusCallback(I2C_SLAVE_WRITE_COMPLETED);
    }

    SSP1CON1bits.CKP    = 1;    // release SCL

} // end I2C_ISR()



void I2C_StatusCallback(I2C_SLAVE_DRIVER_STATUS i2c_bus_state)
{
    static uint8_t WriteAddrCount = 0;
    static uint16_t WriteAddr = 0;
//    uint8_t slaveWriteType = SLAVE_NORMAL_DATA;
    
    switch (i2c_bus_state)
    {
        case I2C_SLAVE_WRITE_REQUEST:
            // the master will be sending the eeprom address next
//            slaveWriteType  = SLAVE_DATA_ADDRESS;
            break;

        case I2C_SLAVE_WRITE_COMPLETED:
            switch (WriteAddrCount)
            {
                case 0: 
                    if (I2C_slaveWriteData == 1) // READ
                        WriteAddrCount++;
                    break;

                case 1: 
                    WriteAddr = I2C_slaveWriteData;
                    WriteAddrCount++;                
                    break;

                case 2: 
                    WriteAddr = (WriteAddr << 8) | I2C_slaveWriteData;
                    WriteAddrCount++;
                    break;
                    
                default: 
                    WriteAddrCount = 0;
                    break;                    
            }
            break;

        case I2C_SLAVE_READ_REQUEST:
            if(WriteAddr >= 0x0020 && WriteAddr < (0x0019 + 0x000B * 2)) { 
                SSP1BUF = NameString [WriteAddr - 0x0020];
            } else {
                switch(WriteAddr) {
                    
                    case 0x0060: 
                        SSP1BUF = (uint8_t)(adc_channels[0].res >> 8);  
                        break;
                    case 0x0061: 
                        SSP1BUF = (uint8_t)(adc_channels[0].res);       
                        break;
                    case 0x0062: 
                        SSP1BUF = (uint8_t)(adc_channels[1].res >> 8);  
                        break;
                    case 0x0063: 
                        SSP1BUF = (uint8_t)(adc_channels[1].res);       
                        break;
                    case 0x0064: 
                        SSP1BUF = (uint8_t)(adc_channels[2].res >> 8);  
                        break;
                    case 0x0065: 
                        SSP1BUF = (uint8_t)(adc_channels[2].res);       
                        break;
                    case 0x0066: 
                        SSP1BUF = (uint8_t)(adc_channels[3].res >> 8);  
                        break;
                    case 0x0067: 
                        SSP1BUF = (uint8_t)(adc_channels[3].res); 
                        break;
                }
                ++WriteAddr;
            }
            
            break;

        case I2C_SLAVE_READ_COMPLETED:
            WriteAddrCount = 0;
            WriteAddr = 0;
            break;
        default:;

    } // end switch(i2c_bus_state)

}

