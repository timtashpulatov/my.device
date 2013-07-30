#include <exec/types.h>

#include "cs8900.h"

/*****************************************************************************
 *
 * peek
 *
 *****************************************************************************/
UBYTE peek (ULONG addr) {
ULONG *ptr;
ULONG value;

    // Dummy read
    ptr = 0x40000000;
    value = *ptr;
    ptr = (ULONG *)addr;
    value = (*ptr) >> ((addr & 0x03) << 3);

    return value;
}


/*****************************************************************************
 *
 * poke
 *
 *****************************************************************************/
void poke (ULONG addr, UBYTE value) {
UBYTE *ptr;
UBYTE dummy;
    // Dummy read
    ptr = 0x40000000;
    dummy = *ptr;
    ptr = (BYTE *) addr;
    *ptr = value;
}

/*****************************************************************************
 *
 * ppPeek
 *
 *****************************************************************************/
UWORD ppPeek (UWORD pport) {
UWORD value;
    poke (0x4400000a, (UBYTE)(pport));
    poke (0x4400000b, (UBYTE)(pport >> 8));
//    value = ((UWORD)(peek (0x4400000c)) << 8) + peek (0x4400000d);
    value = ((UWORD)(peek (0x4400000c))) + ((UWORD)(peek (0x4400000d)) << 8);
    return value;
}

/*****************************************************************************
 *
 * ppPoke
 *
 *****************************************************************************/
void ppPoke (UWORD pport, UWORD value) {
    poke (0x4400000a, (UBYTE)(pport));
    poke (0x4400000b, (UBYTE)(pport >> 8));

    poke (0x4400000c, value & 0xff);
    poke (0x4400000d, (value >> 8) & 0xff);
}

