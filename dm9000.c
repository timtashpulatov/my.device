#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/errors.h>
#include <exec/types.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <libraries/dos.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <devices/timer.h>
#include <hardware/intbits.h>

#include "devices/sana2.h"
#include "devices/sana2specialstats.h"


#include "device.h"
#include "dm9000.h"


UWORD ntohw (UWORD val) {
    return ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);
}


//         //poke (unit->io_base + 0x4000, peek (unit->io_base + 0x4000) | TESTREG_IRQ);

void SetTestRegister (UBYTE *addr, UBYTE val) {
    poke (addr + 0x4000, peek (addr + 0x4000) | val);
}


void ClearTestRegister (UBYTE *addr, UBYTE val) {
    poke (addr + 0x4000, (peek (addr + 0x4000)) & ~val);
}



/************************************************************
 * peek
 ************************************************************/
/*
UBYTE peek (APTR io_addr) {
ULONG *ptr;
ULONG value;

    ptr = (ULONG *)io_addr;
    value = *ptr;

    return *ptr & 0xff;
}
*/


/************************************************************
 * peek_w
 ************************************************************/
/*
UWORD peek_w (APTR io_addr) {
ULONG *ptr;
ULONG value;

    ptr = (ULONG *)io_addr;
    value = *ptr;

    return value & 0xffff;
}
*/


/************************************************************
 * poke
 ************************************************************/
/* void poke (APTR io_addr, UBYTE value) {
UBYTE *ptr;

    ptr = (BYTE *) io_addr + 16;        // anti caching hack
    *ptr = value;
}
*/


/************************************************************
 * poke_w
 ************************************************************/
/*
void poke_w (APTR io_addr, UWORD value) {
UWORD *ptr;

    ptr = (UWORD *)((UBYTE *)io_addr + 16);        // anti caching hack
    *ptr = value;
}
*/


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
 * dm9k_read_block_w
 ************************************************************/
void dm9k_read_block_w (APTR io_addr, UBYTE reg, UWORD *dst, UWORD len) {

    // Set test register    
//    SetTestRegister (io_addr, TESTREG_BLOCK_RD);
    
    poke ((UBYTE *)io_addr, reg);
    while (len --) {
        register UWORD val;
        val = peek_w ((UBYTE *)io_addr + 4);
        *dst ++ = ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);       // ntohw
    }

    // Clear test register
//    ClearTestRegister (io_addr, TESTREG_BLOCK_RD);


}

/************************************************************
 * dm9k_write
 ************************************************************/
void dm9k_write (APTR io_addr, UBYTE reg, UBYTE value) {
    poke ((UBYTE *)io_addr, reg);
    poke ((UBYTE *)io_addr + 4, value);
}

/************************************************************
 * dm9k_write_w
 ************************************************************/
void dm9k_write_w (APTR io_addr, UBYTE reg, UWORD value) {
UWORD *ptr;

    poke ((UBYTE *)io_addr, reg);

    // replace w/direct code
    // poke_w ((UBYTE *)io_addr + 4, value);
   


    ptr = (UWORD *)((UBYTE *)io_addr + 16 + 4);        // anti caching hack
    *ptr = value;
   
}

/************************************************************
 * dm9k_write_block_w
 ************************************************************/
void dm9k_write_block_w (APTR io_addr, UBYTE reg, UWORD *src, UWORD len) {
UWORD *ptr;

    // Set test register
    SetTestRegister (io_addr, TESTREG_BLOCK_WR);


    poke ((UBYTE *)io_addr, reg);
    ptr = (UWORD *)((UBYTE *)io_addr + 16 + 4);        // anti caching hack
    
    while (len--) {
        register UWORD val;    
        
        val = *src++;
        // poke_w ((UBYTE *)io_addr + 4, ntohw (*src++));
    
        *ptr = ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);   // ntohw
        
    }

    // Clear test register
    ClearTestRegister (io_addr, TESTREG_BLOCK_WR);


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
 * dm9k_clear_bits
 ************************************************************/
void dm9k_clear_bits (APTR io_addr, UBYTE reg, UBYTE value) {
UBYTE tmp;

    poke (io_addr, reg);
    tmp = peek ((UBYTE *)io_addr + 4) & ~value;
    poke ((UBYTE *)io_addr + 4, tmp);

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
    dm9k_write (io_addr, NCR, NCR_RST | NCR_LBK_MAC);      // 
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

/************************************************************
 * dm9k_clear_interrupts
 ************************************************************/
void dm9k_clear_interrupts (APTR io_addr) {
    dm9k_write (io_addr, ISR, 0x3f);
}


/************************************************************
 * dm9k_hash_table
 ************************************************************/
void dm9k_hash_table (APTR io_addr) {
ULONG hash_val;
UWORD i, offset, hash_table [4];
    
    /* clear Hash Table 4 words */
    for (i = 0; i < 4; i++)
        hash_table [i] = 0;

    /* Broadcast Address */
    hash_table [3] = 0x8000;
    
    /* HASH Table calculating for 64-bit Multicast Address */
//    for (i = 0; i < mc_cnt; i++, mcptr = mcptr->next) {
    //    hash_val = cal_CRC( (char *)mcptr->dmi_addr, 6, 0) & 3Fh;
    //    hash_table[hash_val / 16] |= (u16) 1 << (hash_val % 16); 
//    }
    
    /* WRITE HASH Table into 8-byte MAB0~MAB7 (Multicast Address) for MAC MD Table */   
}








