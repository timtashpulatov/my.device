#include <exec/types.h>

#include "dm9000.h"



/************************************************************
 * peek
 ************************************************************/
UBYTE peek (ULONG addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
    ptr = (ULONG *)0x40000000;
    value = *ptr;

    ptr = (ULONG *)addr;
    value = *ptr;

    return value & 0xff;
}

/************************************************************
 * peek_w
 ************************************************************/
UWORD peek_w (ULONG addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
    ptr = (ULONG *)0x40000000;
    value = *ptr;

    ptr = (ULONG *)addr;
    value = *ptr;

    return value & 0xffff;
}

/************************************************************
 * poke
 ************************************************************/
void poke (ULONG addr, UBYTE value) {
UBYTE *ptr;
UBYTE dummy;

    // Dummy read
    ptr = (ULONG *)0x40000000;
    dummy = *ptr;


    ptr = (BYTE *) addr;
    *ptr = value;
}

/************************************************************
 * dm9k_read
 ************************************************************/
UBYTE dm9k_read (UBYTE reg) {
    poke (base_index, reg);
    return peek (base_data);
}

/************************************************************
 * dm9k_read_w
 ************************************************************/
UWORD dm9k_read_w (UBYTE reg) {
    poke (base_index, reg);
    return peek_w (base_data);
}

/************************************************************
 * dm9k_write
 ************************************************************/
void dm9k_write (UBYTE reg, UBYTE value) {
    poke (base_index, reg);
    poke (base_data, value);
}

/************************************************************
 * dm9k_set_bits
 ************************************************************/
void dm9k_set_bits (UBYTE reg, UBYTE value) {
    dm9k_write (reg, dm9k_read (reg) | value);
}


/************************************************************
 * dm9k_wait_eeprom
 ************************************************************/
void dm9k_wait_eeprom () {
UBYTE status;

    while (1) {
        status = dm9k_read (EPCR);
        if ((status & EPCR_ERRE) == 0)
            break;
    }

}

/************************************************************
 * dm9k_read_eeprom
 ************************************************************/
UWORD dm9k_read_eeprom (UBYTE offset) {
    
//   dm9k_wait_eeprom ();
    
    dm9k_write (EPAR, offset);
    dm9k_write (EPCR, EPCR_ERPRR);
    
    dm9k_wait_eeprom ();
    
    dm9k_write (EPCR, 0);
    
    return ((dm9k_read (EPDRH) << 8) + dm9k_read (EPDRL));
}

/************************************************************
 * dm9k_write_eeprom
 ************************************************************/
void dm9k_write_eeprom (UBYTE offset, UWORD value) {

    dm9k_write (EPAR, offset);
    dm9k_write (EPDRH, (value >> 8) & 0xff);
    dm9k_write (EPDRL, value & 0xff);
    dm9k_write (EPCR, EPCR_WEP | EPCR_ERPRW);
    
    dm9k_wait_eeprom ();
    
    dm9k_write (EPCR, 0);    
}

/************************************************************
 * dm9k_write_phy
 ************************************************************/
void dm9k_write_phy (UBYTE offset, UWORD value) {

    dm9k_write (EPAR, offset | 0x40);
    dm9k_write (EPDRH, (value >> 8) & 0xff);
    dm9k_write (EPDRL, value & 0xff);
    dm9k_write (EPCR, EPCR_EPOS | EPCR_ERPRW);

    dm9k_wait_eeprom ();

    dm9k_write (EPCR, 0);
}

/************************************************************
 * dm9k_reset
 ************************************************************/
void dm9k_reset () {
    dm9k_write (NCR, NCR_RST);
    while (dm9k_read (NCR) & NCR_RST);
}

/************************************************************
 * dm9k_phy_reset
 ************************************************************/
void dm9k_phy_reset () {
    dm9k_write_phy (PHY_CONTROL, 0x8000);
}

/************************************************************
 * dm9k_phy_down
 ************************************************************/
void dm9k_phy_down () {
    dm9k_write (GPCR, 1);
    dm9k_write (GPR, GPR_PHYPD);
}

/************************************************************
 * dm9k_phy_up
 ************************************************************/
void dm9k_phy_up () {
    dm9k_write (GPR, 0);
}


inline UWORD ntohw (UWORD val) {
    return ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);
}

