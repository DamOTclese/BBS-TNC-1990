
/* **********************************************************************
   * BBS-TNC - Fredric Rice, FidoNet 1:102/901.0                        *
   *                                                                    *
   * This program requires a FOSSIL driver! X00 or BNU will work. Look  *
   * for a FOSSIL driver on any FidoNet system. It's needed for the     *
   * Async Interruption for the serial I/O.                             *
   *                                                                    *
   * If power is removed from the TNC, we _could_ drop the whole        *
   * session simply by inserting a call to the power check function     *
   * into the main loop. We do not do this, however, so that power may  *
   * be cycled on the TNC while a user is active and we will not drop   *
   * the session.                                                       *
   *                                                                    *
   * Environment variable BBSTNCCONFIG should be set to the path of     *
   * where to find the BBS-TNC.CFG file! If no path information is      *
   * offered, the default directory will be used to see where the       *
   * configuration file is. If the configuration file can't be found,   *
   * then default configuration will be used.                           *
   *                                                                    *
   * Example in AUTOEXEC.BAT:                                           *
   *                                                                    *
   *	set BBSTNCCONFIG=C:\FRED                                        *
   *                                                                    *
   *                                                                    *
   ********************************************************************** */

#include <stdio.h>
#include <bios.h>
#include <time.h>
#include <conio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include "fossil.h"

/* **********************************************************************
   * Here are some defines that are simply constants.                   *
   *                                                                    *
   ********************************************************************** */

#define FALSE   0
#define TRUE    1
#define BOOL    unsigned char

/* **********************************************************************
   * Return values.                                                     *
   *                                                                    *
   * Most likely, these would not be used by any reason unless this     *
   * program is brought up with a batch file.                           *
   *                                                                    *
   ********************************************************************** */

#define Normal_Termination      0
#define Error_Bad_Parameter     10
#define Error_No_Carrier        11
#define Carrier_Lost            12
#define Inactivity_Time_Out     13
#define No_TNC_Power            14
#define Incorrect_Password      15
#define Incorrect_Call_Sign     16
#define No_FOSSIL_Driver        17
#define Non_Active_Port         18

/* **********************************************************************
   * Define macros that the System Operator may wish to change.         *
   *                                                                    *
   ********************************************************************** */

#define Input_Time_Out  (60 * 4)        /* 4 minute keyboard time-out */

/* **********************************************************************
   * Define any other macros.                                           *
   *                                                                    *
   ********************************************************************** */

#define skipspace(x)    while (*x == 0x20 || *x == 0x09) { x++; }

/* **********************************************************************
   * Let's have some globally defined data.                             *
   *                                                                    *
   ********************************************************************** */

    static time_t t_start, t_end;
    static BOOL password_needed, call_sign;
    static char system_password[81];
    static char config_file_path[81];
    static char user_call_sign[15];
    static char myalias[81];

/* **********************************************************************
   * Set the string offered to upper case.                              *
   *                                                                    *
   * toupper() may be a macro depending on your compiler so do not post	*
   * incriment the variable within the toupper() call.                  *
   *                                                                    *
   ********************************************************************** */

static void ucase(char *string)
{
    while (*string) {
        *string = (char)toupper(*string);
        string++;
    }
}

/* **********************************************************************
   * Is the TNC powered up? Data Set Ready should be active if it is.   *
   *                                                                    *
   ********************************************************************** */

static char tnc_power(char com_port)
{
    return((bioscom(3, 0, com_port) & 0x20) == 0x20);
}

/* **********************************************************************
   * If there is data carrier detect, return TRUE, else, return FALSE.  *
   *                                                                    *
   ********************************************************************** */

static BOOL get_carrier_status(char com_port)
{
    return((ComPortStat(com_port) & PS_CARRIER) > 0);
}

/* **********************************************************************
   * If there is a byte at the com port, return it, else return 0.      *
   *                                                                    *
   ********************************************************************** */

static char have_byte(char com_port)
{
    if (ComPortStat(com_port) & PS_RXCHARS)
        return(ComRxChar(com_port));

    return(0);
}

/* **********************************************************************
   * Send the byte to the specified com port.                           *
   *                                                                    *
   * We reset the various timer ticks.                                  *
   *                                                                    *
   ********************************************************************** */

static char send_byte(char byte, char com_port)
{
    ComTxChar(com_port, byte);
    (void)time(&t_start);
    (void)time(&t_end);
    return(byte);
}

/* **********************************************************************
   * Print the string that's offered.                                   *
   *                                                                    *
   * Append a carriage return to the end of a line feed.                *
   *                                                                    *
   ********************************************************************** */

static void print_buffer(char *this_string,
    char com_port,
    BOOL echo_to_video)
{
    char byte;

    while (*this_string) {
	byte = send_byte(*this_string++, com_port);
        if (byte == 0x0a) {
	    (void)send_byte(0x0d, com_port);
            if (echo_to_video) {
                (void)putchar(0x0d);
            }
        }

        if (echo_to_video) {
            (void)putchar(byte);
        }
    }
}

/* **********************************************************************
   * See if there has been too much inactivity. If so, return TRUE.     *
   *                                                                    *
   ********************************************************************** */

static BOOL check_for_time_out(void)
{
    (void)time(&t_end);
    return((t_end - t_start) >= (time_t)Input_Time_Out);
}

/* **********************************************************************
   * Go through the system configuration file and extract everything    *
   * that we are interested in.  Ignore everything else.                *
   *                                                                    *
   ********************************************************************** */

static void extract_configuration(void)
{
    FILE *config;
    char record[101], *point;
    char file_path[81];

    (void)strcpy(file_path, config_file_path);
    (void)strcat(file_path, "BBS-TNC.CFG");

    if ((config = fopen(file_path, "rt")) == (FILE *)NULL)
        return;

    while (! feof(config)) {
        (void)fgets(record, 100, config);
        if (! feof(config)) {
            point = record;
            skipspace(point);
            if (! strnicmp(point, "pass", 4)) {
                point += 4;
                skipspace(point);
                if (! strnicmp(point, "yes", 3)) {
                    password_needed = TRUE;
                }
                else {
                    password_needed = FALSE;
                }
            }
            else if (! strnicmp(point, "key", 3)) {
                point += 3;
                skipspace(point);
		(void)strncpy(system_password, point, strlen(point) - 1);
                ucase(system_password);
            }
            else if (! strnicmp(point, "call", 4)) {
                point += 4;
                skipspace(point);
                if (! strnicmp(point, "yes", 3)) {
                    call_sign = TRUE;
                }
                else {
                    call_sign = FALSE;
                }
            }
            else if (! strnicmp(point, "myalias", 7)) {
                point += 7;
                skipspace(point);
                (void)strncpy(myalias, point, strlen(point) - 1);
                ucase(myalias);
            }
        }
    }

    (void)fclose(config);
}

/* **********************************************************************
   * Get some input.                                                    *
   *                                                                    *
   * This function is a very simple input function which supplies only  *
   * a few features.                                                    *
   *                                                                    *
   * Backspace is checked for and will erase the input data stream if   *
   * any remains.                                                       *
   *                                                                    *
   * It performs bounds checking to see if the input length would       *
   * exceed the length of the buffer.                                   *
   *                                                                    *
   * It employs the inactivity time-out timers.                         *
   *                                                                    *
   ********************************************************************** */

static void input(char *to_this, char com_port, int how_many)
{
    int b_count;
    char byte, report[10];
    time_t t_start, t_end;

    b_count = 0;

    (void)time(&t_start);
    (void)time(&t_end);

/*
    An endless loop will help. A time-out is employed to make sure
    that we exit the program if no keys are typed after a long time
    only if the program is being run on a COM port.
*/

    while (TRUE) {
        (void)time(&t_end);
        if (! get_carrier_status(com_port)) {
            return;
        }

        if ((t_end - t_start) >= (time_t)Input_Time_Out) {
            print_buffer("\n!!! Keyboard Timed out !!!\n", com_port, TRUE);
            (void)fcloseall();
            exit(Inactivity_Time_Out);
        }

	byte = have_byte(com_port);

	if (byte > 0) {
            if (byte == 0x08) {         /* Backspace? */
                if (b_count > 0) {
                    to_this[b_count--] = (char)NULL;
                    (void)sprintf(report, "%c %c", 0x08, 0x08);
                    print_buffer(report, com_port, FALSE);
                }
            }
            else if (byte == 0x0d) {    /* Carriage return? */
                putchar(0x0d);
                putchar(0x0a);
                (void)send_byte(0x0d, com_port);
                (void)send_byte(0x0a, com_port);
                return;
	    }
	    else {
                if ((b_count + 1) == how_many) {
                    send_byte(0x07, com_port);
                }
                else {
                    to_this[b_count++] = byte;
                    to_this[b_count] = (char)NULL;
                    (void)send_byte(byte, com_port); /* Echo to port */
		}
            }
        }
    }
}

/* **********************************************************************
   * Validate the users call sign inasmuch as it's possible to          *
   * validate. The following aspects are searched for:                  *
   *                                                                    *
   *    o There must be only one numerical character only               *
   *    o We allow 1x3 (5 characters),                                  *
   *               2x3 (6 characters),                                  *
   *           and 2x1 (4 characters) call signs only.                  *
   *                                                                    *
   ********************************************************************** */

static BOOL validate_call(char *this_call)
{
    char *search;
    BOOL found_digit;

    found_digit = FALSE;

/*
    Copy the pointer so that we don't destroy the original
*/
    search = this_call;

    while (*search) {
        if (isdigit(*search)) {
            if (found_digit) {
                return(FALSE);
            }

            found_digit = TRUE;
        }

        search++;
    }

    if (strlen(this_call) == 4)
        return(isdigit(this_call[2])); /* 2x1 ie WB3K */

    if (strlen(this_call) == 5)
        return(isdigit(this_call[1])); /* 1x3 ie N5TLL */

    if (strlen(this_call) == 6)
        return(isdigit(this_call[2])); /* 2x3 ie KC9PTT */

    return(FALSE);
}

/* **********************************************************************
   * Get the TNC's attention and then send the offered string to the    *
   * TNC. This is a generic routine.                                    *
   *                                                                    *
   * This routine makes the assumption that Control-C gets the TNCs'    *
   * attention It does not ask the TNC what it really is.               *
   *                                                                    *
   ********************************************************************** */

static void send_tnc_configuration(char *command,
    char *this_string,
    char com_port,
    BOOL get_attention)
{
    char whole_line[101];

/*
    Get the attention. The duration of pause before escape out of
    transparrent mode is configureable yet we make another assumption
    that it's at its default of 1 second. We allow 2 seconds.
*/

    if (get_attention) {
        delay(2);
        (void)sprintf(whole_line, "%c%c%c", 0x03, 0x03, 0x03);
        print_buffer(whole_line, com_port, FALSE);
        delay(1);
    }

/*
    We should be in command mode. Issue a carriage return to flush
    anything that may be accumulated (not likely) and then send
    the string configuration. Then issue a carriage return to get
    it logged into the TNCs' memory.
*/

    (void)sprintf(whole_line, "\r%s %s\r", command, this_string);
    print_buffer(whole_line, com_port, FALSE);
}

/* **********************************************************************
   * Give the opening screen to the BBS port.                           *
   *                                                                    *
   ********************************************************************** */

static void offer_opening_screen(char tnc_port, char bbs_port)
{
    char report[81];

    print_buffer(
        "\n---------------------------------------------------------\n",
        bbs_port, TRUE);

    (void)sprintf(report,
        "   Port %d (BBS) to port %d (TNC) link established\n",
        bbs_port, tnc_port);

    print_buffer(report, bbs_port, FALSE);

    print_buffer(
        "   Hit Control-E (Exit) to exit this program\n",
        bbs_port, TRUE);

    print_buffer(
        "   Hit Control-W (What) for this help screen\n",
        bbs_port, TRUE);

    (void)sprintf(report,
        "   Inactivity time-out set to ---> %d <--- seconds\n",
        Input_Time_Out);

    print_buffer(report, bbs_port, FALSE);

    print_buffer("---------------------------------------------------------\n",
        bbs_port, TRUE);
}

/* **********************************************************************
   * The main entry point.                                              *
   *                                                                    *
   ********************************************************************** */

void main(int argc, char *argv[])
{
    char bbs_port, tnc_port;
    char byte;
    BOOL terminate;
    char report[81];
    struct finfo fossil_information;

/*
    Initialize some data
*/

    password_needed = FALSE;
    call_sign = FALSE;
    system_password[0] = (char)NULL;
    config_file_path[0] = (char)NULL;
    user_call_sign[0] = (char)NULL;
    myalias[0] = (char)NULL;

/*
    Find out where the configuration file should be. We'll get
    a NULL if there is no environment variable configured yet
    this is fine with us!  If there is no backslash at the end
    of the path then we append one.
*/

    (void)strcpy(config_file_path, getenv("BBSTNCCONFIG"));

    if (config_file_path[0] != (char)NULL) {
        if (config_file_path[strlen(config_file_path) - 1] != '\\') {
            (void)strcat(config_file_path, "\\");
        }
    }

/*
    See if we have configuration anywhere
*/

    extract_configuration();

/*
    Get com ports from the command line
*/

    if (argc != 3) {
        (void)printf("I need to know which two ports to gate!\n");
        exit(Error_Bad_Parameter);
    }

    bbs_port = atoi(argv[1]);
    tnc_port = atoi(argv[2]);

    if (bbs_port == tnc_port) {
        (void)printf("The ports may not be the same!\n");
        exit(Error_Bad_Parameter);
    }

/*
    Check to see if we have a FOSSIL driver active. We
    need two serial ports being covered for this to work
    properly.
*/

    if (ComPortInit(tnc_port, 0, &fossil_information) != FSIG) {
        (void)fputs("No FOSSIL driver was found!\n", stderr);
        exit(No_FOSSIL_Driver);
    }
    else {
        if (ComPortInit(bbs_port, 0, &fossil_information) != FSIG) {
            (void)fputs("FOSSIL Driver doesn't have two active ports!\n",
                stderr);

            ComPortDeInit(tnc_port);
            exit(Non_Active_Port);
        }
    }

/*
    Program TNC 8 data, 1 stop, no parity, 9600 baud. We
    leave the BBS port alone!
    Set CTS and DTE
*/

    ComPortSet(tnc_port, CP_B9600 | CP_8N1);
    ComFlowCtl(tnc_port, FC_LOCCTS);

/*
    Make sure that we have carrier om the BBS before going in!
*/

    if (! get_carrier_status(bbs_port)) {
        (void)printf("Carrier not present on port %d", bbs_port);
        ComPortDeInit(tnc_port);
        ComPortDeInit(bbs_port);
        exit(Error_No_Carrier);
    }

/*
    Make sure that we have power. The DSR must be active.
*/

    if (! tnc_power(tnc_port)) {
        print_buffer("\nThe TNC is not powered-up.\n", bbs_port, TRUE);

        print_buffer("Please Yell for the SysOp to turn it on!\n",
            bbs_port, TRUE);

        ComPortDeInit(tnc_port);
        ComPortDeInit(bbs_port);
        exit(No_TNC_Power);
    }

/*
    We have power so it's possible to access the TNC. Check to
    see if there should be a password validation prior to granting
    access to the user.
*/

    if (password_needed) {
        print_buffer("\nA password is required to access the Packet TNC!\n",
            bbs_port, TRUE);

        print_buffer("Please enter the TNC access code: ", bbs_port, TRUE);
        input(report, bbs_port, 20);
        ucase(report);

        if (strcmp(system_password, report)) {
            print_buffer("Password is incorrect!\n", bbs_port, TRUE);

            print_buffer("Please ask the SysOp for the access code!\n",
                bbs_port, TRUE);

            ComPortDeInit(tnc_port);
            ComPortDeInit(bbs_port);
            exit(Incorrect_Password);
        }
    }

    if (call_sign) {
        print_buffer("\nYou must enter your call sign: ", bbs_port, TRUE);
        input(user_call_sign, bbs_port, 10);
        ucase(user_call_sign);

        if (! validate_call(user_call_sign)) {

            print_buffer("That does not appear to be a valid call sign!\n",
                bbs_port, TRUE);

            print_buffer("Please yell for the SysOp for assistance!\n",
                bbs_port, TRUE);

            ComPortDeInit(tnc_port);
            ComPortDeInit(bbs_port);
            exit(Incorrect_Call_Sign);
        }
    }

/*
    Initialize local data and reset the inactivity timers.
*/

    (void)time(&t_start);
    (void)time(&t_end);
    terminate = FALSE;

/*
    Grant some information to the user, making sure they know
    how to exit the system nicely. This is more like an opening
    menu screen more than anything else. Then erase everything
    that's been received from the TNC so far (usuallyt just a
    response to the setting of commands like mycall).
*/

    offer_opening_screen(tnc_port, bbs_port);
    ComRxPurge(tnc_port);

/*
    If a call sign was entered and it validated out ok, send it to
    the TNC, asking for its attention first. Take into consideration
    the possibility that the TNC is in transparrent mode.
*/

    if (call_sign)
        send_tnc_configuration("MYCALL", user_call_sign, tnc_port, TRUE);

/*
    Any other configuration items don't need to get the TNCs'
    attention. Send the myalias if there is one. Any other TNC
    configuration may be sent at this time.
*/

    if (myalias[0] != (char)NULL)
        send_tnc_configuration("MYALIAS", myalias, tnc_port, FALSE);

/*
    All of the preliminary work has been done. Now we'll enter into
    an endless loop...

    Note that if a byte is received from the TNC, we sent it to the
    BBS and then do no other tasks. Because this is polled mode
    operations only, this makes the data received from the TNC a high
    priority poll.

    If no byte is received from the TNC, we then check to see if a
    byte is ready from the BBS.
*/

    while (! terminate) {
        if ((byte = have_byte(tnc_port)) != 0) {
            (void)send_byte(byte, bbs_port);
            (void)putchar(byte);                /* Echo to view screen */
        }
        else {
            if ((byte = have_byte(bbs_port)) != 0) {
                if (byte == 5) {                /* Control-E */
                    terminate = TRUE;
                }
                else if (byte == 23) {          /* Control-W */
                    offer_opening_screen(tnc_port, bbs_port);
                }
                else {
                    (void)send_byte(byte, tnc_port);
                }
            }

/*
    Make sure that the BBS has still got carrier.
*/

            if (! get_carrier_status(bbs_port)) {
                (void)printf("Carrier lost");
                ComPortDeInit(tnc_port);
                ComPortDeInit(bbs_port);
                exit(Carrier_Lost);
            }

/*
    Make sure that we have not times out! We watch for inactivity
    on both of the serial ports.
*/

            if (check_for_time_out()) {
                ComPortDeInit(tnc_port);
                ComPortDeInit(bbs_port);
                exit(Inactivity_Time_Out);
            }

/*
    See if there is a key waiting at the keyboard. If there is, we
    send it to the TNC which may echo it back, causing it to be sent
    to the BBS.
*/

            if (kbhit() != 0) {
                byte = getch();
                send_byte(byte, tnc_port);
            }
        }
    }

/*
    The exit code was entered. Make sure that the FOSSIL drivers
    are deactivated so that no problems occure when we exit.
*/

    ComPortDeInit(tnc_port);
    ComPortDeInit(bbs_port);
    exit(Normal_Termination);
}

