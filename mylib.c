#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/devices.h>
#include <exec/libraries.h>
#include <exec/initializers.h>
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <libraries/dos.h>
#include <devices/timer.h>

#include <exec/semaphores.h>

#include "devices/sana2.h"
#include "devices/sana2specialstats.h"

#include "device.h"


#define VERSION  37
#define REVISION 01

#define MYLIBNAME "my"
#define MYLIBVER  " 37.01 (25.07.2013)"

#define UNIT_COUNT 1

char device_name [] = "my.device";	//"my.library";		// MYLIBNAME ".library";
char MyLibID   [] = "37.01 (25.07.2013)";	//MYLIBNAME MYLIBVER;

char VERSTRING [] = "\0$VER: my 37.01 (25.07.2013)"; 	// EXLIBNAME EXLIBVER;


struct ExecBase 	*SysBase  		= NULL;
struct DOSBase		*DOSBase 		= NULL;
struct UtilityBase	*UtilityBase 	= NULL;


LONG LibStart (void) {
	return -1;
}



/*

typedef struct {
 	struct Library            my_LibNode;
 	APTR                      my_SegList;
 	struct ExecBase           *my_SysBase;
	struct DOSBase            *my_DOSBase;
// struct IntuitionBase  *exb_IntuitionBase;
// struct GfxBase        *exb_GfxBase;
    struct UtilityBase          *my_UtilityBase;
	BPTR                      log;
	struct MinList             units;
} MyBase_t;
*/

__saveds struct MyBase * InitLib (__reg ("a6") struct ExecBase  *sysbase,
                                  __reg ("a0") APTR 		seglist,
                                  __reg ("d0") struct MyBase 	*my);



//__saveds struct MyBase * OpenLib (APTR);
__saveds BYTE DevOpen ( __reg ("a6") MyBase_t *my,
                        __reg ("a1") struct IOSana2Req *iorq,
                        __reg ("d1") ULONG flags,
                        __reg ("d0") ULONG unit_num
	);
__saveds BYTE DevOpenNew ( __reg ("a6") MyBase_t *my, __reg ("a1") struct IOSana2Req *iorq, __reg ("d1") ULONG flags, __reg ("d0") ULONG unit_num);
//__saveds APTR CloseLib (APTR);
__saveds APTR CloseLib (__reg ("a6") struct MyBase *my);
//__saveds APTR ExpungeLib (APTR);
__saveds APTR ExpungeLib (__reg ("a6") struct MyBase *my);
__saveds APTR DevExpunge (__reg ("a6") struct MyBase *base);
ULONG ExtFuncLib (void);

__saveds void BeginIO (	__reg ("a6") MyBase_t *my, 
                        __reg ("a1") struct IOSana2Req *iorq);

void AbortIO (struct IORequest *);


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

extern APTR InitTab [];
extern APTR EndResident; /* below */

/* __aligned */ struct Resident ROMTag =     /* do not change */
{
	RTC_MATCHWORD,
	&ROMTag,
 	&EndResident,
 	RTF_AUTOINIT,
 	VERSION,
 	NT_DEVICE,	// NT_LIBRARY,
 	0,
 	&device_name [0],
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
        0x80,	10, (ULONG) &device_name [0],
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
 	(APTR              *) &FuncTab [0],
 	(struct MyDataInit *) &DataTab,
 	(APTR)                InitLib
};

static const ULONG rx_tags[]=
{
   S2_CopyToBuff,
//   S2_CopyToBuff16
};


static const ULONG tx_tags[]=
{
   S2_CopyFromBuff,
//   S2_CopyFromBuff16,
//   S2_CopyFromBuff32
};



APTR FuncTab [] = {
//	(APTR) DevOpen,	// 	(APTR) OpenLib,
    (APTR) DevOpenNew, 
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
                                  __reg ("a0") APTR seglist,
                                  __reg ("d0") struct MyBase 	*my) 
{


 MyBase = my;

 MyBase->my_SysBase = sysbase;
 MyBase->my_SegList = seglist;

 MyBase->log = NULL;
 
 NewList ((APTR)(&MyBase->units));

 if (L_OpenLibs (MyBase)) 
	return (MyBase);



 L_CloseLibs ();

  {
   ULONG negsize, possize, fullsize;
   UBYTE *negptr = (UBYTE *) MyBase;

   negsize  = MyBase->device.dd_Library.lib_NegSize;
   possize  = MyBase->device.dd_Library.lib_PosSize;
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

void Debug (char *s) {
	FPuts (MyBase->log, s);
}

void DebugHex (UBYTE b) {
register UBYTE nibble;
	nibble = b >> 4;
	FPutC (MyBase->log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);
	nibble = b & 0x0f;
	FPutC (MyBase->log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);
	FPutC (MyBase->log, 0x20);
//	Flush (MyBase->log);

}

void DebugHex16 (UWORD w) {
register UBYTE nibble;
register UBYTE i;

	for (i = 0; i < 4; i ++) {
		nibble = (w >> (12 - i * 4)) & 0x0f;
		FPutC (MyBase->log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);
	}

	FPutC (MyBase->log, 0x20);

	Flush (MyBase->log);
}


void DebugHex32 (ULONG l) {
register UBYTE nibble;
register UBYTE i;

	for (i = 0; i < 8; i ++) {
		nibble = (l >> (12 - i * 4)) & 0x0f;
		FPutC (MyBase->log, (nibble < 10) ? '0' + nibble : 'A' - 10 + nibble);
    }

	FPutC (MyBase->log, 0x20);
	
	
//	Flush (MyBase->log);
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
	//	Debug (my->log, "\n- DevOpen");
		//VFPrintf (my->log, "\n- DevOpen (unit %d, flags %x)",
	          //  unit_num, flags
	    Seek (my->log, 0, OFFSET_END);
		Debug ("\n\n- DevOpen unit ");
		DebugHex (unit_num);
		Debug (", flags ");
		DebugHex32 (flags);
        };
	
	my->device.dd_Library.lib_OpenCnt ++;
	my->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
	
	request->ios2_Req.io_Unit = NULL; // FIXME
	tag_list = request->ios2_BufferManagement;
	request->ios2_BufferManagement = NULL;
	
	// tags
	
    	tx_function = (APTR)GetTagData (S2_CopyFromBuff, NULL, tag_list);
	if (tx_function == NULL)
		Debug ("\n tx_function EMPTY");

	// Task
//	InitializeAndStartTask (my);
	
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

__saveds struct MyBase * OpenLib (__reg ("a6") struct MyBase *my) {

	if (my->log == NULL)
		my->log = Open ("T:log", MODE_READWRITE);

	if (my->log) {
        Seek (my->log, 0, OFFSET_END);
       	Debug ("\n- OpenLib");		
    }
	
	my->device.dd_Library.lib_OpenCnt ++;

	my->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
	
	return (my);
}


/*****************************************************************************
 *
 * DevOpenNew
 *
 *****************************************************************************/
__saveds BYTE DevOpenNew (__reg ("d0") ULONG unit_num,
                     __reg ("a1") struct IOSana2Req *request,
                     __reg ("d1") ULONG flags,
                     __reg ("a6") struct MyBase *base) {
   struct DevUnit *unit;
   BYTE error = 0;
   struct Opener *opener;
   struct TagItem *tag_list;
   UWORD i;

   base->device.dd_Library.lib_OpenCnt ++;
   base->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

    // Log
   	if (base->log == NULL)
    	base->log = Open ("T:log", MODE_READWRITE);

	if (base->log) {
	    Seek (base->log, 0, OFFSET_END);
		Debug ("\n\n- DevOpenNew unit ");
		DebugHex (unit_num);
		Debug (", flags ");
		DebugHex32 (flags);
        };


   request->ios2_Req.io_Unit = NULL;
   tag_list = request->ios2_BufferManagement;
   request->ios2_BufferManagement = NULL;

   /* Check request size and unit number */

   if ((request->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req)) ||
       (unit_num >= UNIT_COUNT))
      error = IOERR_OPENFAIL;

   /* Get the requested unit */

   if (error == 0) {
      request->ios2_Req.io_Unit = unit = GetUnit (unit_num, base);
      if (unit == NULL)
         error = IOERR_OPENFAIL;
   }

   /* Handle device sharing */

   if (error == 0) {
      if ((unit->open_count != 0) && (((unit->flags & UNITF_SHARED) == 0) ||
         ((flags & SANA2OPF_MINE) !=0)))
         error = IOERR_UNITBUSY;
      unit->open_count ++;
   }

   if(error == 0)
   {
      if((flags & SANA2OPF_MINE) == 0)
         unit->flags |= UNITF_SHARED;
      else if ((flags & SANA2OPF_PROM) != 0)
         unit->flags |= UNITF_PROM;

      /* Set up buffer-management structure and get hooks */

      request->ios2_BufferManagement = opener =
         (APTR)AllocVec (sizeof (struct Opener), MEMF_PUBLIC);
      if (opener == NULL)
         error = IOERR_OPENFAIL;
   }

   if(error==0)
   {
      NewList(&opener->read_port.mp_MsgList);
      opener->read_port.mp_Flags=PA_IGNORE;
      NewList((APTR)&opener->initial_stats);

      for(i=0;i<2;i++)
         opener->rx_function=(APTR)GetTagData(rx_tags[i],
            (ULONG)opener->rx_function,tag_list);
      for(i=0;i<3;i++)
         opener->tx_function=(APTR)GetTagData(tx_tags[i],
            (ULONG)opener->tx_function,tag_list);

      opener->filter_hook=(APTR)GetTagData(S2_PacketFilter,NULL,tag_list);
      opener->dma_tx_function = NULL; //   (APTR)GetTagData(S2_DMACopyFromBuff32,NULL,tag_list);

      Disable();
      AddTail((APTR)&unit->openers,(APTR)opener);
      Enable();
   }

   /* Back out if anything went wrong */

   if (error != 0)
      DevClose (request, base);

   /* Return */

   request->ios2_Req.io_Error = error;
   return error;
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

__saveds APTR CloseLib (__reg ("a6") struct MyBase *my) {
	my->device.dd_Library.lib_OpenCnt --;

	if (my->log) {
		Debug ("\n- CloseLib");
		Close (my->log);
		my->log = NULL;
	}

	if (!my->device.dd_Library.lib_OpenCnt) {
		if (my->device.dd_Library.lib_Flags & LIBF_DELEXP) {
	     		return (ExpungeLib (my));
    		}
  	}

 return (NULL);
}


__saveds APTR DevClose (__reg ("a1") struct IOSana2Req *request, __reg ("a6") struct MyBase *base) {
struct DevUnit *unit;
APTR seg_list;
struct Opener *opener;

   /* Free buffer-management resources */

   opener = (APTR)request->ios2_BufferManagement;
   if(opener != NULL) {
      Disable();
      Remove ((APTR)opener);
      Enable();
      FreeVec (opener);
   }

   /* Delete the unit if it's no longer in use */

   unit = (APTR)request->ios2_Req.io_Unit;
   if (unit != NULL) {
      if ((-- unit->open_count) == 0) {
         Remove ((APTR)unit);
         DeleteUnit (unit, base);
      }
   }

   /* Expunge the device if a delayed expunge is pending */

   seg_list = NULL;

   if ((-- base->device.dd_Library.lib_OpenCnt) == 0) {
      if ((base->device.dd_Library.lib_Flags & LIBF_DELEXP) != 0)
         seg_list = DevExpunge (base);
   }

   return seg_list;
}



__saveds APTR DevExpunge (__reg ("a6") struct MyBase *base) {
APTR seg_list;

   if (base->device.dd_Library.lib_OpenCnt == 0) {
      seg_list = base->my_SegList;
      Remove ((APTR)base);
      DeleteDevice (base);
   }
   else {
      base->device.dd_Library.lib_Flags |= LIBF_DELEXP;
      seg_list = NULL;
   }

   return seg_list;
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

__saveds APTR ExpungeLib (__reg ("a6") struct MyBase *my) {
 MyBase_t *MyBase = my;
 APTR seglist;

 if (!MyBase->device.dd_Library.lib_OpenCnt)
  {
   ULONG negsize, possize, fullsize;
   UBYTE *negptr = (UBYTE *) MyBase;

   seglist = MyBase->my_SegList;

   Remove((struct Node *)MyBase);

   L_CloseLibs();

   negsize  = MyBase->device.dd_Library.lib_NegSize;
   possize  = MyBase->device.dd_Library.lib_PosSize;
   fullsize = negsize + possize;
   negptr  -= negsize;

   FreeMem (negptr, fullsize);


   return (seglist);
  }

 MyBase->device.dd_Library.lib_Flags |= LIBF_DELEXP;

 return(NULL);
}


/*****************************************************************************
 *
 *
 *
 *****************************************************************************/

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

ULONG ExtFuncLib (void) {
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

//    OpenDevice (TIMERNAME, UNIT_VBLANK, (APTR)&MyBase->timer_request, 0);
	
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




/*****************************************************************************
 *
 * BeginIO
 *
 *****************************************************************************/
__saveds void BeginIO ( __reg ("a6") MyBase_t *base, __reg ("a1") struct IOSana2Req *iorq) {
struct DevUnit *unit;

    iorq->ios2_Req.io_Error = 0;
    unit = (APTR)iorq->ios2_Req.io_Unit;
        
	Debug ("\n- BeginIO ");
//	DebugHex (my->log, iorq->ios2_Req.io_Unit);
	Debug ("command ");
	DebugHex16 (iorq->ios2_Req.io_Command);
	Debug ("flags ");
	DebugHex32 (iorq->ios2_Req.io_Flags);
            
        switch (iorq->ios2_Req.io_Command) {

            case CMD_WRITE:
                Debug ("\n CMD_WRITE");
                break;
            case S2_DEVICEQUERY:
                Debug ("\n S2_DEVICEQUERY");
                break;
            case S2_GETSTATIONADDRESS:
                Debug ("\n S2_GETSTATIONADDRESS");
                break;
            case S2_CONFIGINTERFACE:
                Debug ("\n S2_CONFIGINTERFACE");
                break;
            case S2_ADDMULTICASTADDRESS:
                Debug ("\n S2_ADDMULTICASTADDRESS");
                break;
            case S2_DELMULTICASTADDRESS:
                Debug ("\n S2_DELMULTICASTADDRESS");
                break;
            case S2_MULTICAST:
                Debug ("\n S2_MULTICAST");
                break;
            case S2_BROADCAST:
                Debug ("\n S2_BROADCAST");
                break;            
            case S2_TRACKTYPE:
                Debug ("\n S2_TRACKTYPE");
                break;
            case S2_UNTRACKTYPE:
                Debug ("\n S2_UNTRACKTYPE");
                break;
            case S2_GETTYPESTATS:
                Debug ("\n S2_GETTYPESTATS");
                break;
            case S2_GETSPECIALSTATS:
                Debug ("\n S2_GETSPECIALSTATS");
                break;
            case S2_GETGLOBALSTATS:
                Debug ("\n S2_GETGLOBALSTATS");
                break;
            case S2_ONEVENT:
                Debug ("\n S2_ONEVENT");
                break;
            case S2_READORPHAN:
                Debug ("\n S2_READORPHAN");
                break;
            case S2_ONLINE:
                Debug ("\n S2_ONLINE");
                break;
            case S2_OFFLINE:
                Debug ("\n S2_OFFLINE");
                break;                

            default:
                Debug ("\n CMD???");
                break;
        }    
            
   if (AttemptSemaphore (&unit->access_lock))
      ServiceRequest (iorq, base);
   else {
      PutRequest (unit->request_ports [GENERAL_QUEUE], (APTR)iorq, base);
   }

   return;
    	
    	

}

void AbortIO (struct IORequest *) {
}



VOID DeleteDevice (struct MyBase *base) {
UWORD neg_size, pos_size;

   /* Close devices */

//   CloseDevice ((APTR)&base->timer_request);

   /* Close libraries */

   if (base->my_UtilityBase != NULL)
      CloseLibrary ((APTR)base->my_UtilityBase);

   /* Free device's memory */

   neg_size = base->device.dd_Library.lib_NegSize;
   pos_size = base->device.dd_Library.lib_PosSize;
   FreeMem ((UBYTE *)base-neg_size, pos_size + neg_size);

   return;
}

