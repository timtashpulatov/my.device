#include <exec/types.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <libraries/dos.h>
#include <libraries/configvars.h>

#include <clib/exec_protos.h>
#include <clib/expansion_protos.h>

#include <stdio.h>
#include <stdlib.h>

#include "dm9000.h"

struct Library *ExpansionBase = NULL;
struct ConfigDev *myCD;
APTR base_index = 0;
APTR base_data = 0;

UBYTE rx_data [2048];




/************************************************************
 * dump_packet
 ************************************************************/
void dump_packet (UWORD len) {
UWORD i, col;
UBYTE c;

    i = 0;
    while (i < len) {
        printf ("\n");
        
        for (col = 0; col < 16; col++)
            printf ("%.2x ", rx_data [i + col]);
            
        printf ("   ");

        for (col = 0; col < 16; col++) {
            c = rx_data [i + col];
            if ((c < 0x20) || (c > 0x7f))
                c = '.';
            printf ("%c", c);
        }

        i += 16;
    }



}


/************************************************************
 * peek
 ************************************************************/
UBYTE peek (APTR addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
//    ptr = (ULONG *)0x40000000;
//    value = *ptr;

    ptr = (ULONG *)addr;
    value = *ptr;

    return value & 0xff;
}

/************************************************************
 * peek_w
 ************************************************************/
UWORD peek_w (APTR addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
//    ptr = (ULONG *)0x40000000;
//    value = *ptr;

    ptr = (ULONG *)addr;
    value = *ptr;

    return value & 0xffff;
}

/************************************************************
 * poke
 ************************************************************/
void poke (APTR addr, UBYTE value) {
//UBYTE *ptr;
ULONG *ptr;

    // Dummy read
//    ptr = (ULONG *)0x40000000;
//    dummy = *ptr;


    ptr = (ULONG *) addr + 16;          // anti caching hack
    *ptr = (ULONG)value;
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
UBYTE attempts = 8;

    while (1) {
        status = dm9k_read (EPCR);
        if ((status & EPCR_ERRE) == 0)
            break;
        
        Delay (1);
        
        if (attempts == 0)
            break;
        
        attempts --;
            
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
    dm9k_write (NCR, NCR_RST | NCR_LBK_MAC);
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


UWORD ntohw (UWORD val) {
    return ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);
}







/************************************************************
 * main
 ************************************************************/
int main (int argc, char *argv []) {
UWORD m,i;
UBYTE p,f,t;
UWORD mac1, mac2, mac3;


    printf ("\nOpening expansion library... ");
    if ((ExpansionBase = OpenLibrary ("expansion.library", 0L)) == NULL)
        exit (-1);

    printf ("done.\nSearching for board... ");

    myCD = NULL;

    while (myCD = FindConfigDev (myCD, 5030, 24)) { /* search for all ConfigDevs */

       	t = myCD->cd_Rom.er_Type;
    	m = myCD->cd_Rom.er_Manufacturer;
    	p = myCD->cd_Rom.er_Product;
    	f = myCD->cd_Rom.er_Flags;
    	i = myCD->cd_Rom.er_InitDiagVec;

       	printf("found.\n"); //, myCD->cd_BoardAddr);
       	break;
	}


    if (myCD) {
        base_index = myCD->cd_BoardAddr;
        base_data = (UBYTE *)(myCD->cd_BoardAddr) + 4;
        
//        poke (base_index, 0x38);
//        poke (base_data, 0x40);
        
        dm9k_write (BUSCR, 0x40);      // set drive current strength
        
//        printf ("INDEX reg: %.2x\n", peek (base_index));
//        printf ("DATA reg: %.2x\n", peek (base_data));
        
        // Read Vendor and Product ID
        printf ("Vendor ID: %.2x%.2x\n", dm9k_read (VIDH), dm9k_read (VIDL));
        printf ("Product ID: %.2x%.2x\n", dm9k_read (PIDH), dm9k_read (PIDL));
        printf ("Chip revision: %.2x\n", dm9k_read (CHIPR));
        printf ("I/O mode: %d-bit\n", (dm9k_read (ISR) & ISR_IOMODE) ? 8 : 16);

        // Other registers
        


        if ((argc > 2) && (strcmp (argv [1], "-mac") == 0)) {
                ULONG w1, w2, w3;

                w1 = w2 = w3 = 0xffff;

                // 01234567890123456
                // aa:bb:cc:dd:ee:ff
                // aabbccddeeff

                if (strlen (argv [2]) == 12) {

                    sscanf (argv [2] + 0, "%4X", &w1);
                    sscanf (argv [2] + 4, "%4X", &w2);
                    sscanf (argv [2] + 8, "%4X", &w3);

 //                   printf ("\n w1 = %x, w2 = %x, w3 = %x\n", w1, w2, w3);
                }



            // Write EEPROM
            printf ("\nWriting EEPROM... ");
            dm9k_write_eeprom (0, w1);
            dm9k_write_eeprom (2, w2);
            dm9k_write_eeprom (4, w3); 
            printf ("done.\n");
        }
        

        // Read EEPROM
        printf ("\nReading EEPROM\n");
        mac1 = dm9k_read_eeprom (0);
        mac2 = dm9k_read_eeprom (2);
        mac3 = dm9k_read_eeprom (4);
        printf ("MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac1 >> 8, mac1 & 0xff, mac2 >> 8, mac2 & 0xff, mac3 >> 8, mac3 & 0xff);
        printf ("Auto Load Control: %.4x\n", dm9k_read_eeprom (6));
        printf ("Vendor ID: %.4x  Product ID: %.4x\n", dm9k_read_eeprom (8), dm9k_read_eeprom (10));
        printf ("Pin control: %.4x\n", dm9k_read_eeprom (12));
        printf ("Wake-up mode control: %.4x\n", dm9k_read_eeprom (14)); 

  
        



        
    }
    else {
        printf ("not found.");
    }

/*
        // Parameters
        printf ("\nParams: %d", argc);
        { UBYTE i = 0;

            while (i < argc) {
                printf ("\n   Param %d %s", i, argv [i]);
                i ++;
            }
        }

*/







    printf ("\n");


  return 0;
}


