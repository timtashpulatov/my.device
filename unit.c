#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <libraries/dos.h>

#include "devices/sana2.h"
#include "devices/sana2specialstats.h"


#include "device.h"

#define TASK_PRIORITY 0
#define STACK_SIZE 4096
#define MAX_TUPLE_SIZE 0xff
#define TUPLE_BUFFER_SIZE (MAX_TUPLE_SIZE+8)


IMPORT struct ExecBase *AbsExecBase;

//static struct AddressRange *FindMulticastRange(struct DevUnit *unit,  ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left,   UWORD upper_bound_right, struct DevBase *base);
static VOID RxInt (__reg("a1") struct DevUnit *unit);
static VOID CopyPacket(struct DevUnit *unit,struct IOSana2Req *request, UWORD packet_size,UWORD packet_type,BOOL all_read,struct MyBase *base);
static BOOL AddressFilter (struct DevUnit *unit, UBYTE *address, struct MyBase *base);
static VOID TxInt(__reg("a1") struct DevUnit *unit);
static VOID TxError(struct DevUnit *unit, struct MyBase *base);
static VOID ReportEvents (struct DevUnit *unit, ULONG events, struct MyBase *base);
static VOID UnitTask();
VOID DeleteUnit (struct DevUnit *unit, struct MyBase *base);
struct DevUnit *FindUnit (ULONG unit_num, struct MyBase *base);
struct DevUnit *CreateUnit (ULONG unit_num, struct MyBase *base);
struct TypeStats *FindTypeStats (struct DevUnit *unit, struct MinList *list, ULONG packet_type, struct MyBase *base);
VOID FlushUnit (struct DevUnit *unit, UBYTE last_queue, BYTE error, struct MyBase *base);



struct DevUnit *GetUnit (ULONG unit_num, struct MyBase *base) {
   struct DevUnit *unit;

   unit = FindUnit (unit_num, base);

   if (unit == NULL) {
      unit = CreateUnit (unit_num, base);
      if (unit != NULL)
         AddTail ((APTR)&(base->units), (APTR)unit);
   }

   return unit;
}

struct DevUnit *FindUnit (ULONG unit_num, struct MyBase *base)
{
   struct DevUnit *unit,*tail;
   BOOL found;

   unit = (APTR)base->units.mlh_Head;
   tail = (APTR)&base->units.mlh_Tail;
   found = FALSE;

   while ((unit != tail) && !found)
   {
      if (unit->unit_num == unit_num)
         found = TRUE;
      else
         unit = (APTR)unit->node.mln_Succ;
   }

   if(!found)
      unit = NULL;

   return unit;
}


struct DevUnit *CreateUnit (ULONG unit_num, struct MyBase *base)
{
   BOOL success = TRUE;
   struct DevUnit *unit;
   struct Task *task;
   struct MsgPort *port;
   UBYTE i;
   APTR stack;
//   struct Interrupt *card_removed_int,*card_inserted_int,*card_status_int;

   unit = (APTR) AllocMem (sizeof (struct DevUnit), MEMF_CLEAR);
   if (unit == NULL)
      success = FALSE;

   if(success)
   {
      unit->unit_num = unit_num;
      unit->device = base;

      InitSemaphore (&unit->access_lock);

      /* Set up packet filter command */
//      unit->rx_filter_cmd=EL3CMD_SETRXFILTER|EL3CMD_SETRXFILTERF_BCAST
//         |EL3CMD_SETRXFILTERF_UCAST;

      /* Create the message ports for queuing requests */

      for (i = 0; i < REQUEST_QUEUE_COUNT; i ++)
      {
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
      if(/*(unit->tuple_buffer==NULL)||*/
         (unit->rx_buffer==NULL)||(unit->tx_buffer==NULL))
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

   if(success) {
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

   if(success)
   {
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

   if (success)
      task->tc_UserData = unit;

   if (!success) {
      DeleteUnit (unit, base);
      unit = NULL;
   }

   return unit;
}


VOID DeleteUnit (struct DevUnit *unit, struct MyBase *base) {
UBYTE i;
struct Task *task;

   if (unit != NULL) {
      task = unit->task;
      if (task != NULL) {
         if (task->tc_SPLower != NULL) {
            RemTask (task);
            FreeMem (task->tc_SPLower, STACK_SIZE);
         }
         FreeMem(task,sizeof(struct Task));
      }

      for(i=0;i<REQUEST_QUEUE_COUNT;i++)
      {
         if(unit->request_ports[i]!=NULL)
            FreeMem(unit->request_ports[i],sizeof(struct MsgPort));
      }

//      FreeVec(unit->tuple_buffer);


      FreeVec (unit->tx_buffer);
      FreeVec (unit->rx_buffer);

      FreeMem (unit,sizeof(struct DevUnit));
   }

   return;
}





VOID GoOnline(struct DevUnit *unit, struct MyBase *base)
{
   volatile UBYTE *io_base;
   UWORD transceiver;

   /* Choose interrupts */

   unit->flags |= UNITF_ONLINE;

   /* Record start time and report Online event */

  // GetSysTime (&unit->stats.LastStart);
   ReportEvents (unit, S2EVENT_ONLINE, base);

   return;
}



VOID GoOffline (struct DevUnit *unit, struct MyBase *base)
{
   volatile UBYTE *io_base;

   io_base = unit->io_base;
   unit->flags &= ~UNITF_ONLINE;

//   if((unit->flags&UNITF_HAVEADAPTER)!=0)
    if (1)
   {
      /* Stop interrupts */


      /* Stop transmission and reception */


      /* Turn off media functions */

   }

   /* Flush pending read and write requests */

   FlushUnit (unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, base);

   /* Report Offline event and return */

   ReportEvents (unit, S2EVENT_OFFLINE, base);
   return;
}




struct TypeStats *FindTypeStats (struct DevUnit *unit, struct MinList *list,
   ULONG packet_type, struct MyBase *base)
{
   struct TypeStats *stats, *tail;
   BOOL found;

   stats = (APTR)list->mlh_Head;
   tail = (APTR)&list->mlh_Tail;
   found = FALSE;

   while ((stats != tail) && !found)
   {
      if(stats->packet_type == packet_type)
         found = TRUE;
      else
         stats = (APTR)stats->node.mln_Succ;
   }

   if (!found)
      stats = NULL;

   return stats;
}


VOID FlushUnit (struct DevUnit *unit, UBYTE last_queue, BYTE error, struct MyBase *base)
{
   struct IORequest *request;
   UBYTE i;
   struct Opener *opener, *tail;

   /* Abort queued requests */

   for (i = 0; i <= last_queue; i++)
   {
      while((request=(APTR)GetMsg(unit->request_ports[i]))!=NULL)
      {
         request->io_Error = IOERR_ABORTED;
         ReplyMsg ((APTR)request);
      }
   }

#if 1
   opener=(APTR)unit->openers.mlh_Head;
   tail=(APTR)&unit->openers.mlh_Tail;

   /* Flush every opener's read queue */

   while(opener!=tail)
   {
      while((request=(APTR)GetMsg(&opener->read_port))!=NULL)
      {
         request->io_Error=error;
         ReplyMsg((APTR)request);
      }
      opener=(APTR)opener->node.mln_Succ;
   }

#else
   opener=request->ios2_BufferManagement;
   while((request=(APTR)GetMsg(&opener->read_port))!=NULL)
   {
      request->io_Error=IOERR_ABORTED;
      ReplyMsg((APTR)request);
   }
#endif

   /* Return */

   return;
}










static VOID RxInt (__reg("a1") struct DevUnit *unit)
{
   volatile UBYTE *io_base;
   UWORD rx_status,packet_size;
   struct MyBase *base;
   BOOL is_orphan,accepted;
   ULONG packet_type,*p,*end;
   UBYTE *buffer;
   struct IOSana2Req *request,*request_tail;
   struct Opener *opener,*opener_tail;
   struct TypeStats *tracker;

   base=unit->device;
   io_base=unit->io_base;
   buffer=unit->rx_buffer;
   end=(ULONG *)(buffer+HEADER_SIZE);

   while(
//   ((rx_status=LEWordIn(io_base+EL3REG_RXSTATUS))
 //     &EL3REG_RXSTATUSF_INCOMPLETE)==0
      1
      )
   {
      if((rx_status /* & EL3REG_RXSTATUSF_ERROR */)==0)
      {
         /* Read packet header */

         is_orphan = TRUE;
         packet_size = rx_status /*& EL3REG_RXSTATUS_SIZEMASK */;
         p = (ULONG *)buffer;
         while(p<end)
            *p++ = 0; //LongIn(io_base+EL3REG_DATA0);

         if(AddressFilter(unit,buffer+PACKET_DEST,base))
         {
            packet_type=BEWord(*((UWORD *)(buffer+PACKET_TYPE)));

            opener=(APTR)unit->openers.mlh_Head;
            opener_tail=(APTR)&unit->openers.mlh_Tail;

            /* Offer packet to every opener */

            while(opener!=opener_tail)
            {
               request=(APTR)opener->read_port.mp_MsgList.lh_Head;
               request_tail=(APTR)&opener->read_port.mp_MsgList.lh_Tail;
               accepted=FALSE;

               /* Offer packet to each request until it's accepted */

               while((request!=request_tail)&&!accepted)
               {
                  if(request->ios2_PacketType==packet_type
                     ||request->ios2_PacketType<=MTU
                     &&packet_type<=MTU)
                  {
                     CopyPacket(unit,request,packet_size,packet_type,
                        !is_orphan,base);
                     accepted=TRUE;
                  }
                  request=
                     (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;
               }

               if(accepted)
                  is_orphan=FALSE;
               opener=(APTR)opener->node.mln_Succ;
            }

            /* If packet was unwanted, give it to S2_READORPHAN request */

            if(is_orphan)
            {
               unit->stats.UnknownTypesReceived++;
               if(!IsMsgPortEmpty(unit->request_ports[ADOPT_QUEUE]))
               {
                  CopyPacket(unit,
                     (APTR)unit->request_ports[ADOPT_QUEUE]->
                     mp_MsgList.lh_Head,packet_size,packet_type,
                     FALSE,base);
               }
            }

            /* Update remaining statistics */

            unit->stats.PacketsReceived++;

            tracker=
               FindTypeStats (unit, &unit->type_trackers, packet_type, base);
            if(tracker!=NULL)
            {
               tracker->stats.PacketsReceived++;
               tracker->stats.BytesReceived+=packet_size;
            }
         }
      }
      else
      {
         unit->stats.BadData++;
         ReportEvents (unit, S2EVENT_ERROR | S2EVENT_HARDWARE | S2EVENT_RX, base);
      }

      /* Discard packet */

      Disable();   /* Needed? */
  //    LEWordOut(io_base+EL3REG_COMMAND,EL3CMD_RXDISCARD);
   //   while((LEWordIn(io_base+EL3REG_STATUS)&
    //     EL3REG_STATUSF_CMDINPROGRESS)!=0);
      Enable();
   }

   /* Return */

//   LEWordOut(io_base+EL3REG_COMMAND,EL3CMD_SETINTMASK|INT_MASK);
   return;
}



/****i* 3c589.device/CopyPacket ********************************************
*
*   NAME
*	CopyPacket -- .
*
*   SYNOPSIS
*	CopyPacket(unit,request,packet_size,packet_type,
*	    all_read)
*
*	VOID CopyPacket(struct DevUnit *,struct IOSana2Req *,UWORD,UWORD,
*	    BOOL);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

static VOID CopyPacket(struct DevUnit *unit,struct IOSana2Req *request,
   UWORD packet_size,UWORD packet_type,BOOL all_read, struct MyBase *base)
{
   volatile UBYTE *io_base;
   struct Opener *opener;
   UBYTE *buffer;
   BOOL filtered=FALSE;
   ULONG *p,*end;

   /* Set multicast and broadcast flags */

   io_base=unit->io_base;
   buffer=unit->rx_buffer;
   request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
   if((*((ULONG *)(buffer+PACKET_DEST))==0xffffffff)&&
      (*((UWORD *)(buffer+PACKET_DEST+4))==0xffff))
      request->ios2_Req.io_Flags|=SANA2IOF_BCAST;
   else if((buffer[PACKET_DEST]&0x1)!=0)
      request->ios2_Req.io_Flags|=SANA2IOF_MCAST;

   /* Set source and destination addresses and packet type */

   CopyMem(buffer+PACKET_SOURCE,request->ios2_SrcAddr,ADDRESS_SIZE);
   CopyMem(buffer+PACKET_DEST,request->ios2_DstAddr,ADDRESS_SIZE);
   request->ios2_PacketType=packet_type;

   /* Read rest of packet */

   if(!all_read)
   {
      p=(ULONG *)(buffer+((PACKET_DATA + 3)&~3));
      end=(ULONG *)(buffer+packet_size);
      while(p<end)
         *p++ = 0;  //LongIn(io_base+EL3REG_DATA0);
   }

   if((request->ios2_Req.io_Flags & SANA2IOF_RAW)==0)
   {
      packet_size-=PACKET_DATA;
      buffer+=PACKET_DATA;
   }
#ifdef USE_HACKS
   else
      packet_size+=4;   /* Needed for Shapeshifter & Fusion */
#endif

   request->ios2_DataLength=packet_size;

   /* Filter packet */

   opener=request->ios2_BufferManagement;
   if((request->ios2_Req.io_Command==CMD_READ)&&
      (opener->filter_hook!=NULL))
      if(!CallHookPkt(opener->filter_hook,request,buffer))
         filtered=TRUE;

   if(!filtered)
   {
      /* Copy packet into opener's buffer and reply packet */

      if(!opener->rx_function(request->ios2_Data,buffer,packet_size))
      {
         request->ios2_Req.io_Error=S2ERR_NO_RESOURCES;
         request->ios2_WireError=S2WERR_BUFF_ERROR;
         ReportEvents (unit,            
            S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX, base);
      }
      Remove ((APTR)request);
      ReplyMsg ((APTR)request);

   }

   return;
}




static BOOL AddressFilter (struct DevUnit *unit, UBYTE *address, struct MyBase *base)
{
   struct AddressRange *range,*tail;
   BOOL accept = TRUE;
   ULONG address_left;
   UWORD address_right;

   /* Check whether address is unicast/broadcast or multicast */

   address_left=BELong(*((ULONG *)address));
   address_right=BEWord(*((UWORD *)(address+4)));

   if(((address_left&0x01000000)!=0)&&
      !((address_left==0xffffffff)&&(address_right==0xffff)))
   {
      /* Check if this multicast address is wanted */

      range = (APTR)unit->multicast_ranges.mlh_Head;
      tail = (APTR)&unit->multicast_ranges.mlh_Tail;
      accept = FALSE;

      while((range!=tail)&&!accept)
      {
         if(((address_left > range->lower_bound_left)||
            (address_left == range->lower_bound_left)&&
            (address_right >= range->lower_bound_right))&&
            ((address_left < range->upper_bound_left)||
            (address_left == range->upper_bound_left)&&
            (address_right <= range->upper_bound_right)))
            accept=TRUE;
         range=(APTR)range->node.mln_Succ;
      }

      if(!accept)
         unit->special_stats[S2SS_ETHERNET_BADMULTICAST&0xffff]++;
   }

   return accept;
}




static VOID TxInt (__reg("a1") struct DevUnit *unit)
{
   volatile UBYTE *io_base;
   UWORD packet_size,data_size,send_size;
   struct MyBase *base;
   struct IOSana2Req *request;
   BOOL proceed=TRUE;
   struct Opener *opener;
   ULONG *buffer,*end,wire_error;
   ULONG *(*dma_tx_function)(APTR __reg("a0"));
   BYTE error;
   struct MsgPort *port;
   struct TypeStats *tracker;

   base = unit->device;
   io_base = unit->io_base;

   port=unit->request_ports [WRITE_QUEUE];

   while(proceed&&(!IsMsgPortEmpty(port)))
   {
      error=0;

      request=(APTR)port->mp_MsgList.lh_Head;
      data_size=packet_size=request->ios2_DataLength;

      if((request->ios2_Req.io_Flags&SANA2IOF_RAW)==0)
         packet_size+=PACKET_DATA;

//      if(LEWordIn(io_base+EL3REG_TXSPACE)>PREAMBLE_SIZE+packet_size)
        if (1)
      {
         /* Write packet preamble */

//         LELongOut(io_base+EL3REG_DATA0,packet_size);

         /* Write packet header */

         send_size=(packet_size+3)&(~0x3);
         if((request->ios2_Req.io_Flags & SANA2IOF_RAW)==0)
         {
   //         LongOut(io_base+EL3REG_DATA0,*((ULONG *)request->ios2_DstAddr));
   //         WordOut(io_base+EL3REG_DATA0,
   //            *((UWORD *)(request->ios2_DstAddr+4)));
   //         WordOut(io_base+EL3REG_DATA0,*((UWORD *)unit->address));
    //        LongOut(io_base+EL3REG_DATA0,*((ULONG *)(unit->address+2)));
     //       BEWordOut(io_base+EL3REG_DATA0,request->ios2_PacketType);
            send_size -= PACKET_DATA;
         }

         /* Get packet data */

         opener=(APTR)request->ios2_BufferManagement;
         dma_tx_function=opener->dma_tx_function;
         if(dma_tx_function!=NULL)
            buffer=dma_tx_function(request->ios2_Data);
         else
            buffer=NULL;

         if(buffer==NULL)
         {
            buffer=(ULONG *)unit->tx_buffer;
            if(!opener->tx_function(buffer,request->ios2_Data,data_size))
            {
               error = S2ERR_NO_RESOURCES;
               wire_error = S2WERR_BUFF_ERROR;
               ReportEvents (unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_TX,
                  base);
            }
         }

         /* Write packet data */

         if(error==0)
         {
            end=buffer+(send_size>>2);
            while(buffer<end)
               // LongOut(io_base+EL3REG_DATA0,*buffer++);
               ;

            if((send_size&0x3)!=0)
               //WordOut(io_base+EL3REG_DATA0,*((UWORD *)buffer));
               ;
         }

         /* Reply packet */

         request->ios2_Req.io_Error=error;
         request->ios2_WireError=wire_error;
         Remove((APTR)request);
         ReplyMsg((APTR)request);

         /* Update statistics */

         if(error==0)
         {
            unit->stats.PacketsSent ++;

            tracker = FindTypeStats (unit, &unit->type_trackers,
                        request->ios2_PacketType,
                        base);
                        
            if(tracker!=NULL)
            {
               tracker->stats.PacketsSent++;
               tracker->stats.BytesSent+=packet_size;
            }
         }
      }
      else
         proceed=FALSE;
   }

   if(proceed)
      unit->request_ports[WRITE_QUEUE]->mp_Flags=PA_SOFTINT;
   else
   {
   //   LEWordOut(io_base+EL3REG_COMMAND,EL3CMD_SETTXTHRESH
//         |(PREAMBLE_SIZE+packet_size));
      unit->request_ports [WRITE_QUEUE]->mp_Flags = PA_IGNORE;
   }

   return;
}



static VOID TxError(struct DevUnit *unit, struct MyBase *base)
{
   volatile UBYTE *io_base;
   UBYTE tx_status,flags=0;

   io_base=unit->io_base;

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


static VOID ReportEvents (struct DevUnit *unit, ULONG events, struct MyBase *base)
{
   struct IOSana2Req *request,*tail,*next_request;
   struct List *list;

   list=&unit->request_ports[EVENT_QUEUE]->mp_MsgList;
   next_request=(APTR)list->lh_Head;
   tail=(APTR)&list->lh_Tail;

   Disable();
   while(next_request!=tail)
   {
      request=next_request;
      next_request=(APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

      if((request->ios2_WireError&events)!=0)
      {
         request->ios2_WireError=events;
         Remove((APTR)request);
         ReplyMsg((APTR)request);
      }
   }
   Enable();

   return;
}



static VOID UnitTask()
{
   struct Task *task;
   struct IORequest *request;
   struct DevUnit *unit;
   struct MyBase *base;
   struct MsgPort *general_port;
   ULONG signals,wait_signals,card_removed_signal,card_inserted_signal,
      general_port_signal;

   /* Get parameters */

   task = AbsExecBase->ThisTask;
   unit = task->tc_UserData;
   base = unit->device;

   /* Activate general request port */

   general_port=unit->request_ports[GENERAL_QUEUE];
   general_port->mp_SigTask=task;
   general_port->mp_SigBit=AllocSignal(-1);
   general_port_signal=1<<general_port->mp_SigBit;
   general_port->mp_Flags=PA_SIGNAL;

   wait_signals = 1 << general_port->mp_SigBit;

   /* Tell ourselves to check port for old messages */

   Signal (task, general_port_signal);

   /* Infinite loop to service requests and signals */

   while(TRUE)
   {
      signals=Wait(wait_signals);


      if((signals&general_port_signal)!=0)
      {
         while((request=(APTR)GetMsg(general_port))!=NULL)
         {
            /* Service the request as soon as the unit is free */

            ObtainSemaphore (&unit->access_lock);
            ServiceRequest ((APTR)request, base);
         }
      }
   }
}






