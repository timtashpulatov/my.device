/*
 *  DM9000 Register Definitions
 */

#define NCR         0x00    // Network Control Register
#define     NCR_RST     0x01        // Software reset and auto clear after 10us

#define NSR         0x01    // Network Status Register
#define     NSR_SPEED    0x80    // Media Speed 0:100Mbps 1:10Mbps, when Internal PHY is used. This bit has no meaning when LINKST=0
#define     NSR_LINKST   0x40    // Link Status 0:link failed 1:link OK
#define     NSR_WAKEST   0x20    // Wakeup Event Status. Clears by read or write 1 (work in 8-bit mode). This bit will not be affected after software reset
#define     NSR_RSRV1    0x10    
#define     NSR_TX2END   0x08    // TX Packet 2 Complete Status. Clears by read or write 1. Transmit completion of packet index 2
#define     NSR_TX1END   0x04    // TX Packet 1 Complete status. Clears by read or write 1. Transmit completion of packet index 1
#define     NSR_RXOV     0x02    // RX FIFO Overflow
#define     NSR_RSRV2    0x01

#define TCR         0x02    // TX Control Register
#define TSR1        0x03    // TX Status Register I
#define TSR2        0x04    // TX Status Register II

#define RCR         0x05    // RX Control Register
#define 	RCR_WTDIS    0x40    // Watchdog Timer Disable. When set, the Watchdog Timer (2048 bytes) is disabled. Otherwise it is enabled
#define 	RCR_DIS_LONG 0x20    // Discard Long Packet (Packet length is over 1522 bytes)
#define 	RCR_DIS_CRC  0x10    // Discard CRC Error Packet
#define 	RCR_ALL      0x08    // Pass All Multicast
#define 	RCR_RUNT     0x04    // Pass Runt Packet
#define 	RCR_PRMSC    0x02    // Promiscuous Mode
#define 	RCR_RXEN     0x01    // RX Enable

#define RSR         0x06    // RX Status Register
#define ROCR        0x07    // Receive Overflow Counter Register
#define BPTR        0x08    // Back Pressure Threshold Register
#define FCTR        0x09    // Flow Control Threshold Register

#define FCR         0x0a    // RX Flow Control Register
#define     FCR_TXP0     0x80    // TX Pause Packet. Auto clears after pause packet transmission completion. Set to TX pause packet with time = 0000h
#define     FCR_TXPF     0x40    // TX Pause packet. Auto clears after pause packet transmission completion. Set to TX pause packet with time = FFFFH
#define     FCR_TXPEN    0x20    // Force TX Pause Packet Enable. Enables the pause packet for high/low water threshold control
#define     FCR_BKPA     0x10    // Back Pressure Mode. This mode is for half duplex mode only. It generates a jam pattern when any packet comes and RX SRAM is over BPHW of register 8.
#define     FCR_BKPM     0x08    // Back Pressure Mode. This mode is for half duplex mode only. It generates a jam pattern when a packet�s DA matches and RX SRAM is over BPHW of register 8.
#define     FCR_RXPS     0x04    // RX Pause Packet Status, latch and read clearly
#define     FCR_RXPCS    0x02    // RX Pause Packet Current Status
#define     FCR_FLCE     0x01    // Flow Control Enable. Set to enable the flow control mode (i.e. can disable DM9000B TX function)

#define EPCR        0x0b    // EEPROM & PHY Control Register
#define     EPCR_REEP    0x20    // Reload EEPROM. Driver needs to clear it up after the operation completes
#define     EPCR_WEP     0x10    // Write EEPROM Enable
#define     EPCR_EPOS    0x08    // EEPROM or PHY Operation Select. When reset, select EEPROM; when set, select PHY
#define     EPCR_ERPRR   0x04    // EEPROM Read or PHY Register Read Command. Driver needs to clear it up after the operation completes.
#define     EPCR_ERPRW   0x02    // EEPROM Write or PHY Register Write Command. Driver needs to clear it up after the operation completes.
#define     EPCR_ERRE    0x01    // EEPROM Access Status or PHY Access Status. When set, it indicates that the EEPROM or PHY access is in progress

#define EPAR        0x0c    // EEPROM & PHY Address Register
#define EPDRL       0x0d    // EEPROM & PHY Low Byte Data Register
#define EPDRH       0x0e    // EEPROM & PHY High Byte Data Register

#define WCR         0x0f    // Wake Up Control Register (in 8-bit mode)


#define PAR1        0x10    // Physical Address Register 
#define PAR2        0x11    // 
#define PAR3        0x12    // 
#define PAR4        0x13    // 
#define PAR5        0x14    // 
#define PAR6        0x15    // 

#define MAR

#define GPCR        0x1e    // General Purpose Control Register (in 8-bit mode)

#define GPR         0x1f    // General Purpose Register
#define     GPR_PHYPD   0x01    // PHY Power Down Control 
                                //      1: power down PHY 
                                //      0: power up PHY
                                // If this bit is updated from �1� to �0�, the whole MAC Registers can not be accessed within 1ms.

#define VIDL        0x28    // Vendor ID
#define VIDH        0x29
#define PIDL        0x2a    // Product ID
#define PIDH        0x2b
#define CHIPR       0x2c    // Chip Revision

#define TCR2        0x2d    // Transmit Control Register 2
#define     TCR2_LED    0x80    // Led Mode. When set, the LED pins act as led mode 1. When cleared, the led mode is default mode 0 or depending EEPROM setting.

#define LEDCR       0x34    // LED Pin Control Register

#define BUSCR       0x38    // Processor Bus Control Register

#define INTCR       0x39    // INT Pin Control Register

#define MRCMDX      0xf0    // Memory Data Pre-Fetch Read Command Without Address Increment Register
#define MRCMDX1     0xf1    // Memory Data Read Command without Address Increment Register
#define MRCMD       0xf2    // Memory Data Read Command with Address Increment Register

#define ISR         0xfe    // Interrupt Status Register
#define 	ISR_IOMODE   0x80    // 0 : 16-bit mode 1: 8-bit mode
#define 	ISR_RSVD     0x40
#define 	ISR_LNKCHG   0x20    // Link Status Change
#define 	ISR_UDRUN    0x10    // Transmit Under-run
#define 	ISR_ROO      0x08    // Receive Overflow Counter Overflow
#define 	ISR_ROS      0x04    // Receive Overflow
#define 	ISR_PT       0x02    // Packet Transmitted
#define 	ISR_PR       0x01    // Packet Received

#define IMR         0xff    // Interrupt Mask Register
#define 	IMR_PAR      0x80    // Enable the SRAM read/write pointer to automatically return to the start address when pointer addresses are over the SRAM size. 
                             // When driver sets this bit, REG_F5 will set to 0Ch automatically
#define IMR_RSVD     0x40
#define 	IMR_LNKCHGI  0x20    // Enable Link Status Change Interrupt
#define 	IMR_UDRUNI   0x10    // Enable Transmit Under-run Interrupt
#define 	IMR_ROOI     0x08    // Enable Receive Overflow Counter Overflow Interrupt
#define 	IMR_ROI      0x04    // Enable Receive Overflow Interrupt
#define 	IMR_PTI      0x02    // Enable Packet Transmitted Interrupt
#define 	IMR_PRI      0x01    // Enable Packet Received Interrupt

/* Phy Registers */

#define PHY_CONTROL 0x00
#define PHY_STATUS  0x01
#define PHY_ID1     0x02
#define PHY_ID2     0x03

UBYTE dm9k_read (ULONG io_addr, UBYTE reg);
UWORD dm9k_read_w (ULONG io_addr, UBYTE reg);
void dm9k_write (ULONG io_addr, UBYTE reg, UBYTE value);
void dm9k_set_bits (ULONG io_addr, UBYTE reg, UBYTE value);
