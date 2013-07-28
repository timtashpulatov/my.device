#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/libraries.h>
#include <exec/initializers.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <libraries/dos.h>
#include <devices/timer.h>

#include "devices/sana2.h"


#define VERSION  37
#define REVISION 01

#define MYLIBNAME "my"
#define MYLIBVER  " 37.01 (25.07.2013)"

char MyLibName [] = "my.device";	//"my.library";		// MYLIBNAME ".library";
char MyLibID   [] = "37.01 (25.07.2013)";	//MYLIBNAME MYLIBVER;

char VERSTRING [] = "\0$VER: my 37.01 (25.07.2013)"; 	// EXLIBNAME EXLIBVER;


struct ExecBase *SysBase    = NULL;
struct DOSBase	*DOSBase 	= NULL;
struct UtilityBase	*UtilityBase 	= NULL;


LONG LibStart (void) {
	return -1;
}

#define ADDRESS_SIZE    6
#define MTU             1500
#define STAT_COUNT      3

/* Unit flags */

#define UNITF_SHARED        (1<<0)
#define UNITF_ONLINE        (1<<1)
//#define UNITF_HAVECARD      (1<<2)
//#define UNITF_HAVEADAPTER   (1<<3)
#define UNITF_CONFIGURED    (1<<4)
#define UNITF_PROM          (1<<5)
//#define UNITF_ROADRUNNER    (1<<6)

UBYTE address [ADDRESS_SIZE];  // ?
UBYTE default_address [ADDRESS_SIZE] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
UWORD flags;

struct Sana2DeviceQuery sana2_info =
{
   0,
   0,
   0,
   0,
   ADDRESS_SIZE * 8,
   MTU,
   10000000,
   S2WireType_Ethernet
};

struct Sana2DeviceStats stats;
ULONG special_stats [STAT_COUNT];



typedef struct {
 	struct Library            my_LibNode;
 	APTR                      my_SegList;
 	struct ExecBase           *my_SysBase;
	struct DOSBase            *my_DOSBase;
// struct IntuitionBase  *exb_IntuitionBase;
// struct GfxBase        *exb_GfxBase;
    struct UtilityBase          *my_UtilityBase;
	BPTR                      log;
} MyBase_t;


__saveds struct MyBase * InitLib (void); // (struct ExecBase *, APTR, MyBase_t *);


__saveds struct MyBase * OpenLib (APTR);
__saveds BYTE DevOpen ( __reg ("a6") MyBase_t *my,
                        __reg ("a1") struct IOSana2Req *iorq,
                        __reg ("d1") ULONG flags,
                        __reg ("d0") ULONG unit_num
	);

__saveds APTR CloseLib (APTR);
__saveds APTR ExpungeLib (APTR);
ULONG ASM ExtFuncLib (void);

__saveds void BeginIO (	__reg ("a6") MyBase_t *my, 
                        __reg ("a1") struct IOSana2Req *iorq);

void AbortIO (struct IORequest *);

static BOOL CmdConfigInterface (struct IOSana2Req *iorq, struct DevBase *my);
static BOOL CmdS2DeviceQuery (struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdGetStationAddress (struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdGetSpecialStats (struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdGetGlobalStats (struct IOSana2Req *request, struct DevBase *base);
static BOOL CmdBroadcast (struct IOSana2Req *iorq, struct DevBase *base);
static BOOL CmdWrite (struct IOSana2Req *iorq, MyBase_t *my);
static BOOL CmdOnline (struct IOSana2Req *iorq, struct DevBase *my);
static BOOL CmdOffline (struct IOSana2Req *iorq, struct DevBase *my);
void GoOnline (struct DevBase *my);
void GoOffline (struct DevBase *my);

/* ----------------------------------------------------------------------------------------
   ! ROMTag and Library inilitalization structure:
   !
   ! Below you find the ROMTag, which is the most important "magic" part of a library
   ! (as for any other resident module). You should not need to modify any of the
   ! structures directly, since all the data is referenced from constants from somewhere else.
   !
   ! You may place the ROMTag directly after the LibStart (-> StartUp.c) function as well.
   !
   ! Note, that the data initialization structure may be somewhat redundant - it's
   ! for demonstration purposes.
   !
   ! EndResident can be placed somewhere else - but it must follow the ROMTag and
   ! it must not be placed in a different SECTION.
   ---------------------------------------------------------------------------------------- */

extern ULONG InitTab [];
extern APTR EndResident; /* below */

__aligned struct Resident ROMTag =     /* do not change */
{
	RTC_MATCHWORD,
	&ROMTag,
 	&EndResident,
 	RTF_AUTOINIT,
 	VERSION,
 	NT_DEVICE,	// NT_LIBRARY,
 	0,
 	&MyLibName [0],
 	&MyLibID [0],
 	&InitTab [0]
};

APTR EndResident;



struct MyDataInit {                     /* do not change */
 UWORD ln_Type_Init;      UWORD ln_Type_Offset;      UWORD ln_Type_Content;
 UBYTE ln_Name_Init;      UBYTE ln_Name_Offset;      ULONG ln_Name_Content;
 UWORD lib_Flags_Init;    UWORD lib_Flags_Offset;    UWORD lib_Flags_Content;
 UWORD lib_Version_Init;  UWORD lib_Version_Offset;  UWORD lib_Version_Content;
 UWORD lib_Revision_Init; UWORD lib_Revision_Offset; UWORD lib_Revision_Content;
 UBYTE lib_IdString_Init; UBYTE lib_IdString_Offset; ULONG lib_IdString_Content;
 ULONG ENDMARK;
} DataTab = {


	0xe000, 8, NT_DEVICE << 0x08,	// NT_LIBRARY << 0x08,
        0x80,	10, (ULONG) &MyLibName [0],
        0xe000,	14, (LIBF_SUMUSED|LIBF_CHANGED) << 8,
        0xd000,	20, VERSION,
        0xd000,	22, REVISION,
        0x80,	24, (ULONG) &MyLibID [0],
        (ULONG) 0


//{
// INITBYTE(OFFSET(Node,         ln_Type),      NT_LIBRARY),
// 0x80, (UBYTE) OFFSET(Node,    ln_Name),      (ULONG) &MyLibName[0],
// INITBYTE(OFFSET(Library,      lib_Flags),    LIBF_SUMUSED|LIBF_CHANGED),
// INITWORD(OFFSET(Library,      lib_Version),  VERSION),
// INITWORD(OFFSET(Library,      lib_Revision), REVISION),
// 0x80, (UBYTE) OFFSET(Library, lib_IdString), (ULONG) &MyLibID[0],
// (ULONG) 0
};



MyBase_t *MyBase;




/* ----------------------------------------------------------------------------------------
   ! Function and Data Tables:
   !
   ! The function and data tables have been placed here for traditional reasons.
   ! Placing the RomTag structure before (-> LibInit.c) would also be a good idea,
   ! but it depends on whether you would like to keep the "version" stuff separately.
   ---------------------------------------------------------------------------------------- */

extern APTR FuncTab [];
//extern struct MyDataInit * DataTab;

struct InitTable {                       /* do not change */
	ULONG              LibBaseSize;
	APTR              *FunctionTable;
 	struct MyDataInit *DataTable;
 	APTR               InitLibTable;
} InitTab = {
 	(ULONG)               sizeof(MyBase_t),
 	(APTR              *) &FuncTab[0],
 	(struct MyDataInit *) &DataTab,
 	(APTR)                InitLib
};

APTR FuncTab [] = {
	(APTR) DevOpen,	// 	(APTR) OpenLib,
 	(APTR) CloseLib,
 	(APTR) ExpungeLib,
	(APTR) ExtFuncLib,

	(APTR) BeginIO,
	(APTR) AbortIO,

 	//EXF_TestRequest,  /* add your own functions here */

 	(APTR) ((LONG)-1)
};



__saveds __stdargs ULONG L_OpenLibs (MyBase_t *MyBase);
__saveds __stdargs void L_CloseLibs (void);


/* ----------------------------------------------------------------------------------------
   ! InitLib:
   !
   ! This one is single-threaded by the Ramlib process. Theoretically you can do, what
   ! you like here, since you have full exclusive control over all the library code and data.
   ! But due to some bugs in Ramlib V37-40, you can easily cause a deadlock when opening
   ! certain libraries here (which open other libraries, that open other libraries, that...)
   !
   ---------------------------------------------------------------------------------------- */

__saveds struct MyBase * InitLib (__reg ("a6") struct ExecBase  *sysbase,
                                  __reg ("a0") APTR 		seglist,
                                  __reg ("d0") MyBase_t 	*my) 
{


 MyBase = my;

 MyBase->my_SysBase = sysbase;
 MyBase->my_SegList = seglist;

 MyBase->log = NULL;

 if (L_OpenLibs (MyBase)) 
	return (MyBase);

 L_CloseLibs ();

  {
   ULONG negsize, possize, fullsize;
   UBYTE *negptr = (UBYTE *) MyBase;

   negsize  = MyBase->my_LibNode.lib_NegSize;
   possize  = MyBase->my_LibNode.lib_PosSize;
   fullsize = negsize + possize;
   negptr  -= negsize;

   FreeMem (negptr, fullsize);

  }

 return (NULL);
}




/*
static BYTE DevOpen(ULONG unit_num REG("d0"),
   struct IOSana2Req *request REG("a1"),ULONG flags REG("d1"),
   struct DevBase *base REG(BASE_REG))
*/

void Debug (BPTR log, char *s) {
	FPuts (log, s);
}

void DebugHex (BPTR log, UBYTE b) {
register UBYTE nibble;
	nibble = b >> 4;
	FPutC (log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);
	nibble = b & 0x0f;
	FPutC (log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);
	FPutC (log, 0x20);
//	Flush (log);

}

void DebugHex16 (BPTR log, UWORD w) {
register UBYTE nibble;
	nibble = w >> 12;
	FPutC (log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);

	nibble = w >> 8;
	FPutC (log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);

	nibble = w >> 4;
	FPutC (log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);

	nibble = w & 0x0f;
	FPutC (log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);

	FPutC (log, 0x20);

	Flush (log);
}


void DebugHex32 (BPTR log, ULONG l) {

	DebugHex16 (l >> 16);
	DebugHex16 (l);

	FPutC (log, 0x20);
	Flush (log);
}





BOOL (*tx_function)(APTR __reg("a0"), APTR __reg("a1"), ULONG __reg("d0"));

__saveds BYTE DevOpen (	__reg ("a6") MyBase_t *my,
			__reg ("a1") struct IOSana2Req *request,
			__reg ("d1") ULONG flags,
			__reg ("d0") ULONG unit_num
	) 
{
    struct TagItem *tag_list;
    
	if (my->log == NULL)
		my->log = Open ("T:log", MODE_READWRITE);

	if (my->log) {
	//	FPuts (my->log, "\n- DevOpen");
		//VFPrintf (my->log, "\n- DevOpen (unit %d, flags %x)",
	          //  unit_num, flags
		Debug (my->log, "\n- DevOpen unit ");
		DebugHex (my->log, unit_num);
		Debug (my->log, ", flags ");
		DebugHex32 (my->log, flags);
        };
	


	my->my_LibNode.lib_OpenCnt ++;
	my->my_LibNode.lib_Flags &= ~LIBF_DELEXP;
	
	request->ios2_Req.io_Unit = 1;
	tag_list = request->ios2_BufferManagement;
	request->ios2_BufferManagement = NULL;
	
	// tags
	
    	tx_function = (APTR)GetTagData (S2_CopyFromBuff, NULL, tag_list);
	if (tx_function == NULL)
		VFPrintf (my->log, "\n tx_function EMPTY");

	
	
	return (0);
}


/* ----------------------------------------------------------------------------------------
   ! OpenLib:
   !
   ! This one is enclosed within a Forbid/Permit pair by Exec V37-40. Since a Wait() call
   ! would break this Forbid/Permit(), you are not allowed to start any operations that
   ! may cause a Wait() during their processing. It's possible, that future OS versions
   ! won't turn the multi-tasking off, but instead use semaphore protection for this
   ! function.
   !
   ! Currently you only can bypass this restriction by supplying your own semaphore
   ! mechanism.
   ---------------------------------------------------------------------------------------- */

__saveds struct MyBase * OpenLib (__reg ("a6") MyBase_t *my) {

	if (my->log == NULL)
		my->log = Open ("T:log", MODE_READWRITE);

	if (my->log) {
        Seek (my->log, 0, OFFSET_END);
       	Debug (my->log, "\n- OpenLib");		
    }
	
	my->my_LibNode.lib_OpenCnt ++;

	my->my_LibNode.lib_Flags &= ~LIBF_DELEXP;
	
	return (my);
}




/* ----------------------------------------------------------------------------------------
   ! CloseLib:
   !
   ! This one is enclosed within a Forbid/Permit pair by Exec V37-40. Since a Wait() call
   ! would break this Forbid/Permit(), you are not allowed to start any operations that
   ! may cause a Wait() during their processing. It's possible, that future OS versions
   ! won't turn the multi-tasking off, but instead use semaphore protection for this
   ! function.
   !
   ! Currently you only can bypass this restriction by supplying your own semaphore
   ! mechanism.
   ---------------------------------------------------------------------------------------- */

__saveds APTR CloseLib (__reg ("a6") MyBase_t *my)
{
	my->my_LibNode.lib_OpenCnt --;

	if (my->log) {
		Debug (my->log, "\n- CloseLib");
		Close (my->log);
		my->log = NULL;
	}

	if (!my->my_LibNode.lib_OpenCnt) {
		if (my->my_LibNode.lib_Flags & LIBF_DELEXP) {
	     		return (ExpungeLib (my));
    		}
  	}

 return (NULL);
}

/* ----------------------------------------------------------------------------------------
   ! ExpungeLib:
   !
   ! This one is enclosed within a Forbid/Permit pair by Exec V37-40. Since a Wait() call
   ! would break this Forbid/Permit(), you are not allowed to start any operations that
   ! may cause a Wait() during their processing. It's possible, that future OS versions
   ! won't turn the multi-tasking off, but instead use semaphore protection for this
   ! function.
   !
   ! Currently you only could bypass this restriction by supplying your own semaphore
   ! mechanism - but since expunging can't be done twice, one should avoid it here.
   ---------------------------------------------------------------------------------------- */

__saveds APTR ExpungeLib (__reg ("a6") MyBase_t *my)
{
 MyBase_t *MyBase = my;
 APTR seglist;

 if (!MyBase->my_LibNode.lib_OpenCnt)
  {
   ULONG negsize, possize, fullsize;
   UBYTE *negptr = (UBYTE *) MyBase;

   seglist = MyBase->my_SegList;

   Remove((struct Node *)MyBase);

   L_CloseLibs();

   negsize  = MyBase->my_LibNode.lib_NegSize;
   possize  = MyBase->my_LibNode.lib_PosSize;
   fullsize = negsize + possize;
   negptr  -= negsize;

   FreeMem (negptr, fullsize);


   return (seglist);
  }

 MyBase->my_LibNode.lib_Flags |= LIBF_DELEXP;

 return(NULL);
}


/* ----------------------------------------------------------------------------------------
   ! ExtFunct:
   !
   ! This one is enclosed within a Forbid/Permit pair by Exec V37-40. Since a Wait() call
   ! would break this Forbid/Permit(), you are not allowed to start any operations that
   ! may cause a Wait() during their processing. It's possible, that future OS versions
   ! won't turn the multi-tasking off, but instead use semaphore protection for this
   ! function.
   !
   ! Currently you only can bypass this restriction by supplying your own semaphore
   ! mechanism - but since this function currently is unused, you should not touch
   ! it, either.
   ---------------------------------------------------------------------------------------- */

ULONG ASM ExtFuncLib (void) {
	return (NULL);
}


/* ----------------------------------------------------------------------------------------
   ! L_OpenLibs:
   !
   ! Since this one is called by InitLib, libraries not shareable between Processes or
   ! libraries messing with RamLib (deadlock and crash) may not be opened here.
   !
   ! You may bypass this by calling this function fromout LibOpen, but then you will
   ! have to a) protect it by a semaphore and b) make sure, that libraries are only
   ! opened once (when using global library bases).
   ---------------------------------------------------------------------------------------- */

__saveds __stdargs ULONG L_OpenLibs (MyBase_t *MyBase)
{
	SysBase = (*((struct ExecBase **) 4));
	DOSBase = (struct DOSBase *) OpenLibrary ("dos.library", 37);
	if (!DOSBase) return (FALSE);
	
    UtilityBase = (APTR)OpenLibrary ("utility.library", 37);
    if (!UtilityBase) return (FALSE);
	
    MyBase->my_UtilityBase = UtilityBase;
	MyBase->my_SysBase = SysBase;
	MyBase->my_DOSBase = DOSBase;

 return (TRUE);
}

/* ----------------------------------------------------------------------------------------
   ! L_CloseLibs:
   !
   ! This one by default is called by ExpungeLib, which only can take place once
   ! and thus per definition is single-threaded.
   !
   ! When calling this fromout LibClose instead, you will have to protect it by a
   ! semaphore, since you don't know whether a given CloseLibrary(foobase) may cause a Wait().
   ! Additionally, there should be protection, that a library won't be closed twice.
   ---------------------------------------------------------------------------------------- */

__saveds __stdargs void L_CloseLibs (void)
{
// if(GfxBase)       CloseLibrary((struct Library *) GfxBase);
// if(IntuitionBase) CloseLibrary((struct Library *) IntuitionBase);
	if (DOSBase) CloseLibrary ((struct Library *) DOSBase);
	if (UtilityBase) CloseLibrary ((struct Library *) UtilityBase);
}






__saveds void BeginIO ( __reg ("a6") MyBase_t *my, 
                        __reg ("a1") struct IOSana2Req *iorq) 
{
    BOOL complete;
    
//	if (my->log) {
        
    iorq->ios2_Req.io_Error = 0;
        
//    	VFPrintf (my->log, "\n- BeginIO iorq: %x", (ULONG)iorq);
	Debug (my->log, "\n- BeginIO ");
//	DebugHex (my->log, iorq->ios2_Req.io_Unit);
	Debug (my->log, "command ");
	DebugHex16 (my->log, iorq->ios2_Req.io_Command);
	Debug (my->log, "flags ");
	DebugHex16 (my->log, iorq->ios2_Req.io_Flags);
            
        switch (iorq->ios2_Req.io_Command) {

            case CMD_WRITE:
                FPuts (my->log, "\n CMD_WRITE");
                complete = CmdWrite (iorq, my);
                break;

            case S2_DEVICEQUERY:
                FPuts (my->log, "\n S2_DEVICEQUERY");
                complete = CmdS2DeviceQuery ((APTR)iorq, my);
                break;

            case S2_GETSTATIONADDRESS:
                FPuts (my->log, "\n S2_GETSTATIONADDRESS");
                complete = CmdGetStationAddress ((APTR)iorq, my);
                break;

            case S2_CONFIGINTERFACE:
                FPuts (my->log, "\n S2_CONFIGINTERFACE");
                complete = CmdConfigInterface ((APTR) iorq, my);
                break;

            case S2_ADDMULTICASTADDRESS:
                FPuts (my->log, "\n S2_ADDMULTICASTADDRESS n/a");
                break;

            case S2_DELMULTICASTADDRESS:
                FPuts (my->log, "\n S2_DELMULTICASTADDRESS n/a");
                break;

            case S2_MULTICAST:
                FPuts (my->log, "\n S2_MULTICAST");
                complete = CmdWrite ((APTR)iorq, my);
                break;
                
            case S2_BROADCAST:
                FPuts (my->log, "\n S2_BROADCAST");
                complete = CmdBroadcast ((APTR)iorq, my);
                break;
                
            case S2_TRACKTYPE:
                FPuts (my->log, "\n S2_TRACKTYPE n/a");
                break;
            case S2_UNTRACKTYPE:
                FPuts (my->log, "\n S2_UNTRACKTYPE n/a");
                break;
            case S2_GETTYPESTATS:
                FPuts (my->log, "\n S2_GETTYPESTATS n/a");
                break;

            case S2_GETSPECIALSTATS:
                FPuts (my->log, "\n S2_GETSPECIALSTATS");
                complete = CmdGetSpecialStats ((APTR)iorq, my);
                break;

            case S2_GETGLOBALSTATS:
                FPuts (my->log, "\n S2_GETGLOBALSTATS");
                complete = CmdGetGlobalStats ((APTR)iorq, my);
                break;

            case S2_ONEVENT:
                FPuts (my->log, "\n S2_ONEVENT n/a");
                break;
            case S2_READORPHAN:
                FPuts (my->log, "\n S2_READORPHAN n/a");
                break;

            case S2_ONLINE:
                FPuts (my->log, "\n S2_ONLINE");
                complete = CmdOnline ((APTR)iorq, my);
                break;

            case S2_OFFLINE:
                FPuts (my->log, "\n S2_OFFLINE");
                complete = CmdOffline ((APTR)iorq, my);
                break;                

            default:
                FPuts (my->log, "\n CMD???");
                break;
        }    
            
//    }
    	
//   if (complete && ((iorq->ios2_Req.io_Flags & IOF_QUICK) == 0))
      ReplyMsg ((APTR)iorq);
    	
    	

}

void AbortIO (struct IORequest *) {
}



static BOOL CmdS2DeviceQuery (
    struct IOSana2Req *iorq,
    struct DevBase *base)
{

   struct Sana2DeviceQuery *info;
   ULONG size_available, size;

   /* Copy device info */

   info = iorq->ios2_StatData;
   size = size_available = info->SizeAvailable;
   if (size > sizeof (struct Sana2DeviceQuery))
      size = sizeof (struct Sana2DeviceQuery);

   CopyMem (&sana2_info, info, size);

   info->SizeAvailable = size_available;
   info->SizeSupplied = size;

   /* Return */

   return TRUE;
}


static BOOL CmdGetStationAddress (
    struct IOSana2Req *iorq,
    struct DevBase *base)
{ 
   /* Copy addresses */

//   CopyMem (address, iorq->ios2_SrcAddr, ADDRESS_SIZE);
   CopyMem (default_address, iorq->ios2_DstAddr, ADDRESS_SIZE);

   return TRUE;
}

static BOOL CmdGetGlobalStats (struct IOSana2Req *iorq, struct DevBase *base)
{
   CopyMem (stats, iorq->ios2_StatData, sizeof (struct Sana2DeviceStats));
   return TRUE;
}


static BOOL CmdBroadcast (struct IOSana2Req *iorq, struct DevBase *base)
{
   /* Fill in the broadcast address as destination */

   *((ULONG *)iorq->ios2_DstAddr) = 0xffffffff;
   *((UWORD *)(iorq->ios2_DstAddr + 4)) = 0xffff;

   /* Queue the write as normal */

   return CmdWrite (iorq, base);
}



/************************************************************************
 *
 * CmdWrite
 *
 ************************************************************************/ 
static BOOL CmdWrite (struct IOSana2Req *iorq, MyBase_t *my)
{

   BYTE error = 0;
   ULONG wire_error;
   BOOL complete = FALSE;

	UBYTE pkt [2048];

   /* Check request is valid */

   
   if((flags & UNITF_ONLINE) == 0)
   {
      error = S2ERR_OUTOFSERVICE;
      wire_error = S2WERR_UNIT_OFFLINE;
   }
   else if ((iorq->ios2_Req.io_Command == S2_MULTICAST) &&
            ((iorq->ios2_DstAddr [0]& 0x1) == 0))
   {
      error = S2ERR_BAD_ADDRESS;
      wire_error = S2WERR_BAD_MULTICAST;
   }

   /* Queue request for sending */

   if (error == 0) {
//      PutRequest (request_ports [WRITE_QUEUE], (APTR)iorq, base);

        // > /dev/nul
        //iorq->ios2_Req.io_Error = 0;
        //iorq->ios2_WireError = 0;
        //complete = TRUE;

        VFPrintf (my->log, "\n CmdWrite %d bytes: \n", iorq->ios2_DataLength);

    // cannot access the data other than via ios2_BufferManagement 
    
        { UBYTE i; 

		//p [0] = p [1] = p [2] = p [3] = 0x77;

            tx_function (pkt, iorq->ios2_Data, iorq->ios2_DataLength);
            
            for (i = 0; i < iorq->ios2_DataLength; i++)
                DebugHex (my->log, pkt [i]);
	    Flush (my->log);

        }
  
                
    }
    
   else
   {
      iorq->ios2_Req.io_Error = error;
      iorq->ios2_WireError = wire_error;
      complete = TRUE;
   }

   /* Return */

   return complete;
}

static BOOL CmdOnline (struct IOSana2Req *iorq, struct DevBase *my)
{
   struct DevUnit *unit;
   BYTE error = 0;
   ULONG wire_error;
   UWORD i;

   /* Check request is valid */

   if ((flags & UNITF_CONFIGURED) == 0)
   {
      error = S2ERR_BAD_STATE;
      wire_error = S2WERR_NOT_CONFIGURED;
   }

   /* Clear global and special stats and put adapter back online */

   if ((error == 0) && ((flags & UNITF_ONLINE) == 0))
   {
      stats.PacketsReceived = 0;
      stats.PacketsSent = 0;
      stats.BadData = 0;
      stats.Overruns = 0;
      stats.UnknownTypesReceived = 0;
      stats.Reconfigurations = 0;

      for (i = 0; i < STAT_COUNT; i ++)
         special_stats [i] = 0;

      GoOnline (my);
   }

   /* Return */

   iorq->ios2_Req.io_Error = error;
   iorq->ios2_WireError = wire_error;
   return TRUE;
}




static BOOL CmdOffline (struct IOSana2Req *iorq, struct DevBase *my)
{
   /* Put adapter offline */

   if((flags & UNITF_ONLINE) != 0)
      GoOffline (my);

   /* Return */

   return TRUE;
}


void GoOnline (struct DevBase *my)
{    
    flags |= UNITF_ONLINE;

   /* Enable the transceiver */


   /* Record start time and report Online event */

//   GetSysTime (stats.LastStart);
   
//   ReportEvents (unit, S2EVENT_ONLINE, base);

   return;
}


void GoOffline (struct DevBase *my)
{
   flags &= ~UNITF_ONLINE;

      /* Stop interrupts */


      /* Stop transmission and reception */

      /* Turn off media functions */


   /* Flush pending read and write requests */

//   FlushUnit (unit, WRITE_QUEUE, S2ERR_OUTOFSERVICE, my);    

   /* Report Offline event and return */

//   ReportEvents(unit,S2EVENT_OFFLINE,base);
   return;
}


static BOOL CmdConfigInterface (struct IOSana2Req *iorq, struct DevBase *my)
{

   /* Configure adapter */


   if ((flags & UNITF_CONFIGURED) == 0)
   {
      CopyMem (iorq->ios2_SrcAddr, address, ADDRESS_SIZE);
      flags |= UNITF_CONFIGURED;
   }
   else
   {
      iorq->ios2_Req.io_Error = S2ERR_BAD_STATE;
      iorq->ios2_WireError = S2WERR_IS_CONFIGURED;
   }

   /* Return */

   return TRUE;
}


static BOOL CmdGetSpecialStats(struct IOSana2Req *iorq, struct DevBase *my)
{

   UWORD i, stat_count;
   struct Sana2SpecialStatHeader *header;
   struct Sana2SpecialStatRecord *record;

   /* Fill in stats */

   header = iorq->ios2_StatData;
   record = (APTR)(header + 1);

   stat_count = header->RecordCountMax;
   if (stat_count > STAT_COUNT)
      stat_count = STAT_COUNT;

   for (i = 0; i < stat_count; i ++)
   {
      record->Type = (S2WireType_Ethernet << 16) + i;
      record->Count = special_stats [i];
//      record->String = (TEXT *)&unit->openers.mlh_Tail;
        record->String = 0;
      record++;
   }

   header->RecordCountSupplied = stat_count;

   /* Return */

   return TRUE;
}




