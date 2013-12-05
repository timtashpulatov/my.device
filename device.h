#ifndef DEVICE_H
#define DEVICE_H


#define ADDRESS_SIZE 6
#define HEADER_SIZE 14
#define PREAMBLE_SIZE 4
#define PACKET_DEST 0
#define PACKET_SOURCE 6
#define PACKET_TYPE 12
#define PACKET_DATA 14
#define MTU 1500
#define MAX_PACKET_SIZE ((HEADER_SIZE)+(MTU))

#define STAT_COUNT 3



#define NSCMD_DEVICEQUERY   0x4000

/* Unit flags */

#define UNITF_SHARED        (1<<0)
#define UNITF_ONLINE        (1<<1)
//#define UNITF_HAVECARD      (1<<2)
//#define UNITF_HAVEADAPTER   (1<<3)
#define UNITF_CONFIGURED    (1<<4)
#define UNITF_PROM          (1<<5)
//#define UNITF_ROADRUNNER    (1<<6)

enum {
   WRITE_QUEUE,
   ADOPT_QUEUE,
   EVENT_QUEUE,
   GENERAL_QUEUE,
//   TIMER_QUEUE,
   REQUEST_QUEUE_COUNT
};


struct MyBase {
 	struct Device            device;
 	APTR                      my_SegList;
 	struct ExecBase           *my_SysBase;
	struct DOSBase            *my_DOSBase;
// struct IntuitionBase  *exb_IntuitionBase;
// struct GfxBase        *exb_GfxBase;
    struct UtilityBase          *my_UtilityBase;
	struct MinList             units;
//	struct timerequest     *timer_request;

    APTR                    io_base;        // for dm9k_ routines

};

struct DevUnit {
   struct MinNode node;
   ULONG unit_num;
   ULONG open_count;
   UWORD flags;  
   struct Task *task;
   struct MsgPort *request_ports [REQUEST_QUEUE_COUNT];
   struct MyBase *device;   //   struct DevBase *device;

//   UBYTE *tuple_buffer;       ?? what for?
   UBYTE *rx_buffer;
   UBYTE *tx_buffer;
   volatile UBYTE *config_base;
   volatile UBYTE *io_base;
   ULONG range_count;
   UBYTE address [ADDRESS_SIZE];
   UBYTE default_address [ADDRESS_SIZE];
   struct MinList openers;
   struct MinList type_trackers;
   struct MinList multicast_ranges;
   struct Interrupt rx_int;
   struct Interrupt tx_int;
   struct Interrupt linkchg_int;
   struct Sana2DeviceStats stats;
   ULONG special_stats [STAT_COUNT];
   struct SignalSemaphore access_lock;
   UWORD rx_filter_cmd;
   
   // 
   UBYTE isr;
   UBYTE tx_busy;
};

struct TypeStats {
   struct MinNode node;
   ULONG packet_type;
   struct Sana2PacketTypeStats stats;
};

struct TypeTracker {
   struct MinNode node;
   ULONG packet_type;
   struct Sana2PacketTypeStats stats;
   ULONG user_count;
};


struct AddressRange {
   struct MinNode node;
   ULONG add_count;
   ULONG lower_bound_left;
   ULONG upper_bound_left;
   UWORD lower_bound_right;
   UWORD upper_bound_right;
};


struct Opener
{
   struct MinNode node;
   struct MsgPort read_port;
   BOOL (*rx_function)(APTR __reg("a0"),APTR __reg("a1"),ULONG __reg("d0"));
   BOOL (*tx_function)(APTR __reg("a0"),APTR __reg("a1"),ULONG __reg("d0"));
   ULONG *(*dma_tx_function)(APTR __reg("a0"));
   struct Hook *filter_hook;
   struct MinList initial_stats;
};


extern char device_name [];

/* Endianness macros */

#define BEWord(A)\
   (A)

#define MakeBEWord(A)\
   BEWord(A)

#define LEWord(A)\
   ({\
      UWORD _LEWord_A=(A);\
      _LEWord_A=((_LEWord_A)<<8)|((_LEWord_A)>>8);\
   })

#define MakeLEWord(A)\
   LEWord(A)

#define BELong(A)\
   (A)

#define LELong(A)\
   ({\
      ULONG _LELong_A=(A);\
      _LELong_A=(LEWord(_LELong_A)<<16)|LEWord((_LELong_A)>>16);\
   })

#define MakeLELong(A)\
   LELong(A)


#endif


extern struct TypeStats *FindTypeStats (struct DevUnit *unit, struct MinList *list, ULONG packet_type, struct MyBase *base);
struct DevUnit *GetUnit (ULONG unit_num, struct MyBase *base);



//#define NODEBUG
void KPrintF (UBYTE *fmt, ...);
