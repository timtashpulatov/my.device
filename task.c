#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/tasks.h>


#define TASK_PRIORITY 0
#define STACK_SIZE 4096


IMPORT struct ExecBase *AbsExecBase;

extern char device_name [];


/************************************************************************
 *
 * UnitTask
 *
 ************************************************************************/
void UnitTask () {
struct Task *task;
struct IORequest *request;
struct DevUnit *unit;
struct DevBase *base;
struct MsgPort *general_port;
ULONG signals, wait_signals, general_port_signal;

   /* Get parameters */

   task = AbsExecBase->ThisTask;
//   unit = task->tc_UserData;
//   base = unit->device;
    
    base = task->tc_UserData;
//    Debug ("\n\n --- UnitTask started");

    
    while (1) {
        signals = Wait (wait_signals);
    }

}

/************************************************************************
 *
 * InitializeAndStartTask
 *
 ************************************************************************/
void InitializeAndStartTask (struct DevBase *my) {
BOOL success = TRUE;
//   struct DevUnit *unit;
struct Task *task;
struct MsgPort *port;
UBYTE i;
APTR stack;
//struct Interrupt *card_removed_int,*card_inserted_int,*card_status_int;

    /* Create a new task */

    Debug ("\n- InitializeAndStartTask");

    task = (struct Task *)AllocMem (sizeof (struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    if (task == NULL) {
        Debug ("\n*** task AllocMem failure");
         success = FALSE;
    }


    if (success) {
        stack = (APTR) AllocMem (STACK_SIZE, MEMF_PUBLIC);
        if (stack == NULL) {
            Debug ("\n*** stack AllocMem failure");
            success = FALSE;
        }
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

        if (AddTask (task, UnitTask, NULL) == NULL) {
            Debug ("\n*** AddTask failure");
            success = FALSE;
        }
   }

   /* Send the unit to the new task */

   if (success) {
      task->tc_UserData = my;
      Debug ("\n--- Task creation succes.");
    }

   if (!success) {
      Debug ("\n*** failure, need to clean up");      
//      DeleteUnit (unit,base);
//      unit = NULL;
   }

}

