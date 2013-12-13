#ifndef NEWSTYLE_H
#define NEWSTYLE_H

#define DRIVE_NEWSTYLE  (0x4E535459L)   /* 'NSTY' */

#define NSCMD_TD_READ64     0xC000
#define NSCMD_TD_WRITE64    0xC001
#define NSCMD_TD_SEEK64     0xC002
#define NSCMD_TD_FORMAT64   0xC003

#define NSCMD_TDF_EXTCOM (1<<13) /* for internal use only! */

#define NSCMD_ETD_READ64     (NSCMD_TD_READ64|NSCMD_TDF_EXTCOM)
#define NSCMD_ETD_WRITE64    (NSCMD_TD_WRITE64|NSCMD_TDF_EXTCOM)
#define NSCMD_ETD_SEEK64     (NSCMD_TD_SEEK64|NSCMD_TDF_EXTCOM)
#define NSCMD_ETD_FORMAT64   (NSCMD_TD_FORMAT64|NSCMD_TDF_EXTCOM)

#define NSDEVTYPE_UNKNOWN       0   /* No suitable category, anything */
#define NSDEVTYPE_GAMEPORT      1   /* like gameport.device */
#define NSDEVTYPE_TIMER         2   /* like timer.device */
#define NSDEVTYPE_KEYBOARD      3   /* like keyboard.device */
#define NSDEVTYPE_INPUT         4   /* like input.device */
#define NSDEVTYPE_TRACKDISK     5   /* like trackdisk.device */
#define NSDEVTYPE_CONSOLE       6   /* like console.device */
#define NSDEVTYPE_SANA2         7   /* A >=SANA2R2 networking device */
#define NSDEVTYPE_AUDIO         8   /* like audio.device */
#define NSDEVTYPE_CLIPBOARD     9   /* like clipboard.device */
#define NSDEVTYPE_PRINTER       10  /* like printer.device */
#define NSDEVTYPE_SERIAL        11  /* like serial.device */
#define NSDEVTYPE_PARALLEL      12  /* like parallel.device */

#define NSCMD_DEVICEQUERY   0x4000

struct NSDeviceQueryResult
{
    /*
    ** Standard information
    */
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */

    /*
    ** Common information (READ ONLY!)
    */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    UWORD   *SupportedCommands;     /* 0 terminated list of cmd's   */

    /* May be extended in the future! Check SizeAvailable! */
};

#endif
