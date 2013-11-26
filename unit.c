#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/errors.h>
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

#define TASK_PRIORITY 0
#define STACK_SIZE 4096
//#define MAX_TUPLE_SIZE 0xff
//#define TUPLE_BUFFER_SIZE (MAX_TUPLE_SIZE+8)


IMPORT struct ExecBase *AbsExecBase;

static struct AddressRange *FindMulticastRange (struct DevUnit *unit, ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right, struct MyBase *base);
static VOID RxInt (__reg("a1") struct DevUnit *unit);
static VOID CopyPacket (struct DevUnit *unit, struct IOSana2Req *request, UWORD packet_size, UWORD packet_type, BOOL all_read, struct MyBase *base);
static BOOL AddressFilter (struct DevUnit *unit, UBYTE *address, struct MyBase *base);
static VOID TxInt (__reg("a1") struct DevUnit *unit);
static VOID TxError (struct DevUnit *unit, struct MyBase *base);
static VOID ReportEvents (struct DevUnit *unit, ULONG events, struct MyBase *base);
static VOID UnitTask ();
VOID DeleteUnit (struct DevUnit *unit, struct MyBase *base);
struct DevUnit *FindUnit (ULONG unit_num, struct MyBase *base);
struct DevUnit *CreateUnit (ULONG unit_num, struct MyBase *base);
struct TypeStats *FindTypeStats (struct DevUnit *unit, struct MinList *list, ULONG packet_type, struct MyBase *base);
VOID FlushUnit (struct DevUnit *unit, UBYTE last_queue, BYTE error, struct MyBase *base);
void FindCard (struct DevUnit *unit, struct MyBase *base);
void InitialiseCard (struct DevUnit *unit, struct MyBase *base);
VOID ConfigureCard (struct DevUnit *unit, struct MyBase *base);






UBYTE fakeMAC [6] = {0x00, 0x44, 0x66, 0x88, 0xaa, 0xcc};
//UBYTE fakeMAC [6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*****************************************************************************
 *
 * FindCard
 *
 *****************************************************************************/
void FindCard (struct DevUnit *unit, struct MyBase *base) {
UBYTE *p, i;
struct ConfigDev *myCD;

    Debug ("\n     Searching for card\n");

    myCD = NULL;

    // search for all ConfigDevs
    while (myCD = (struct ConfigDev *)FindConfigDev (myCD, 5030, 24)) {     // TODO put some defines
        Debug ("     FindConfigDev success\n");
        break;
    }

    if (myCD) {
        Debug ("     Found card\n");
        
        unit->io_base = (UBYTE *)myCD->cd_BoardAddr;
        
        base->io_base = (APTR)myCD->cd_BoardAddr;
        
        // Set drive current strength
        dm9k_write (base->io_base, BUSCR, 0x60);

        // Get default MAC address
        p = unit->default_address;

        for (i = 0; i < ADDRESS_SIZE; i ++) {
            *p++ = fakeMAC [i];
        }   

    }
    else {
        Debug ("     No card\n");
    }

    Flush (base->log);
}


/*****************************************************************************
 *
 * InitialiseCard
 *
 *****************************************************************************/
void InitialiseCard (struct DevUnit *unit, struct MyBase *base) {
UBYTE *p, i;

    // Set drive current strength
    //dm9k_write (base->io_base, BUSCR, 0x40);

    dm9k_phy_up (unit->io_base);         // (5) PHY ENABLE      - wait 64 us
    Delay (1);

    dm9k_phy_reset (unit->io_base);     // (1) PHY RESET
    Delay (1);

    dm9k_reset (unit->io_base);         // (4) Software RESET  - wait 20 us
    Delay (1);

    dm9k_reset (unit->io_base);         // (4) Software RESET  - wait 20 us
    Delay (1);

    // Set registers
    dm9k_write (unit->io_base, NCR, 0);                                    // normal mode
    dm9k_write (unit->io_base, TCR, 0);                                    // TX polling clear
//    dm9k_set_bits (unit->io_base, RCR, RCR_DIS_CRC);                       // discard RX CRC Error Packet
    dm9k_set_bits (unit->io_base, FCR, FCR_FLCE);                          // Flow Control FLCE bit ON

    dm9k_write (unit->io_base, NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END); // clear Network Status latched bits
    dm9k_write (unit->io_base, ISR, ISR_ROO | ISR_ROS | ISR_PT | ISR_PR);  // clear Interrupt Status latched bits

    // MAC Node Address and FILTER Hash Table
    // dm9000_hash_table(dev); /* map HASH Table (see ch.3-1 & ch.6 ) */
    
    
    for (i = 0; i < 8; i++)
        dm9k_write (unit->io_base, MAB0 + i, 0);
    
    //Always accept broadcast packets
    dm9k_write (unit->io_base, MAB7, 0x80);
    

    // Activate DM9000
//        dm9k_set_bits (base->io_base, RCR, RCR_RXEN | RCR_PRMSC | RCR_ALL);    // RXCR.0 RXEN bit ON Enable
//        dm9k_write (base->io_base, IMR, IMR_PAR | IMR_PTI | IMR_PRI);          // IMR.7 PAR bit ON and TX & RX INT MASK ON

/*
        // Hack: use register 34H to control LEDs via GPIO
        dm9k_write (base->io_base, LEDCR, 0x02);
        dm9k_write (base->io_base, GPCR, 0x06);        // GP2..GP1 for output
        dm9k_write (base->io_base, GPR, 0x06);         // GP2..GP1 set to 1
*/



}





/*****************************************************************************
 *
 * GetUnit
 *
 *****************************************************************************/
struct DevUnit *GetUnit (ULONG unit_num, struct MyBase *base) {
struct DevUnit *unit;

    unit = FindUnit (unit_num, base);

    if (unit == NULL) {
        Debug ("\n Unit not found, creating new");
        unit = CreateUnit (unit_num, base);    
    
        if (unit != NULL) {
            Debug ("\n Adding unit to base->units list");
            AddTail ((APTR)&(base->units), (APTR)unit);
        }
    }
   


   return unit;
}

/*****************************************************************************
 *
 * FindUnit
 *
 *****************************************************************************/
struct DevUnit *FindUnit (ULONG unit_num, struct MyBase *base) {
struct DevUnit *unit,*tail;
BOOL found;

    unit = (APTR)base->units.mlh_Head;
    tail = (APTR)&base->units.mlh_Tail;
    found = FALSE;

    while ((unit != tail) && !found) {
        if (unit->unit_num == unit_num)
            found = TRUE;
        else
            unit = (APTR)unit->node.mln_Succ;
    }

    if (!found)
        unit = NULL;

    return unit;
}


/*****************************************************************************
 *
 * CreateUnit
 *
 *****************************************************************************/
struct DevUnit *CreateUnit (ULONG unit_num, struct MyBase *base) {
BOOL success = TRUE;
struct DevUnit *unit;
struct Task *task;
struct MsgPort *port;
UBYTE i;
APTR stack;
//   struct Interrupt *card_removed_int,*card_inserted_int,*card_status_int;


    Debug ("\n  CreateUnit");
    Flush (base->log);

    unit = (APTR) AllocMem (sizeof (struct DevUnit), MEMF_CLEAR);
    if (unit == NULL)
        success = FALSE;

    if (success) {
        unit->unit_num = unit_num;
        unit->device = base;

        InitSemaphore (&unit->access_lock);

      /* Set up packet filter command */
//      unit->rx_filter_cmd=EL3CMD_SETRXFILTER|EL3CMD_SETRXFILTERF_BCAST
//         |EL3CMD_SETRXFILTERF_UCAST;

        /* Create the message ports for queueing requests */

        for (i = 0; i < REQUEST_QUEUE_COUNT; i ++) {
            unit->request_ports [i] = port = (APTR) AllocMem (sizeof (struct MsgPort),
                                                                MEMF_PUBLIC | MEMF_CLEAR);
            if (port == NULL)
                success = FALSE;

            if (success) {
                NewList (&port->mp_MsgList);
                port->mp_Flags = PA_IGNORE;
                port->mp_SigTask = &unit->tx_int;
            }
        }

//      unit->tuple_buffer=
 //        AllocVec(TUPLE_BUFFER_SIZE,MEMF_ANY);
        unit->rx_buffer = (APTR) AllocVec ((MAX_PACKET_SIZE + 3) &~ 3, MEMF_PUBLIC);
        unit->tx_buffer = (APTR) AllocVec (MAX_PACKET_SIZE, MEMF_PUBLIC);
        if (/*(unit->tuple_buffer==NULL)||*/
            (unit->rx_buffer == NULL) || (unit->tx_buffer == NULL))
                success = FALSE;
    }

    if (success) {
        NewList ((APTR)&unit->openers);
        NewList ((APTR)&unit->type_trackers);
        NewList ((APTR)&unit->multicast_ranges);

        /* Initialise transmit and receive interrupts */

        unit->rx_int.is_Node.ln_Name = (TEXT *)device_name;
        unit->rx_int.is_Node.ln_Pri = 16;
        unit->rx_int.is_Code = RxInt;
        unit->rx_int.is_Data = unit;

        unit->tx_int.is_Node.ln_Name = (TEXT *)device_name;
        unit->tx_int.is_Code = TxInt;
        unit->tx_int.is_Data = unit;

        unit->request_ports [WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    }


    if (success) {
        
        /* Find card and obtain its io_base address */
        
        unit->io_base = NULL;
        
        FindCard (unit, base);
        
        if (unit->io_base == NULL)
            success = FALSE;
    }
    
    if (success) {
        
        /* Create a new task */

        unit->task = task = (struct Task *) AllocMem (sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
        if (task == NULL)
            success = FALSE;
    }

    if (success) {
        stack = (APTR) AllocMem (STACK_SIZE, MEMF_PUBLIC);
        if (stack == NULL)
            success = FALSE;
    }

    if (success) {
       /* Initialise and start task */

        task->tc_Node.ln_Type = NT_TASK;
        task->tc_Node.ln_Pri = TASK_PRIORITY;
        task->tc_Node.ln_Name = (APTR)device_name;
        task->tc_SPUpper = (APTR)((ULONG)stack + STACK_SIZE);
        task->tc_SPLower = stack;
        task->tc_SPReg = (APTR)((ULONG)stack + STACK_SIZE);
        NewList (&task->tc_MemEntry);

        if (AddTask (task, UnitTask, NULL) == NULL)
            success = FALSE;
    }

   /* Send the unit to the new task */

   if (success) {
      task->tc_UserData = unit;
      Debug ("\n  AddTask () success");
   }

   if (!success) {
      DeleteUnit (unit, base);
      unit = NULL;
   }

    Flush (base->log);

   return unit;
}


/*****************************************************************************
 *
 * DeleteUnit
 *
 *****************************************************************************/
VOID DeleteUnit (struct DevUnit *unit, struct MyBase *base) {
UBYTE i;
struct Task *task;

    Debug ("\n  DeleteUnit");

    if (unit != NULL) {
        task = unit->task;
        if (task != NULL) {
            if (task->tc_SPLower != NULL) {
                RemTask (task);
                FreeMem (task->tc_SPLower, STACK_SIZE);
            }
            FreeMem (task, sizeof (struct Task));
        }

        for (i = 0; i < REQUEST_QUEUE_COUNT; i ++) {
            if (unit->request_ports [i] != NULL)
                FreeMem (unit->request_ports [i], sizeof (struct MsgPort));
        }

//      FreeVec(unit->tuple_buffer);


        FreeVec (unit->tx_buffer);
        FreeVec (unit->rx_buffer);

        FreeMem (unit, sizeof (struct DevUnit));
    }

    return;
}




/*****************************************************************************
 *
 * GoOnline
 *
 *****************************************************************************/
VOID GoOnline (struct DevUnit *unit, struct MyBase *base) {
//volatile UBYTE *io_base;


    // LEDs on
//    dm9k_write (base->io_base, GPR, 0x06);

    // Init dm9000
    InitialiseCard (unit, base);        // TODO ?


    /* Choose interrupts */
    unit->flags |= UNITF_ONLINE;


    // Enable RX and TX

    dm9k_write (base->io_base, IMR, 
                                  IMR_PAR 
                                | IMR_PTI           // TX interrupt
                                | IMR_PRI           // RX interrupt
                                );                  // IMR.7 PAR bit ON and TX & RX INT MASK ON        
    
    dm9k_write (base->io_base, RCR, 
                                  RCR_DIS_CRC       // Discard CRC error packet
                                | RCR_DIS_LONG      // Discard long packets (over 1522 bytes)
                             // | RCR_PRMSC
                                | RCR_RXEN          // RX Enable
                                );

    /* Record start time and report Online event */

    // GetSysTime (&unit->stats.LastStart);
    ReportEvents (unit, S2EVENT_ONLINE, base);

   return;
}


/*****************************************************************************
 *
 * GoOffline
 *
 *****************************************************************************/
VOID GoOffline (struct DevUnit *unit, struct MyBase *base) {
//   volatile UBYTE *io_base;

//   io_base = unit->io_base;
  
    unit->flags &= ~UNITF_ONLINE;


   {
        
    // LEDs off
//    dm9k_write (unit->io_base, GPR, 0x00);
    
        
        
      /* Stop interrupts */

    dm9k_write (unit->io_base, IMR, IMR_PAR);   // disable all interrupts


      /* Stop transmission and reception */
    
    dm9k_write (unit->io_base, RCR, 0);    // stop RX


      /* Turn off media functions */

   }

   /* Flush pending read and write requests */

   FlushUnit (unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents (unit, S2EVENT_OFFLINE, base);
   return;
}


/*****************************************************************************
 *
 * AddMulticastRange
 *
 *****************************************************************************/
BOOL AddMulticastRange (struct DevUnit *unit, UBYTE *lower_bound,
                        UBYTE *upper_bound, struct MyBase *base) {
struct AddressRange *range;
ULONG lower_bound_left, upper_bound_left;
UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = BELong (*((ULONG *)lower_bound));
    lower_bound_right = BEWord (*((UWORD *)(lower_bound + 4)));
    upper_bound_left = BELong (*((ULONG *)upper_bound));
    upper_bound_right = BEWord (*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange (unit, lower_bound_left, lower_bound_right,
                                upper_bound_left, upper_bound_right, base);

    if (range != NULL)
        range->add_count ++;
    else {
        range = (APTR)AllocMem (sizeof (struct AddressRange), MEMF_PUBLIC);
        if (range != NULL) {
            range->lower_bound_left = lower_bound_left;
            range->lower_bound_right = lower_bound_right;
            range->upper_bound_left = upper_bound_left;
            range->upper_bound_right = upper_bound_right;
            range->add_count = 1;

            Disable ();
            AddTail ((APTR)&unit->multicast_ranges, (APTR)range);
            Enable ();

            if (unit->range_count ++ == 0) {
//                unit->rx_filter_cmd |= EL3CMD_SETRXFILTERF_MCAST;
//                LEWordOut (unit->io_base + EL3REG_COMMAND, unit->rx_filter_cmd);
            }
        }
    }

    return range != NULL;
}



/*****************************************************************************
 *
 * RemMulticastRange
 *
 *****************************************************************************/
BOOL RemMulticastRange (struct DevUnit *unit, UBYTE *lower_bound, UBYTE *upper_bound, struct MyBase *base) {
struct AddressRange *range;
ULONG lower_bound_left, upper_bound_left;
UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = BELong(*((ULONG *)lower_bound));
    lower_bound_right = BEWord(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = BELong(*((ULONG *)upper_bound));
    upper_bound_right = BEWord(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange (unit, lower_bound_left, lower_bound_right,
            upper_bound_left, upper_bound_right, base);

    if (range != NULL) {
        if (--range->add_count == 0) {
            Disable ();
            Remove ((APTR)range);
            Enable ();
            FreeMem (range, sizeof (struct AddressRange));

            if (--unit->range_count == 0) {
//            unit->rx_filter_cmd&=~EL3CMD_SETRXFILTERF_MCAST;
//            LEWordOut(unit->io_base+EL3REG_COMMAND,unit->rx_filter_cmd);
            }
        }
    }

    return range != NULL;
}



/***** 3c589.device/FindMulticastRange *************************************
*
*   NAME
*	FindMulticastRange -- .
*
*   SYNOPSIS
*	range = FindMulticastRange(unit,lower_bound_left,
*	    lower_bound_right,upper_bound_left,upper_bound_right)
*
*	struct AddressRange *FindMulticastRange(struct DevUnit *,ULONG,
*	    UWORD,ULONG,UWORD);
*
****************************************************************************
*/

static struct AddressRange *FindMulticastRange (struct DevUnit *unit,
        ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,
        UWORD upper_bound_right, struct MyBase *base) {
struct AddressRange *range,*tail;
BOOL found;

   range=(APTR)unit->multicast_ranges.mlh_Head;
   tail=(APTR)&unit->multicast_ranges.mlh_Tail;
   found=FALSE;

   while((range!=tail)&&!found)
   {
      if((lower_bound_left==range->lower_bound_left)&&
         (lower_bound_right==range->lower_bound_right)&&
         (upper_bound_left==range->upper_bound_left)&&
         (upper_bound_right==range->upper_bound_right))
         found=TRUE;
      else
         range=(APTR)range->node.mln_Succ;
   }

   if(!found)
      range=NULL;

   return range;
}





/*****************************************************************************
 *
 * FindTypeStats
 *
 *****************************************************************************/
struct TypeStats *FindTypeStats (struct DevUnit *unit, struct MinList *list,
   ULONG packet_type, struct MyBase *base) {
struct TypeStats *stats, *tail;
BOOL found;

   stats = (APTR)list->mlh_Head;
   tail = (APTR)&list->mlh_Tail;
   found = FALSE;

   while ((stats != tail) && !found) {
      if(stats->packet_type == packet_type)
         found = TRUE;
      else
         stats = (APTR)stats->node.mln_Succ;
   }

   if (!found)
      stats = NULL;

   return stats;
}


/*****************************************************************************
 *
 * FlushUnit
 *
 *****************************************************************************/
VOID FlushUnit (struct DevUnit *unit, UBYTE last_queue, BYTE error, struct MyBase *base) {
struct IORequest *request;
UBYTE i;
struct Opener *opener, *tail;

    /* Abort queued requests */

    for (i = 0; i <= last_queue; i++) {
        while ((request = (APTR)GetMsg (unit->request_ports [i])) != NULL) {
            request->io_Error = IOERR_ABORTED;
            ReplyMsg ((APTR)request);
        }
    }

#if 1
    opener = (APTR)unit->openers.mlh_Head;
    tail = (APTR)&unit->openers.mlh_Tail;

    /* Flush every opener's read queue */

    while (opener != tail) {
        while ((request = (APTR)GetMsg (&opener->read_port)) != NULL) {
            request->io_Error = error;
            ReplyMsg ((APTR)request);
        }
        opener = (APTR)opener->node.mln_Succ;
    }

#else
    opener = request->ios2_BufferManagement;
    while ((request = (APTR)GetMsg (&opener->read_port)) != NULL) {
        request->io_Error = IOERR_ABORTED;
        ReplyMsg ((APTR)request);
    }
#endif

    /* Return */

    return;
}




UBYTE emulated_packet [] = {
    0xfe, 0xed, 0xfa, 0xce, 0xaa, 0x55, 0xde, 0xad, 0xbe, 0xef, 0xff, 0x00, 0x77, 0x77, 0x77, 0x77, 
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0
/*    
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0
*/
};




/*****************************************************************************
 *
 * RxInt
 *
 *****************************************************************************/
static VOID RxInt (__reg("a1") struct DevUnit *unit) {
UWORD rx_status, packet_size;
struct MyBase *base;
BOOL is_orphan, accepted;
ULONG packet_type;
//UWORD status;
UWORD *p, *end;
UBYTE *buffer;
struct IOSana2Req *request, *request_tail;
struct Opener *opener, *opener_tail;
struct TypeStats *tracker;

volatile UBYTE r;
UWORD SRAMaddr;


    base = unit->device;

    buffer = unit->rx_buffer;
    end = (UWORD *)(buffer + HEADER_SIZE);


    do {

        r = dm9k_read (unit->io_base, MRCMDX);      // dummy read
        r = dm9k_read (unit->io_base, MRCMDX);

        if (r == 0x01) {
    
        //    SRAMaddr = (dm9k_read (unit->io_base, MDRAH) << 8) | dm9k_read (unit->io_base, MDRAL);

            is_orphan = TRUE;
     
            // Read status and packet size
                
            rx_status = dm9k_read_w (unit->io_base, MRCMD);
            packet_size = dm9k_read_w (unit->io_base, MRCMD);





            // Read whole packet    TODO read only header, skip the rest if not needed

            p = (UWORD *)(buffer);
            end = (UWORD *)(buffer + packet_size);
                  
            while (p < end)
                *p++ =  ntohw (dm9k_read_w (unit->io_base, MRCMD)); 

//            if (1) {
            if (AddressFilter (unit, buffer + PACKET_DEST, base)) {
                
                packet_type = BEWord (*((UWORD *)(buffer + PACKET_TYPE)));

                opener = (APTR)unit->openers.mlh_Head;
                opener_tail = (APTR)&unit->openers.mlh_Tail;

                /* Offer packet to every opener */

                while (opener != opener_tail) {
                    request = (APTR)opener->read_port.mp_MsgList.lh_Head;
                    request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
                    accepted = FALSE;

                    // Offer packet to each request until it's accepted 

                    while ((request != request_tail) && !accepted) {
                    
                        if (request->ios2_PacketType == packet_type
                            || request->ios2_PacketType <= MTU
                            && packet_type <= MTU) {

                            CopyPacket (unit, request, packet_size, packet_type,
                                !is_orphan, base);
                                
                            accepted = TRUE;
                        }
                        request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;
                    }

                    if (accepted)
                        is_orphan = FALSE;
                        
                    opener = (APTR)opener->node.mln_Succ;
                }

                /* If packet was unwanted, give it to S2_READORPHAN request */

                if (is_orphan) {
                    unit->stats.UnknownTypesReceived ++;
                    if (!IsMsgPortEmpty (unit->request_ports [ADOPT_QUEUE])) {
                        
                        CopyPacket (unit, (APTR)unit->request_ports [ADOPT_QUEUE]->mp_MsgList.lh_Head, 
                                packet_size, packet_type,
                                FALSE, base);

                    }
                }

                /* Update remaining statistics */

                unit->stats.PacketsReceived ++;

                tracker = FindTypeStats (unit, &unit->type_trackers, packet_type, base);
                
                if (tracker != NULL) {
                    tracker->stats.PacketsReceived ++;
                    tracker->stats.BytesReceived += packet_size;
                }                                              
            }
        }
        else {
            unit->stats.BadData ++;
            ReportEvents (unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX, base);            

/*

            // ------- Forge fake packet ------------------------

            emulated_packet [16] = r;                                    // MRCMDX
            emulated_packet [17] = dm9k_read (unit->io_base, RSR);       // RX Status Register
            emulated_packet [18] = unit->isr;                            //  saved Interrupt Status Register
            emulated_packet [19] = dm9k_read (unit->io_base, NSR);       // Network Status Register

            emulated_packet [20] = dm9k_read (unit->io_base, MDRAH);
            emulated_packet [21] = dm9k_read (unit->io_base, MDRAL);


            packet_size = sizeof (emulated_packet);
            CopyMem (emulated_packet, unit->rx_buffer, packet_size);

            unit->stats.UnknownTypesReceived ++;
            if (!IsMsgPortEmpty (unit->request_ports [ADOPT_QUEUE])) {
                CopyPacket (unit, (APTR)unit->request_ports [ADOPT_QUEUE]->mp_MsgList.lh_Head,
                            packet_size, packet_type,
                            TRUE, base);
            }

            // ------- Forge fake packet ------------------------
*/

        }



      /* Discard packet */

/*      
        // Adjust MDRAH/L to skip unread part of packet
        // If the packet was read then it won't hurt

        packet_size -= 14;      // the header was read out already

        if ((SRAMaddr + packet_size) < 0x4000)
            SRAMaddr += packet_size;
        else
            SRAMaddr = 0x0c00 + (0x4000 - SRAMaddr);

        dm9k_write (unit->io_base, MDRAH, SRAMaddr >> 8);
        dm9k_write (unit->io_base, MDRAL, SRAMaddr);
*/
        
        r = dm9k_read (unit->io_base, MRCMDX);      // dummy read
        r = dm9k_read (unit->io_base, MRCMDX);

    

    } while (0);  //    } while (r == 0x01);


    // Enable ints back
    dm9k_write (base->io_base, IMR,
                                IMR_PAR |
                                IMR_PRI |           // RX interrupt
                                IMR_PTI             // TX interrupt
                                );                  


    return;
}


/*****************************************************************************
 *
 * CopyPacket
 *
 *****************************************************************************/
static VOID CopyPacket (struct DevUnit *unit, struct IOSana2Req *request,
   UWORD packet_size, UWORD packet_type, BOOL all_read, struct MyBase *base) {
//volatile UBYTE *io_base;
struct Opener *opener;
UBYTE *buffer;
BOOL filtered = FALSE;
UWORD *p, *end;

   /* Set multicast and broadcast flags */

//   io_base = unit->io_base;
   buffer = unit->rx_buffer;
   request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);    // clear bcast and mcast flags

   if ((*((ULONG *)(buffer + PACKET_DEST)) == 0xffffffff) &&
       (*((UWORD *)(buffer + PACKET_DEST + 4)) == 0xffff))
        request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
   else if ((buffer [PACKET_DEST] & 0x1) != 0)
        request->ios2_Req.io_Flags |= SANA2IOF_MCAST;

   /* Set source and destination addresses and packet type */
   CopyMem (buffer + PACKET_SOURCE, request->ios2_SrcAddr, ADDRESS_SIZE);
   CopyMem (buffer + PACKET_DEST, request->ios2_DstAddr, ADDRESS_SIZE);
   request->ios2_PacketType = packet_type;


   /* Read rest of packet */
/*
    if (!all_read) {
    
        p = (UWORD *)(buffer + ((PACKET_DATA + 1) & ~1));              // p=(ULONG *)(buffer+((PACKET_DATA+3)&~3));
//        p = (UWORD *)(buffer + PACKET_DATA);
        end = (UWORD *)(buffer + packet_size);
      
        while (p < end)
            *p ++ = (dm9k_read_w (unit->io_base, MRCMD));       // htonw?
    }

*/

    if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0) {
        packet_size -= PACKET_DATA;
        buffer += PACKET_DATA;         
    }

#ifdef USE_HACKS
   else
      packet_size += 4;   /* Needed for Shapeshifter & Fusion */        // ??? WTF ???
#endif

   request->ios2_DataLength = packet_size;

   /* Filter packet */

   opener = request->ios2_BufferManagement;
   if ((request->ios2_Req.io_Command == CMD_READ) &&
      (opener->filter_hook != NULL))
      if (!CallHookPkt (opener->filter_hook, request, buffer))
         filtered = TRUE;

   if (!filtered) {
      /* Copy packet into opener's buffer and reply packet */

      if (!opener->rx_function (request->ios2_Data, buffer, packet_size)) {
         request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
         request->ios2_WireError = S2WERR_BUFF_ERROR;
         ReportEvents (unit,
            S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX, base);
      }
      
      Remove ((APTR)request);
      ReplyMsg ((APTR)request);

   }

   return;
}



/*****************************************************************************
 *
 * AddressFilter
 *
 *****************************************************************************/
static BOOL AddressFilter (struct DevUnit *unit, UBYTE *address, struct MyBase *base) {
struct AddressRange *range, *tail;
BOOL accept = TRUE;
ULONG address_left;
UWORD address_right;

   /* Check whether address is unicast/broadcast or multicast */

   address_left = BELong (*((ULONG *)address));
   address_right = BEWord (*((UWORD *)(address + 4)));

   if (((address_left & 0x01000000) != 0) &&
      !((address_left == 0xffffffff) && (address_right == 0xffff))) {
      /* Check if this multicast address is wanted */

      range = (APTR)unit->multicast_ranges.mlh_Head;
      tail = (APTR)&unit->multicast_ranges.mlh_Tail;
      accept = FALSE;

      while ((range != tail) && !accept) {
         if (((address_left > range->lower_bound_left) ||
                (address_left == range->lower_bound_left) &&
                (address_right >= range->lower_bound_right)) &&
                ((address_left < range->upper_bound_left) ||
                (address_left == range->upper_bound_left) &&
                (address_right <= range->upper_bound_right)))
                
            accept = TRUE;
            
         range = (APTR)range->node.mln_Succ;
      }

      if (!accept)
         unit->special_stats [S2SS_ETHERNET_BADMULTICAST & 0xffff] ++;
   }

   return accept;
}



/*****************************************************************************
 *
 * TxInt
 *
 *****************************************************************************/
static VOID TxInt (__reg("a1") struct DevUnit *unit) {
UWORD packet_size, data_size, send_size;
struct MyBase *base;
struct IOSana2Req *request;
BOOL proceed = TRUE;
struct Opener *opener;
UWORD *buffer, *end;
ULONG wire_error;
ULONG *(*dma_tx_function)(APTR __reg("a0"));
BYTE error;
struct MsgPort *port;
struct TypeStats *tracker;

    base = unit->device;
    port = unit->request_ports [WRITE_QUEUE];

    while (proceed && (!IsMsgPortEmpty (port))) {
    
    // run once DEBUG
    //if (!IsMsgPortEmpty (port)) {
    
        error = 0;

        request = (APTR)port->mp_MsgList.lh_Head;
        data_size = packet_size = request->ios2_DataLength;

        if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            packet_size += PACKET_DATA;

             
        if (1) {


            /* Write packet header */
            
            send_size = (packet_size + 1) & (~0x1);

            if ((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0) {
                dm9k_write_block_w (unit->io_base, MWCMD, request->ios2_DstAddr, 3);
                dm9k_write_block_w (unit->io_base, MWCMD, unit->address, 3);
                
                dm9k_write_w (unit->io_base, MWCMD, ntohw (request->ios2_PacketType));
                     
                send_size -= PACKET_DATA;                                
            }

            /* Get packet data */

            opener = (APTR)request->ios2_BufferManagement;
            dma_tx_function = opener->dma_tx_function;
            if (dma_tx_function != NULL)
                buffer = (UWORD *)dma_tx_function (request->ios2_Data);
            else
                buffer = NULL;

            if (buffer == NULL) {
                buffer = (UWORD *)unit->tx_buffer;
                if (!opener->tx_function (buffer, request->ios2_Data, data_size)) {
                    error = S2ERR_NO_RESOURCES;
                    wire_error = S2WERR_BUFF_ERROR;
                    ReportEvents (unit,
                        S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_TX,
                        base);
                }
            }

            /* Write packet data */

            if (error == 0) {
                end = buffer + (send_size >> 1);
                while (buffer < end)
                    dm9k_write_w (unit->io_base, MWCMD, ntohw (*buffer++));

                if ((send_size & 0x1) != 0)
                    dm9k_write_w (unit->io_base, MWCMD, ntohw (*buffer));
                
                // Write transmitted data length to DM9000
                dm9k_write (unit->io_base, TXPLH, packet_size >> 8);
                dm9k_write (unit->io_base, TXPLL, packet_size);
                
                
                // Start TX
                dm9k_write (unit->io_base, TCR, TCR_TXREQ);


            }




            /* Reply packet */

            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            Remove ((APTR)request);
            ReplyMsg ((APTR)request);

            /* Update statistics */

            if (error == 0) {
                unit->stats.PacketsSent ++;

                tracker = FindTypeStats (unit, &unit->type_trackers,
                                        request->ios2_PacketType,
                            base);
                        
                if (tracker != NULL) {
                    tracker->stats.PacketsSent ++;
                    tracker->stats.BytesSent += packet_size;
                }
            }
        }
        else
            proceed = FALSE;
    }

    if (proceed)
        unit->request_ports [WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else {
   //   LEWordOut(io_base+EL3REG_COMMAND,EL3CMD_SETTXTHRESH
//         |(PREAMBLE_SIZE+packet_size));
        unit->request_ports [WRITE_QUEUE]->mp_Flags = PA_IGNORE;
    }


    // Enable ints back
    dm9k_write (base->io_base, IMR,
                                IMR_PAR |
                                IMR_PRI |           // RX interrupt
                                IMR_PTI             // TX interrupt
                                );



    return;
}


/*****************************************************************************
 *
 * TxError
 *
 *****************************************************************************/
static VOID TxError (struct DevUnit *unit, struct MyBase *base) {
volatile UBYTE *io_base;
UBYTE tx_status, flags = 0;

   io_base = unit->io_base;

   /* Gather all errors */


   /* Restart transmitter if necessary */

/*
   if((flags&EL3REG_TXSTATUSF_JABBER)!=0)
   {
      Disable();
      LEWordOut(io_base+EL3REG_COMMAND,EL3CMD_TXRESET);
      while((LEWordIn(io_base+EL3REG_STATUS)&
         EL3REG_STATUSF_CMDINPROGRESS)!=0);
      Enable();
   }

   if((flags&(EL3REG_TXSTATUSF_JABBER|EL3REG_TXSTATUSF_OVERFLOW))!=0)
      LEWordOut(io_base+EL3REG_COMMAND,EL3CMD_TXENABLE);
*/
   /* Report the error(s) */

   ReportEvents (unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_TX, base);

   return;
}


/*****************************************************************************
 *
 * ReportEvents
 *
 *****************************************************************************/
static VOID ReportEvents (struct DevUnit *unit, ULONG events, struct MyBase *base) {
struct IOSana2Req *request, *tail, *next_request;
struct List *list;

   list = &unit->request_ports [EVENT_QUEUE]->mp_MsgList;
   next_request = (APTR)list->lh_Head;
   tail = (APTR)&list->lh_Tail;

   Disable ();
   while (next_request != tail) {
      request = next_request;
      next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

        if ((request->ios2_WireError & events) != 0) {
            request->ios2_WireError = events;
            Remove ((APTR)request);
            ReplyMsg ((APTR)request);
        }
    }
    Enable ();

   return;
}


/*****************************************************************************
 *
 * ConfigureCard
 *
 *****************************************************************************/
VOID ConfigureCard (struct DevUnit *unit, struct MyBase *base) {
volatile UBYTE *io_base;
UBYTE i;

   /* Set MAC address */

    for (i = 0; i < ADDRESS_SIZE; i += 2)
//      ByteOut (io_base+EL3REG_ADDRESS0+i,unit->address[i]);
   //     ppPoke (PP_IA + i, unit->address [i] + (unit->address [i + 1] << 8));

    ;

   /* Decide on promiscuous mode */

    if ((unit->flags & UNITF_PROM) != 0)
    
    
    ;
    
     //   ppPoke (PP_RxCTL, PP_RxCTL_Promiscuous | PP_RxCTL_RxOK /*| PP_RxCTL_RUNT */);
    else
     //   ppPoke (PP_RxCTL, PP_RxCTL_IA | PP_RxCTL_Broadcast | PP_RxCTL_RxOK);

    ;

   /* Go online */

   GoOnline (unit, base);

   /* Return */

   return;
}








/************************************************************
 *
 * InterruptServer
 *
 ************************************************************/
__saveds void InterruptServer (__reg("a1") struct Task *task) {
struct DevUnit *unit;
UBYTE r;
UBYTE index;

    unit = task->tc_UserData;

    // Save dm9000 INDEX register
    index = peek (unit->io_base);

    r = dm9k_read (unit->io_base, ISR);

    if (r & 0x3f) {

        // Acknowledge all interrupts
        dm9k_write (unit->io_base, ISR, 0x3f);      // you MUST do this

        // Disable all interrupts
        dm9k_write (unit->io_base, IMR, IMR_PAR);

        // Save ISR for later inspection
        unit->isr = r;

//        // Acknowledge all interrupts
//        dm9k_write (unit->io_base, ISR, 0x3f);      // you MUST do this


        if (r & ISR_LNKCHG) {   // Link changed
        }
    

        if (r & ISR_PT) {       // Packet transmitted
            Cause (&unit->tx_int);
        }


        if (r & ISR_PR) {       // Packet received
            Cause (&unit->rx_int);
        }

    }
    
    // Restore dm9000 INDEX register
    poke (unit->io_base, index);

    
}




















/*****************************************************************************
 *
 * UnitTask
 *
 *****************************************************************************/
static VOID UnitTask () {
struct Task *task;
struct IORequest *request;
struct DevUnit *unit;
struct MyBase *base;
struct DOSBase *DOSBase;
struct MsgPort *general_port;
struct MsgPort *int_port;
struct Interrupt *myInt;

ULONG signals, 
    wait_signals, 
    general_port_signal;
    
UBYTE rxbyte;

    /* Get parameters */

    task = AbsExecBase->ThisTask;
    unit = task->tc_UserData;
    base = unit->device;


    /* Activate general request port */

    general_port = unit->request_ports [GENERAL_QUEUE];
    general_port->mp_SigTask = task;
    general_port->mp_SigBit = AllocSignal (-1);
    general_port_signal = 1 << general_port->mp_SigBit;
    general_port->mp_Flags = PA_SIGNAL;


    wait_signals = general_port_signal;


    // Install interrupt handler        NB perhaps we should do this when creating Unit

    myInt = (struct Interrupt *)AllocVec (sizeof (struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    if (myInt) {
        myInt->is_Node.ln_Name = "dm9000";      // TODO
        myInt->is_Node.ln_Pri = 16;
        myInt->is_Code = InterruptServer;
        myInt->is_Data = task;

        AddIntServer (INTB_PORTS, myInt);
    }




   /* Tell ourselves to check port for old messages */

   Signal (task, general_port_signal);



   /* Infinite loop to service requests and signals */

   while (TRUE) {
      signals = Wait (wait_signals);            

        if ((signals & general_port_signal) != 0) {
            while ((request = (APTR)GetMsg (general_port)) != NULL) {
                
                /* Service the request as soon as the unit is free */

                ObtainSemaphore (&unit->access_lock);
                ServiceRequest ((APTR)request, base);
            }
        }


   }
}






