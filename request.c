#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/initializers.h>
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <libraries/dos.h>
#include <devices/timer.h>

//#include <devices/newstyle.h>

#include "devices/sana2.h"
#include "devices/sana2specialstats.h"

#include "device.h"



#define KNOWN_EVENTS (S2EVENT_ERROR | S2EVENT_TX | S2EVENT_RX | S2EVENT_ONLINE | \
                        S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE)


static BOOL CmdInvalid (struct IOSana2Req *request, struct MyBase *base);
static BOOL CmdRead (struct IOSana2Req *request, struct MyBase *base);
static BOOL CmdWrite (struct IOSana2Req *request, struct MyBase *base);
static BOOL CmdFlush (struct IORequest *request,struct MyBase *base);
static BOOL CmdS2DeviceQuery (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdGetStationAddress (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdConfigInterface (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdBroadcast (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdTrackType (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdUntrackType (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdGetTypeStats (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdGetSpecialStats (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdGetGlobalStats (struct IOSana2Req *request,   struct MyBase *base);
static BOOL CmdOnEvent (struct IOSana2Req *request,struct MyBase *base);
static BOOL CmdReadOrphan (struct IOSana2Req *request, struct MyBase *base);
static BOOL CmdOnline (struct IOSana2Req *request,struct MyBase *base);
static BOOL CmdOffline (struct IOSana2Req *request,struct MyBase *base);
static BOOL CmdDeviceQuery (struct IOStdReq *request, struct MyBase *base);
static BOOL CmdAddMulticastAddresses (struct IOSana2Req *request, struct MyBase *base);
static BOOL CmdDelMulticastAddresses (struct IOSana2Req *request, struct MyBase *base);

VOID PutRequest (struct MsgPort *port, struct IORequest *request, struct MyBase *base);



static const UWORD supported_commands [] = {
   CMD_READ,
   CMD_WRITE,
   CMD_FLUSH,
   S2_DEVICEQUERY,
   S2_GETSTATIONADDRESS,
   S2_CONFIGINTERFACE,
   S2_ADDMULTICASTADDRESS,
   S2_DELMULTICASTADDRESS,
   S2_MULTICAST,
   S2_BROADCAST,
   S2_TRACKTYPE,
   S2_UNTRACKTYPE,
   S2_GETTYPESTATS,
   S2_GETSPECIALSTATS,
   S2_GETGLOBALSTATS,
   S2_ONEVENT,
   S2_READORPHAN,
   S2_ONLINE,
   S2_OFFLINE,
   NSCMD_DEVICEQUERY,
//   S2_ADDMULTICASTADDRESSES,
//   S2_DELMULTICASTADDRESSES,
   0
};


struct Sana2DeviceQuery sana2_info = {
   0,
   0,
   0,
   0,
   ADDRESS_SIZE * 8,
   MTU,
   10000000,
   S2WireType_Ethernet
};



/*****************************************************************************
 *
 * DebugS2Request
 *
 *****************************************************************************/
void DebugS2Request (struct IOSana2Req *request) {
UBYTE *data;

//    KPrintF ("\n  --- IOSana2Req ---");
//    KPrintF ("\n  ios2_Command: %8lx", request->ios2_Req.io_Command);
//    KPrintF ("\n  ios2_Flags: %8lx", request->ios2_Req.io_Flags);
//    KPrintF ("\n  ios2_PacketType: %8lx", request->ios2_PacketType);
//    KPrintF ("\n  ios2_WireError:  %8lx", request->ios2_WireError);

/*
    data = request->ios2_SrcAddr;
    KPrintF ("\n  ios2_SrcAddr: %2x %2x %2x %2x %2x %2x %2x %2x", 
        *data ++, *data ++, *data ++, *data ++, *data ++, *data ++, *data ++, *data ++);

    data = request->ios2_DstAddr;
    KPrintF ("\n  ios2_DstAddr: %2x %2x %2x %2x %2x %2x %2x %2x",
        *data ++, *data ++, *data ++, *data ++, *data ++, *data ++, *data ++, *data ++);
*/
//    KPrintF ("\n  ios2_DataLength: %8lx\n", request->ios2_DataLength);
/*    
    KPrintF ("\n  ios2_Data: ");
    data = request->ios2_Data;
    DebugHex (data [0]); DebugHex (data [1]); DebugHex (data [2]); DebugHex (data [3]); DebugHex (data [4]); DebugHex (data [5]); DebugHex (data [6]); DebugHex (data [7]);
*/
  //  KPrintF (" ...\n");
}


/*****************************************************************************
 *
 * ServiceRequest
 *
 *****************************************************************************/
VOID ServiceRequest (struct IOSana2Req *request, struct MyBase *base) {
BOOL complete;


    KPrintF ("\n= ServiceRequest: ");

    switch (request->ios2_Req.io_Command) {
    case CMD_READ:
      complete = CmdRead (request, base);
      break;
   case CMD_WRITE:
      complete = CmdWrite (request, base);
      break;
   case CMD_FLUSH:
      complete = CmdFlush ((APTR)request, base);
      break;
   case S2_DEVICEQUERY:
      complete = CmdS2DeviceQuery ((APTR)request, base);
      break;
   case S2_GETSTATIONADDRESS:
      complete = CmdGetStationAddress ((APTR)request, base);
      break;
   case S2_CONFIGINTERFACE:
      complete = CmdConfigInterface ((APTR)request, base);
      break;     
   case S2_ADDMULTICASTADDRESS:
      complete = FALSE; // CmdAddMulticastAddresses ((APTR)request, base);
      break;
   case S2_DELMULTICASTADDRESS:
      complete = FALSE; // CmdDelMulticastAddresses ((APTR)request, base);
      break;
   case S2_MULTICAST:
      complete = CmdWrite ((APTR)request, base);
      break;
   case S2_BROADCAST:
      complete = CmdBroadcast ((APTR)request, base);
      break;
   case S2_TRACKTYPE:
      complete = CmdTrackType ((APTR)request, base);

      break;
   case S2_UNTRACKTYPE:
      complete = CmdUntrackType ((APTR)request, base);

      break;
   case S2_GETTYPESTATS:
      complete = CmdGetTypeStats ((APTR)request, base);

      break;
   case S2_GETSPECIALSTATS:
      complete = CmdGetSpecialStats ((APTR)request, base);

      break;
   case S2_GETGLOBALSTATS:
      complete = CmdGetGlobalStats ((APTR)request, base);

      break;
   case S2_ONEVENT:
      complete = CmdOnEvent ((APTR)request, base);

      break;
      
   case S2_READORPHAN:
      complete = CmdReadOrphan ((APTR)request, base);

      break;
      
   case S2_ONLINE:
      complete = CmdOnline ((APTR)request, base);

      break;

   case S2_OFFLINE:
      complete = CmdOffline ((APTR)request, base);

      break;

   case NSCMD_DEVICEQUERY:
      complete = CmdDeviceQuery ((APTR)request, base);

      break;
     
   default:
      complete = CmdInvalid ((APTR)request, base);

   }

    if (complete && ((request->ios2_Req.io_Flags & IOF_QUICK) == 0)) {
        KPrintF ("\n= Complete, calling ReplyMsg");
        ReplyMsg ((APTR)request);
    }

   ReleaseSemaphore (&((struct DevUnit *)request->ios2_Req.io_Unit)->access_lock);
   
   
   return;
}

/*****************************************************************************
 *
 * CmdInvalid
 *
 *****************************************************************************/
static BOOL CmdInvalid (struct IOSana2Req *request, struct MyBase *base) {
    
    KPrintF ("CmdInvalid");
    
   request->ios2_Req.io_Error = IOERR_NOCMD;
   request->ios2_WireError = S2WERR_GENERIC_ERROR;

   return TRUE;
}

/*****************************************************************************
 *
 * CmdRead
 *
 *****************************************************************************/
static BOOL CmdRead (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
struct Opener *opener;
BOOL complete = FALSE;


    KPrintF ("CmdRead");

    unit = (APTR)request->ios2_Req.io_Unit;

DebugS2Request (request);

    if ((unit->flags & UNITF_ONLINE) != 0) {
        opener = request->ios2_BufferManagement;
        PutRequest (&opener->read_port, (APTR)request, base);
    }
    else {
        KPrintF (" S2WERR_UNIT_OFFLINE!");
        request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        request->ios2_WireError = S2WERR_UNIT_OFFLINE;
        complete = TRUE;
    }

   /* Return */

   return complete;
}

/*****************************************************************************
 *
 * CmdWrite
 *
 *****************************************************************************/
static BOOL CmdWrite (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
BYTE error = 0;
ULONG wire_error;
BOOL complete = FALSE;

    KPrintF ("CmdWrite");

   /* Check request is valid */

   unit = (APTR)request->ios2_Req.io_Unit;
   
DebugS2Request (request);   
   
   
   if ((unit->flags & UNITF_ONLINE) == 0) {
      error = S2ERR_OUTOFSERVICE;
      wire_error = S2WERR_UNIT_OFFLINE;
   }
   else if ((request->ios2_Req.io_Command == S2_MULTICAST) &&
      ((request->ios2_DstAddr [0] & 0x1) == 0)) {
            error = S2ERR_BAD_ADDRESS;
            wire_error = S2WERR_BAD_MULTICAST;
   }

   /* Queue request for sending */

   if (error == 0) {
      PutRequest (unit->request_ports [WRITE_QUEUE], (APTR)request, base);
      
//      Cause (&unit->tx_int);      // HACK
      
      
      
    }
   else {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      complete = TRUE;
   }






   /* Return */

   return complete;
}


/*****************************************************************************
 *
 * CmdFlush
 *
 *****************************************************************************/
static BOOL CmdFlush (struct IORequest *request,struct MyBase *base) {
    
    KPrintF ("CmdFlush");
    
   FlushUnit ((APTR)request->io_Unit, EVENT_QUEUE, IOERR_ABORTED, base);
   return TRUE;
}


/*****************************************************************************
 *
 * CmdS2DeviceQuery
 *
 *****************************************************************************/
static BOOL CmdS2DeviceQuery (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
struct Sana2DeviceQuery *info;
ULONG size_available, size;

    KPrintF ("CmdS2DeviceQuery");

   /* Copy device info */

   unit = (APTR)request->ios2_Req.io_Unit;
   info = request->ios2_StatData;
   size = size_available = info->SizeAvailable;
   if (size > sizeof (struct Sana2DeviceQuery))
      size = sizeof (struct Sana2DeviceQuery);



   CopyMem (&sana2_info, info, size);
  
   info->SizeAvailable = size_available;
   info->SizeSupplied = size;

   /* Return */

   return TRUE;
}


/*****************************************************************************
 *
 * CmdGetStationAddress
 *
 *****************************************************************************/
static BOOL CmdGetStationAddress (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;

    KPrintF ("CmdGetStationAddress");
    
    
   unit = (APTR)request->ios2_Req.io_Unit;
   CopyMem (unit->address, request->ios2_SrcAddr, ADDRESS_SIZE);
   CopyMem (unit->default_address, request->ios2_DstAddr, ADDRESS_SIZE);

   return TRUE;
}


/*****************************************************************************
 *
 * CmdConfigInterface
 *
 *****************************************************************************/
static BOOL CmdConfigInterface (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;


    KPrintF ("CmdConfigInterface");
    
   /* Configure adapter */

   unit = (APTR)request->ios2_Req.io_Unit;
   if ((unit->flags & UNITF_CONFIGURED) == 0) {
      CopyMem (request->ios2_SrcAddr, unit->address, ADDRESS_SIZE);
  
  //       ConfigureAdapter(unit,base);
      unit->flags |= UNITF_CONFIGURED;
   }
   else {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_IS_CONFIGURED;
   }

   /* Return */

   return TRUE;
}



/*****************************************************************************
 *
 * CmdBroadcast
 *
 *****************************************************************************/
static BOOL CmdBroadcast (struct IOSana2Req *request, struct MyBase *base) {

//    KPrintF ("CmdBroadcast");

   /* Fill in the broadcast address as destination */

   *((ULONG *)request->ios2_DstAddr) = 0xffffffff;
   *((UWORD *)(request->ios2_DstAddr + 4)) = 0xffff;

   /* Queue the write as normal */

   return CmdWrite (request, base);
}

/*****************************************************************************
 *
 * CmdTrackType
 *
 *****************************************************************************/
static BOOL CmdTrackType (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
struct Opener *opener;
ULONG packet_type, wire_error;
struct TypeTracker *tracker;
struct TypeStats *initial_stats;
BYTE error = 0;

    KPrintF ("CmdTrackType");
    
   unit = (APTR)request->ios2_Req.io_Unit;
   packet_type = request->ios2_PacketType;

   /* Get global tracker */

   tracker = (struct TypeTracker *)
      FindTypeStats (unit, &unit->type_trackers, packet_type, base);

   if (tracker != NULL)
      tracker->user_count ++;
   else {
      tracker = (APTR)AllocMem (sizeof (struct TypeTracker), MEMF_PUBLIC | MEMF_CLEAR);
      if (tracker != NULL) {
         tracker->packet_type = packet_type;
         tracker->user_count = 1;

         Disable ();
         AddTail ((APTR)&unit->type_trackers, (APTR)tracker);
         Enable ();
      }
   }

   /* Store initial figures for this opener */

   opener = request->ios2_BufferManagement;
   initial_stats = FindTypeStats (unit, &opener->initial_stats, packet_type,
      base);

   if (initial_stats != NULL) {
      error = S2ERR_BAD_STATE;
      wire_error = S2WERR_ALREADY_TRACKED;
   }

   if (error == 0) {
      initial_stats = (APTR)AllocMem (sizeof (struct TypeStats), MEMF_PUBLIC);
      if (initial_stats == NULL) {
         error = S2ERR_NO_RESOURCES;
         wire_error = S2WERR_GENERIC_ERROR;
      }
   }

   if (error == 0) {
      CopyMem (tracker, initial_stats, sizeof (struct TypeStats));
      AddTail ((APTR)&opener->initial_stats, (APTR)initial_stats);
   }

   /* Return */

   request->ios2_Req.io_Error=error;
   request->ios2_WireError=wire_error;
   return TRUE;
}




/*****************************************************************************
 *
 * CmdUntrackType
 *
 *****************************************************************************/
static BOOL CmdUntrackType (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
struct Opener *opener;
ULONG packet_type;
struct TypeTracker *tracker;
struct TypeStats *initial_stats;

    KPrintF ("CmdUntrackType");

   unit = (APTR)request->ios2_Req.io_Unit;
   packet_type = request->ios2_PacketType;

   /* Get global tracker and initial figures */

   tracker = (struct TypeTracker *)
      FindTypeStats (unit, &unit->type_trackers, packet_type, base);
   opener = request->ios2_BufferManagement;
   initial_stats = FindTypeStats (unit, &opener->initial_stats, packet_type, base);

   /* Decrement tracker usage and free unused structures */

   if (initial_stats != NULL) {
      if ((--tracker->user_count) == 0) {
         Disable ();
         Remove ((APTR)tracker);
         Enable ();
         FreeMem (tracker, sizeof (struct TypeTracker));
      }

      Remove ((APTR)initial_stats);
      FreeMem (initial_stats, sizeof(struct TypeStats));
   }
   else {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_NOT_TRACKED;
   }

   /* Return */

   return TRUE;
}





static BOOL CmdGetTypeStats (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
struct Opener *opener;
ULONG packet_type;
struct TypeStats *initial_stats, *tracker;
struct Sana2PacketTypeStats *stats;

    KPrintF ("CmdGetTypeStats");

   unit = (APTR)request->ios2_Req.io_Unit;
   packet_type = request->ios2_PacketType;

   /* Get global tracker and initial figures */

   tracker = FindTypeStats (unit, &unit->type_trackers, packet_type, base);
   opener = request->ios2_BufferManagement;
   initial_stats = FindTypeStats (unit, &opener->initial_stats, packet_type, base);

   /* Copy and adjust figures */

   if (initial_stats != NULL) {
      stats = request->ios2_StatData;
      CopyMem (&tracker->stats, stats, sizeof (struct Sana2PacketTypeStats));
      stats->PacketsSent -= initial_stats->stats.PacketsSent;
      stats->PacketsReceived -= initial_stats->stats.PacketsReceived;
      stats->BytesSent -= initial_stats->stats.BytesSent;
      stats->BytesReceived -= initial_stats->stats.BytesReceived;
      stats->PacketsDropped -= initial_stats->stats.PacketsDropped;
   }
   else {
      request->ios2_Req.io_Error = S2ERR_BAD_STATE;
      request->ios2_WireError = S2WERR_NOT_TRACKED;
   }

   /* Return */

   return TRUE;
}



/****** 3c589.device/S2_GETSPECIALSTATS ************************************
*
*   NAME
*	S2_GETSPECIALSTATS --
*
*   FUNCTION
*
*   INPUTS
*	ios2_StatData - Pointer to Sana2SpecialStatHeader structure.
*
*   RESULTS
*	io_Error
*	ios2_WireError
*
****************************************************************************
*
*/

static BOOL CmdGetSpecialStats (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
UWORD i, stat_count;
struct Sana2SpecialStatHeader *header;
struct Sana2SpecialStatRecord *record;

    KPrintF ("CmdGetSpecialStats");

   /* Fill in stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   header = request->ios2_StatData;
   record = (APTR)(header + 1);

   stat_count = header->RecordCountMax;
   if (stat_count > STAT_COUNT)
      stat_count = STAT_COUNT;

   for (i = 0; i < stat_count; i ++) {
      record->Type = (S2WireType_Ethernet << 16) + i;
      record->Count = unit->special_stats [i];
      record->String = (TEXT *)&unit->openers.mlh_Tail;
      record++;
   }

   header->RecordCountSupplied = stat_count;

   /* Return */

   return TRUE;
}



/****** 3c589.device/S2_GETGLOBALSTATS *************************************
*
****************************************************************************
*
*/

static BOOL CmdGetGlobalStats (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;

    KPrintF ("CmdGetGlobalStats");

   /* Copy stats */

   unit = (APTR)request->ios2_Req.io_Unit;
   CopyMem (&unit->stats, request->ios2_StatData, sizeof (struct Sana2DeviceStats));

   /* Return */

   return TRUE;
}



/****** 3c589.device/S2_ONEVENT ********************************************
*
*
****************************************************************************
*
*/

static BOOL CmdOnEvent (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
ULONG events, wanted_events;
BOOL complete = FALSE;

    KPrintF ("CmdOnEvent");

   /* Check if we understand the event types */

   unit = (APTR)request->ios2_Req.io_Unit;
   
   
DebugS2Request (request);   
   
   wanted_events = request->ios2_WireError;
   if ((wanted_events & ~KNOWN_EVENTS) != 0) {
      request->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
      events = S2WERR_BAD_EVENT;
   }
   else {
      if ((unit->flags & UNITF_ONLINE) != 0)
         events = S2EVENT_ONLINE;
      else
         events = S2EVENT_OFFLINE;

      events &= wanted_events;
   }

   /* Reply request if a wanted event has already occurred */

   if (events != 0) {
      request->ios2_WireError = events;
      complete = TRUE;
   }
   else
      PutRequest (unit->request_ports [EVENT_QUEUE], (APTR)request, base);

   /* Return */

   return complete;
}


/*****************************************************************************
 *
 * CmdReadOrphan
 *
 *****************************************************************************/
static BOOL CmdReadOrphan (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
BYTE error = 0;
ULONG wire_error;
BOOL complete = FALSE;

    KPrintF ("CmdReadOrphan");

   /* Check request is valid */

   unit = (APTR)request->ios2_Req.io_Unit;
   
DebugS2Request (request);   
   
   if ((unit->flags & UNITF_ONLINE) == 0) {
      error = S2ERR_OUTOFSERVICE;
      wire_error = S2WERR_UNIT_OFFLINE;
   }

   /* Queue request */

   if (error == 0)
      PutRequest (unit->request_ports [ADOPT_QUEUE], (APTR)request, base);
   else {
      request->ios2_Req.io_Error = error;
      request->ios2_WireError = wire_error;
      complete = TRUE;
   }

   /* Return */

   return complete;
}



/*****************************************************************************
 *
 * CmdOnline
 *
 *****************************************************************************/
static BOOL CmdOnline (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
BYTE error = 0;
ULONG wire_error;
UWORD i;

    KPrintF ("CmdOnline");

   /* Check request is valid */

    unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->flags & UNITF_CONFIGURED) == 0) {
        error = S2ERR_BAD_STATE;
        wire_error = S2WERR_NOT_CONFIGURED;
    }

    /* Clear global and special stats and put adapter back online */

    if ((error == 0) && ((unit->flags & UNITF_ONLINE) == 0)) {
        unit->stats.PacketsReceived = 0;
        unit->stats.PacketsSent = 0;
        unit->stats.BadData = 0;
        unit->stats.Overruns = 0;
        unit->stats.UnknownTypesReceived = 0;
        unit->stats.Reconfigurations = 0;

        for (i = 0; i < STAT_COUNT; i ++)
            unit->special_stats [i] = 0;

        GoOnline (unit, base);
    }

    /* Return */

    request->ios2_Req.io_Error = error;
    request->ios2_WireError = wire_error;
    return TRUE;
}


/*****************************************************************************
 *
 * CmdOffline
 *
 *****************************************************************************/
static BOOL CmdOffline (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;

    KPrintF ("CmdOffline");
    
    /* Put adapter offline */

    unit = (APTR)request->ios2_Req.io_Unit;
    
    if ((unit->flags & UNITF_ONLINE) != 0)
        GoOffline (unit, base);

    /* Return */

    return TRUE;
}



/****** 3c589.device/NSCMD_DEVICEQUERY *************************************
*
*   NAME
*	NSCMD_DEVICEQUERY -- Query device capabilities.
*
*   FUNCTION
*
*   INPUTS
*	io_Length - ???.
*	io_Data - pointer to NSDeviceQueryResult structure.
*
*   RESULTS
*	io_Error
*	io_Actual - size of structure device can handle.
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
* Note that we have to pretend the request structure is an IOStdReq.
*
*/

static BOOL CmdDeviceQuery (struct IOStdReq *request, struct MyBase *base) {
    
   KPrintF ("CmdDeviceQuery");
    
/*
   struct NSDeviceQueryResult *info;

  

   info = request->io_Data;
   request->io_Actual = info->SizeAvailable = 
      (ULONG)OFFSET(NSDeviceQueryResult,SupportedCommands) + sizeof(APTR);

  

   info->DeviceType = NSDEVTYPE_SANA2;
   info->DeviceSubType = 0;

   info->SupportedCommands = (APTR)supported_commands;

  
*/
//   return TRUE;

    return FALSE;
}


/*****************************************************************************
 *
 * CmdAddMulticastAddresses
 *
 *****************************************************************************/
static BOOL CmdAddMulticastAddresses (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
UBYTE *lower_bound, *upper_bound;

    KPrintF ("CmdAddMulticastAddresses");

    unit = (APTR) request->ios2_Req.io_Unit;

    lower_bound = request->ios2_SrcAddr;
    if (request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESS)
        upper_bound = lower_bound;
    else
        upper_bound = request->ios2_DstAddr;

    if (!AddMulticastRange (unit, lower_bound, upper_bound, base)) {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_GENERIC_ERROR;
    }

    /* Return */

    return TRUE;
}


/*****************************************************************************
 *
 * CmdDelMulticastAddresses
 *
 *****************************************************************************/
static BOOL CmdDelMulticastAddresses (struct IOSana2Req *request, struct MyBase *base) {
struct DevUnit *unit;
UBYTE *lower_bound, *upper_bound;

    KPrintF ("CmdDelMulticastAddresses");

    unit = (APTR) request->ios2_Req.io_Unit;

    lower_bound = request->ios2_SrcAddr;
    if (request->ios2_Req.io_Command == S2_DELMULTICASTADDRESS)
        upper_bound = lower_bound;
    else
        upper_bound = request->ios2_DstAddr;

    if (!RemMulticastRange (unit, lower_bound, upper_bound,base)) {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_BAD_MULTICAST;
    }

    /* Return */

    return TRUE;
}


/*****************************************************************************
 *
 * PutRequest
 *
 *****************************************************************************/
VOID PutRequest (struct MsgPort *port, struct IORequest *request, struct MyBase *base) {
    
    KPrintF ("\n= PutRequest =\n");
    
    request->io_Flags &= ~IOF_QUICK;
    PutMsg (port, (APTR)request);

   return;
}



