/*
 *   Konica-Minolta command filter for the Common UNIX Printing System.
 *
 *   Copyright 2010 by Reinhold Kainhofer <reinhold@kainhofer.com>
 *   Based in part on commandtoepson:
 *         Copyright 1993-2000 by Easy Software Products.
 *   Based in part on commandtops:
 *         Copyright 2008 by Apple Inc.
 *   Based in part on snmp-supplies.c:
 *         Copyright 2008-2009 by Apple Inc.
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   main() - Main entry and command processing.
 */

/*
 * Include necessary headers...
 */

#include <cups/sidechannel.h>
#include <cups/cups.h>
#include <cups/ppd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


inline int
max(int a, int b)
{
    return a > b ? a : b;
}


/*
 * Macros...
 */

#define pwrite(s,n) fwrite((s), 1, (n), stdout)

void            report_levels(int negate);
void            auto_configure(void);

/*
 * 'main()' - Main entry and processing of driver.
 */

int				/* O - Exit status */
main(int argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
    FILE        *fp;		/* Command file */
    char        line[1024],	/* Line from file */
                *lineptr;	/* Pointer into line */
    ppd_file_t	*ppd;
    ppd_attr_t	*attr;
    int		negate = 1;

    /*
     * Check for valid arguments...
     */
    if (argc < 6 || argc > 7)
    {
	/*
	 * We don't have the correct number of arguments; write an error message
	 * and return.
	 */
	fprintf(stderr, "ERROR: %s job-id user title copies options [file]\n",
		argv[0]);
	return (1);
    }

    /*
     * Get the negate parm from the PPD file
     */
    ppd = ppdOpenFile(getenv("PPD"));
    if (ppd)
    {
	attr = ppdFindAttr(ppd, "foo2zjsNegateMarkerLevels", NULL);
	if (attr && strcmp(attr->value, "False") == 0)
	    negate = 0;
	ppdClose(ppd);
    }
    fprintf(stderr, "DEBUG: foo2zjsNegateMarkerLevels=%d\n", negate);

    /*
     * Open the command file as needed...
     */
    if (argc == 7)
    {
	if ((fp = fopen(argv[6], "r")) == NULL)
	{
	    perror("ERROR: Unable to open command file - ");
	    return (1);
	}
    }
    else
	fp = stdin;

    /*
     * Read the commands from the file and send the appropriate commands...
     */
    while (fgets(line, sizeof(line), fp) != NULL)
    {
	// Drop trailing newline...
	lineptr = line + strlen(line) - 1;
	if (*lineptr == '\n')
	    *lineptr = '\0';

	// Skip leading whitespace...
	for (lineptr = line; isspace(*lineptr); lineptr++);

	// Skip comments and blank lines...
	if (*lineptr == '#' || !*lineptr)
	    continue;

	// Parse the command...
	if (strncasecmp(lineptr, "AutoConfigure", 13) == 0)
	{
	    // Retrieve the settings from the printer and change the PPD
	    // according
	    // to the installed options
	    // TODO: This is not fully implemented!
	    // auto_configure ();
	}
	else if (strncasecmp(lineptr, "ReportStatus", 12) == 0)
	{
	    // Report Status...
	    // pwrite("\033%-12345X@PJL INFO STATUS\015\012", 27);
	    // pwrite("\033%-12345X", 9);

	    // TODO: Read back-channel data
	    // TODO: Parse back-channel data
	    // TODO: Feed parsed data to the scheduller

	}
	else if (strncasecmp(lineptr, "ReportLevels", 12) == 0)
	{
	    // Report ink levels...
	    report_levels(negate);
	}
	else
	    fprintf(stderr, "ERROR: Invalid printer command \"%s\"!\n",
		    lineptr);
    }

    /*
     * Close the command file and return...
     */
    if (fp != stdin)
	fclose(fp);

    return (0);
}



/****************************************************************************
 *                         Dealing with supplies                            *
 ****************************************************************************/


#define CUPS_MAX_SUPPLIES	32	/* Maximum number of supplies for a
					 * printer */
#define CUPS_MAX_TEXTLEN	155	/* Maximum length of supply names */

typedef struct Supply			/**** Printer supply data ****/
{
    char	id[CUPS_MAX_TEXTLEN],	/* ID used in the response */
		name[CUPS_MAX_TEXTLEN],	/* Name of supply */
		color[8],		/* Color: "#RRGGBB" or "none" */
		type[CUPS_MAX_TEXTLEN];	/* Type of supply, e.g. toner */
    int		capacity,		/* Maximum capacity */
		level;			/* Current level value */
} Supply;

static const char *const default_supplies[10][4] =
{
    { "B",       "Blue",           "#0000FF", "toner" },
    { "C",       "Cyan",           "#00FFFF", "toner" },
    { "G",       "Green",          "#00FF00", "toner" },
    { "K",       "Black",          "#000000", "toner" },
    { "M",       "Magenta",        "#FF00FF", "toner" },
    { "R",       "Red",            "#FF0000", "toner" },
    { "W",       "White",          "#FFFFFF", "toner" },
    { "Y",       "Yellow",         "#FFFF00", "toner" },
    { "TRSBELT", "Transfer unit",  "#808080", "transferUnit" },
    { "FUSER",   "Fuser",          "#808080", "fuser" },
};


/*
 * Find the supply information with given ID in the list of supplies. If not
 * found, add a new entry with defaults as specified in default_supplies 
 */
int
locate_supply_information(Supply * supplies, int num_supplies, int max_supplies,
			  const char *id)
{
    // Check whether we find it in the current list:
    int             pos = 0;
    for (pos = 0; pos < num_supplies; ++pos)
    {
	if (!strcmp(supplies[pos].id, id))
	{
	    return pos;
	}
    }
    // Not found, create new entry:
    if (num_supplies >= max_supplies)
    {
	// No space left in supplies!
	return -1;
    }
    pos = num_supplies;
    strcpy(supplies[pos].id, id);
    int             deflen =
	(int) (sizeof(default_supplies) / sizeof(default_supplies[0]));
    int             k;
    for (k = 0; k < deflen; k++)
    {
	if (!strcmp(default_supplies[k][0], id))
	{			// Found the defaults entry!
	    // Initialize to defaults from default_supplies:
	    strcpy(supplies[pos].name, default_supplies[k][1]);
	    strcpy(supplies[pos].color, default_supplies[k][2]);
	    strcpy(supplies[pos].type, default_supplies[k][3]);
	    supplies[pos].capacity = 0;
	    supplies[pos].level = 0;
	    break;
	}
    }
    return pos;
};


void
report_levels(int negate)
{
    // Buffer for the data
    char            buffer[8192];
    ssize_t         bytes;

    // Check whether we can get a response from the printer at all:
    int             datalen = 1;
    if (cupsSideChannelDoRequest(CUPS_SC_CMD_GET_BIDI, buffer, &datalen,
				 30.0) != CUPS_SC_STATUS_OK ||
	buffer[0] != CUPS_SC_BIDI_SUPPORTED)
    {
	fputs("DEBUG: Unable to retrieve supply status from printer - no "
	      "bidirectional I/O available!\n", stderr);
	return;
    }

    // The actual PJL request
    pwrite("\033%-12345X@PJL INFO DSTATUS\015\012", 28);
    pwrite("\033%-12345X", 9);
    fflush(stdout);

    // RER: 07/20/10 - Sleep for a bit!
    sleep(5);

    // Ask the backend to send all data NOW:
    datalen = 0;
    cupsSideChannelDoRequest(CUPS_SC_CMD_DRAIN_OUTPUT, buffer, &datalen, 5.0);

    // Read back the data from the printer
    bytes = cupsBackChannelRead(buffer, sizeof(buffer) - 1, 5.0);
    buffer[bytes] = '\0';

    if (strncmp(buffer, "@PJL INFO DSTATUS", 17))
    {
	fprintf(stderr,
	    "DEBUG: Printer does not return a proper PJL DSTATUS response.\n");
	fprintf(stderr, "DEBUG: Got %d bytes: %s\n", (int) bytes, buffer);
	return;
    }

    // fprintf (stderr, "DEBUG: Got %d bytes: %s\n", bytes, buffer);

    int             num_supplies = 0;	/* Number of supplies found */
    Supply          supplies[CUPS_MAX_SUPPLIES];	/* Supply information */

    // Parse the returned data
    // 
    // FORMAT is (with K,C,M,Y as color abbreviations):
    // 
    // @PJL INFO DSTATUS
    // CODE=600100
    // CONSUMETONERK=16
    // [...]
    // CONSUMETRSBELT=2
    // CONSUMEFUSER=0
    // CONSUMETONERTYPEK=1000
    // [...]
    // CONSUMETONERINSTALLY=YES
    // \0x0c
    const char     *pos = buffer;
    char            supply[255];
    int             sindex = 0;
    while ((pos = strstr(pos, "CONSUME")))
    {
	sindex = -1;
	pos += 7;
	if (!strncmp(pos, "TONERTYPE", 9))
	{
	    pos += 9;
	    supply[0] = pos[0];
	    supply[1] = '\0';
	    pos += 2;
	    sindex =
		locate_supply_information(supplies, num_supplies,
					  CUPS_MAX_SUPPLIES, supply);
	    if (sindex >= 0)
	    {
		num_supplies = max(sindex + 1, num_supplies);
		supplies[sindex].capacity = atoi(pos);
	    }

	}
	else if (!strncmp(pos, "IMGDRUM", 7))
	{
	    pos += 7;
	    // Don't do anything, this is just dummy information!

	}
	else if (!strncmp(pos, "TONERCOUNTERFEIT", 16))
	{
	    pos += 16;
	    // Don't do anything, this is just dummy information!

	}
	else if (!strncmp(pos, "TONERINSTALL", 12))
	{
	    pos += 12;
	    // Don't do anything, this is just dummy information!

	}
	else if (!strncmp(pos, "TONER", 5))
	{
	    pos += 5;
	    supply[0] = pos[0];
	    supply[1] = '\0';
	    pos += 2;
	    sindex =
		locate_supply_information(supplies, num_supplies,
					  CUPS_MAX_SUPPLIES, supply);
	    // fprintf (stderr, "DEBUG: sindex %d\n", sindex);
	    if (sindex >= 0)
	    {
		num_supplies = max(sindex + 1, num_supplies);
		supplies[sindex].level = negate ? 100 - atoi(pos) : atoi(pos);
	    }

	}
	else if (!strncmp(pos, "FUSER", 5))
	{
	    pos += 6;
	    sindex =
		locate_supply_information(supplies, num_supplies,
					  CUPS_MAX_SUPPLIES, "FUSER");
	    if (sindex >= 0)
	    {
		num_supplies = max(sindex + 1, num_supplies);
		supplies[sindex].level = negate ? 100 - atoi(pos) : atoi(pos);
	    }

	}
	else if (!strncmp(pos, "TRSBELT", 7))
	{
	    pos += 8;
	    sindex =
		locate_supply_information(supplies, num_supplies,
					  CUPS_MAX_SUPPLIES, "TRSBELT");
	    if (sindex >= 0)
	    {
		num_supplies = max(sindex + 1, num_supplies);
		supplies[sindex].level = negate ? 100 - atoi(pos) : atoi(pos);
	    }

	}
	else
	{
	    fprintf(stderr, "DEBUG: Supply return entry did not match any "
		    "known keyword: %s\n", pos);
	}
    }

    // Create the output:
    if (num_supplies)
    {
	int             k;

	// Marker types:
	strcpy(buffer, supplies[0].type);
	for (k = 1; k < num_supplies; ++k)
	    sprintf(buffer+strlen(buffer), ",%s", supplies[k].type);
	fprintf(stderr, "ATTR: marker-types=%s\n", buffer);

	// Marker names
	buffer[0] = '\0';
	for (k = 0; k < num_supplies; ++k)
	{
	    if (k > 0)
		strcat(buffer, ",");
	    if (supplies[k].capacity > 0)
		sprintf(buffer+strlen(buffer), "\"%s (Max %d)\"",
			supplies[k].name, supplies[k].capacity);
	    else
		sprintf(buffer+strlen(buffer), "\"%s\"", supplies[k].name);
	}
	fprintf(stderr, "ATTR: marker-names=%s\n", buffer);

	// Marker colors
	strcpy(buffer, supplies[0].color);
	for (k = 1; k < num_supplies; ++k)
	    sprintf(buffer+strlen(buffer), ",%s", supplies[k].color);
	fprintf(stderr, "ATTR: marker-colors=%s\n", buffer);

	// Marker levels
	sprintf(buffer, "%d", supplies[0].level);
	for (k = 1; k < num_supplies; ++k)
	    sprintf(buffer+strlen(buffer), ",%d", supplies[k].level);
	fprintf(stderr, "ATTR: marker-levels=%s\n", buffer);

    }
    else
	fprintf(stderr,
		"DEBUG: Unable to extract supply information from the "
		"printer's response.\n");

    // fprintf (stderr, "STATE: \n");
}




/****************************************************************************
 *                  Auto-configuration of printer settings                  *
 ****************************************************************************/


void
auto_configure()
{
    // Buffer for the data
    char            buffer[8192];
    ssize_t         bytes;
    int             datalen = 1;

    // Check whether we can get a response from the printer at all:
    if (cupsSideChannelDoRequest(CUPS_SC_CMD_GET_BIDI, buffer, &datalen,
				 30.0) != CUPS_SC_STATUS_OK ||
	buffer[0] != CUPS_SC_BIDI_SUPPORTED)
    {
	fputs("DEBUG: Unable to auto-configure printer - no "
	      "bidirectional I/O available!\n", stderr);
	return;
    }

    // The actual PJL request
    pwrite("\033%-12345X@PJL INFO CONFIG\015\012", 28);
    pwrite("\033%-12345X", 9);
    fflush(stdout);

    // Ask the backend to send all data NOW:
    datalen = 0;
    cupsSideChannelDoRequest(CUPS_SC_CMD_DRAIN_OUTPUT, buffer, &datalen, 5.0);

    // Read back the data from the printer
    bytes = cupsBackChannelRead(buffer, sizeof(buffer) - 1, 5.0);
    buffer[bytes] = '\0';

    if (strncmp(buffer, "@PJL INFO CONFIG", 17))
    {
	fprintf(stderr,
	    "DEBUG: Printer does not return a proper PJL CONFIG response.\n");
	fprintf(stderr, "DEBUG: Got %d bytes: %s\n", (int) bytes, buffer);
	return;
    }

    // Parse the returned data
    // 
    // FORMAT is:
    //
    // @PJL INFO CONFIG
    // IN TRAYS [1 ENUMERATED]
    //         INTRAY1 MP
    // LANGUAGES [1 ENUMERATED]
    //         LAVAFLOW
    // USTATUS [6 ENUMERATED]
    //         DEVICE
    //         JOB
    //         PAGE
    //         TIMED
    //         DDEVICE
    //         DTIMED
    // TRAY2=NOTINSTALLED [2 ENUMERATED]
    //         INSTALLED
    //         NOTINSTALLED
    // TRAY3=NOTINSTALLED [2 ENUMERATED]
    //         INSTALLED
    //         NOTINSTALLED
    // DUPLEX=INSTALLED [2 ENUMERATED]
    //         INSTALLED
    //         NOTINSTALLED
    // TONER=TONEROK [3 ENUMERATED]
    //         TONEROK
    //         TONERDEAD
    //         TONERNOTGENUINE
    // PRINTINGUNIT=PRINTINGUNITOK [2 ENUMERATED]
    //         PRINTINGUNITOK
    //         PRINTINGUNITNOTGENUINE
    // MEMORY=134217728
    // \033

    // TODO
}
