/*
	FOSSIL Interface
*/

#ifndef  __TIME_T
#define  __TIME_T
typedef unsigned long time_t;
#endif

#define	FSIG	0x1954			/* Magic number for FOSSIL */
#define	SETBRK	0x4f50			/* Set break address signature */

#define	MAX_FUNC	0x1a		/* Maximum function required in FOSSIL */

#define	PORTIN	1
#define	KBDIN	2

/* Port Status flags */

#define	PS_RXCHARS	0x0100		/* Characters in input buffer */
#define	PS_RXOVER	0x0200		/* Receive buffer overrun */
#define	PS_MASK		0x0008		/* Bits always set */
#define	PS_TXCHARS	0x2000		/* Transmit buffer not full */
#define	PS_TXEMPTY	0x4000		/* Transmit buffer is empty */
#define	PS_CARRIER	0x0080		/* Carrier detect is high */

/* Communications Parameter flags */

#define CP_SPEED	0x01e0		/* Mask for speed bits */
#define	CP_B19200	0x0000		/*  19200 baud */
#define	CP_B38400	0x0020		/*  38400 baud */
#define	CP_B300		0x0040		/*    300 baud */
#define	CP_B600		0x0060		/*    600 baud */
#define	CP_B1200	0x0080		/*   1200 baud */
#define	CP_B2400	0x00a0		/*   2400 baud */
#define	CP_B4800	0x00c0		/*   4800 baud */
#define	CP_B9600	0x00e0		/*   9600 baud */
#define	CP_B57600	0x0100		/*  57600 baud */
#define	CP_B115200	0x0120		/* 115200 baud */

#define	CP_PARITY	0x0018		/* Mask for parity bits */
#define	CP_PARNONE	0x0000		/* No parity */
#define	CP_PARSPC	0x0010		/* Space/mark parity */
#define	CP_PARODD	0x0008		/* Odd parity */
#define	CP_PAREVEN	0x0018		/* Even parity */

#define	CP_STOPB	0x0004		/* Stop bit mask */
#define	CP_STOP1	0x0000		/* 1 stop bit */
#define	CP_STOP2	0x0004		/* 2 stop bits */

#define	CP_CHRLEN	0x0003		/* Character length mask */
#define	CP_CHLEN8	0x0003		/* 8 data bits */
#define	CP_CHLEN7	0x0002		/* 7 data bits */
#define	CP_CHLEN6	0x0001		/* 6 data bits */

#define	CP_8N1		(CP_CHLEN8|CP_STOP1|CP_PARNONE)
#define	CP_8N2		(CP_CHLEN8|CP_STOP2|CP_PARNONE)
#define	CP_7O1		(CP_CHLEN7|CP_STOP1|CP_PARODD)
#define	CP_7E1		(CP_CHLEN7|CP_STOP1|CP_PAREVEN)
#define	CP_7N1		(CP_CHLEN7|CP_STOP1|CP_PARNONE)
#define	CP_7O2		(CP_CHLEN7|CP_STOP2|CP_PARODD)
#define	CP_7E2		(CP_CHLEN7|CP_STOP2|CP_PAREVEN)
#define	CP_7N2		(CP_CHLEN7|CP_STOP2|CP_PARNONE)

#define	SET_DTR		0x0001
#define	RESET_DTR	0x0000

#define	RXBUFSZ		4096

/* Flow Control flags */

#define	FC_LOCXON	0x0001		/* Enables local Xon/Xoff */
#define	FC_LOCCTS	0x0002		/* Enables local Cts/Dte */
#define	FC_REMXON	0x0008		/* Enables remove Xon/Xoff */
#define FC_XONOFF	0x0000		/* Disables all flow control */

/* Break detect flags */

#define	BRK_ON		0x0001		/* Enable ^C/^K checking */
#define	BRK_OFF		0x0000		/* Disable ^C/^K checking */
#define	SRELEASE	0x0002		/* Force Xoff transmitter */

/* Useful macros */

#define	carrier(p)	(ComPortStat(p) & PS_CARRIER)
#define position(t)	(((t->y+t->cy)*80)+t->x+t->cx)

#define	OK		0
#define	ERROR		-1
#define	NO_CARRIER	-2
#define	TIMEOUT		-3

/* Fossil information struct */

struct finfo {
	char maxfunc;		 /* Maximum function call supported */
	char revision;		 /* FOSSIL revision number */
};

struct tinfo {
	char timerint;				/* Timer tick interrupt number */
	char ticksecs;				/* Number of timer ticks per second */
	char milltick;				/* Number of milliseconds per timer tick */
};

struct fossildata {
	short fdsize;				/* Offset 0 (word) = Structure size */
	char specver;				/*        2 (byte) = FOSSIL spec version */
	char drvlvl;				/*        3 (byte) = Driver rev level */
	char far *drvid;			/*        4 (dwrd) = Pointer to ASCII ID */
	short rxsize;				/*        8 (word) = Input buffer size */
	short rxavail;				/*       0A (word) = Bytes avail (input) */
	short txsize;				/*       0C (word) = Output buffer size */
	short txavail;				/*       0E (word) = Bytes avail (output) */
	char scnwid;				/*       10 (byte) = Screen width, chars */
	char scnlen;				/*       11 (byte) = Screen height, chars */
	short bdmask;				/*       12 (word) = Baud rate mask */
	char filler[2];
};

extern  int pascal far ComPortStat (int port);
extern  int pascal far ComRxChar (int port);
extern  int pascal far ComRxPeek (int port);
extern  int pascal far ComKbPeek (void);
extern  int pascal far ComKbChar (void);
extern  int pascal far ComInstAPI (int code, void far (*entry)(void));
extern  int pascal far ComDeInstAPI (int code, void far *(entry)(void));

/*lint +fvr */
extern  int pascal far ComPortInit (int port, void far *flagptr, struct finfo far *infoptr);
extern  void pascal far ComPortDeInit (int port);
extern  int pascal far ComPortSet (int port, unsigned parms);
extern  int pascal far ComSetDtr (int port, int state);
extern  void pascal far ComFlowCtl (int port, unsigned mask);
extern  int pascal far ComBrkChk (int port, unsigned mask);
extern  void pascal far ComTxBrk (int port, int flag);
extern  void pascal far ComDrvInfo (int port, void far *buf, int sz);
extern  int pascal far ComTxChar (int port, unsigned char chr);
extern  int pascal far ComTxChrNW (int port, unsigned char chr);
extern  void pascal far ComRxPurge (int port);
extern  void pascal far ComTxFlush (int port);
extern  void pascal far ComTxPurge (int port);
extern  int pascal far ComRxBlk (int port, char far *buf, int cnt);
extern  int pascal far ComTxBlk (int port, char far *buf, int cnt);
extern  void pascal far ComSetCur (int col, int row);
extern  void pascal far ComGetCur (int far *row, int far *col);
extern  void pascal far ComWrAnsi (unsigned char ch);
extern  void pascal far ComWrChar (unsigned char ch);
extern  void pascal far ComCdWatch (int port, int flag);
extern  void pascal far ComReboot (int flag);
extern  void pascal far ComTimer (int port, struct tinfo far *t);
extern  int pascal far ComInstFunc (int flag, int far (*func)());
/*lint -fvr */

