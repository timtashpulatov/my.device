#include <exec/types.h>

#include "dm9000.h"


/************************************************************
 * peek
 ************************************************************/
UBYTE peek (APTR io_addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
//    ptr = (ULONG *)0x40000000;
//    value = *ptr;

    ptr = (ULONG *)io_addr;
    value = *ptr;

    return value & 0xff;
}

/************************************************************
 * peek_w
 ************************************************************/
UWORD peek_w (APTR io_addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
//    ptr = (ULONG *)0x40000000;
//    value = *ptr;

    ptr = (ULONG *)io_addr;
    value = *ptr;

    return value & 0xffff;
}

/************************************************************
 * poke
 ************************************************************/
void poke (APTR io_addr, UBYTE value) {
UBYTE *ptr;
UBYTE dummy;

    // Dummy read
//    ptr = (UBYTE *)0x40000000;
//    dummy = *ptr;


    ptr = (BYTE *) io_addr + 16;        // anti caching hack
    *ptr = value;
}

/************************************************************
 * dm9k_read
 ************************************************************/
UBYTE dm9k_read (APTR io_addr, UBYTE reg) {
    poke (io_addr, reg);
    return peek ((UBYTE *)io_addr + 4);
}

/************************************************************
 * dm9k_read_w
 ************************************************************/
UWORD dm9k_read_w (APTR io_addr, UBYTE reg) {
    poke (io_addr, reg);
    return peek_w ((UBYTE *)io_addr + 4);
}

/************************************************************
 * dm9k_read_block
 ************************************************************/
void dm9k_read_block (APTR io_addr, UBYTE reg, UBYTE *dst, UWORD len) {
    poke (io_addr, reg);
    while (len --)
        *dst ++ = peek ((UBYTE *)io_addr + 4);
}


/************************************************************
 * dm9k_write
 ************************************************************/
void dm9k_write (APTR io_addr, UBYTE reg, UBYTE value) {
    poke ((UBYTE *)io_addr, reg);
    poke ((UBYTE *)io_addr + 4, value);
}

/************************************************************
 * dm9k_set_bits
 ************************************************************/
void dm9k_set_bits (APTR io_addr, UBYTE reg, UBYTE value) {
UBYTE tmp;

    poke (io_addr, reg);
    tmp = peek ((UBYTE *)io_addr + 4) | value;
    poke ((UBYTE *)io_addr + 4, tmp);
    
    // dm9k_write (io_addr, reg, dm9k_read (io_addr, reg) | value);
}


/************************************************************
 * dm9k_wait_eeprom
 ************************************************************/
void dm9k_wait_eeprom (APTR io_addr) {
UBYTE status;

    while (1) {
        status = dm9k_read (io_addr, EPCR);
        if ((status & EPCR_ERRE) == 0)
            break;
    }

}

/************************************************************
 * dm9k_read_eeprom
 ************************************************************/
UWORD dm9k_read_eeprom (APTR io_addr, UBYTE offset) {
    
//   dm9k_wait_eeprom ();
    
    dm9k_write (io_addr, EPAR, offset);
    dm9k_write (io_addr, EPCR, EPCR_ERPRR);
    
    dm9k_wait_eeprom (io_addr);
    
    dm9k_write (io_addr, EPCR, 0);
    
    return ((dm9k_read (io_addr, EPDRH) << 8) + dm9k_read (io_addr, EPDRL));
}

/************************************************************
 * dm9k_write_eeprom
 ************************************************************/
void dm9k_write_eeprom (APTR io_addr, UBYTE offset, UWORD value) {

    dm9k_write (io_addr, EPAR, offset);
    dm9k_write (io_addr, EPDRH, (value >> 8) & 0xff);
    dm9k_write (io_addr, EPDRL, value & 0xff);
    dm9k_write (io_addr, EPCR, EPCR_WEP | EPCR_ERPRW);
    
    dm9k_wait_eeprom (io_addr);
    
    dm9k_write (io_addr, EPCR, 0);    
}

/************************************************************
 * dm9k_write_phy
 ************************************************************/
void dm9k_write_phy (APTR io_addr, UBYTE offset, UWORD value) {

    dm9k_write (io_addr, EPAR, offset | 0x40);
    dm9k_write (io_addr, EPDRH, (value >> 8) & 0xff);
    dm9k_write (io_addr, EPDRL, value & 0xff);
    dm9k_write (io_addr, EPCR, EPCR_EPOS | EPCR_ERPRW);

    dm9k_wait_eeprom (io_addr);

    dm9k_write (io_addr, EPCR, 0);
}

/************************************************************
 * dm9k_reset
 ************************************************************/
void dm9k_reset (APTR io_addr) {
    dm9k_write (io_addr, NCR, NCR_RST);
    while (dm9k_read (io_addr, NCR) & NCR_RST);
}

/************************************************************
 * dm9k_phy_reset
 ************************************************************/
void dm9k_phy_reset (APTR io_addr) {
    dm9k_write_phy (io_addr, PHY_CONTROL, 0x8000);
}

/************************************************************
 * dm9k_phy_down
 ************************************************************/
void dm9k_phy_down (APTR io_addr) {
    dm9k_write (io_addr, GPCR, 1);
    dm9k_write (io_addr, GPR, GPR_PHYPD);
}

/************************************************************
 * dm9k_phy_up
 ************************************************************/
void dm9k_phy_up (APTR io_addr) {
    dm9k_write (io_addr, GPR, 0);
}

/************************************************************
 * dm9000_packet_ready
 ************************************************************/
UBYTE dm9000_packet_ready (APTR io_addr) {
UBYTE tmp;

//    poke ((ULONG)io_addr, MRCMDX);
    peek ((UBYTE *)io_addr + 4);  // dummy read
    tmp = peek ((UBYTE *)io_addr + 4);

//    tmp = dm9k_read (io_addr, MRCMDX);  // dummy read
//    tmp = dm9k_read (io_addr, MRCMDX);
    return (tmp);
}


UWORD ntohw (UWORD val) {
    return ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);
}

