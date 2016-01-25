#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Global option flags
 */
int	Debug = 0;

void
debug(int level, char *fmt, ...)
{
	va_list ap;

	if (Debug < level)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

int
error(int fatal, char *fmt, ...)
{
	va_list ap;

	if (fatal)
	    fprintf(stderr, "Error: ");
	else
	    fprintf(stderr, "Warning: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fatal > 0)
	    exit(fatal);
	else
	    return (fatal);
}

void
usage(void)
{
	fprintf(stderr,
"Usage:\n"
"	arm2hpdl [options] sihp1005.img > sihp1005.dl\n"
"\n"
"	Add HP download header/trailer to an ARM ELF binary.\n"
"	If the file already has an HP header, just copy it to stdout.\n"
"\n"
"Options:\n"
"       -D lvl      Set Debug level [%d]\n"
	, Debug
	);

	exit(1);
}

/*
 * Compute HP-style checksum
 */
long
docheck(long check, unsigned char *buf, int len)
{
    int	i;

    if (len & 1)
	error(1, "We should never see an odd number of bytes in this app.\n");

    for (i = 0; i < len; i += 2)
	check += (buf[i]<<0) | (buf[i+1]<<8);
    return check;
}

int
main(int argc, char *argv[])
{
	extern int	optind;
	extern char	*optarg;
	int		c;
	int		rc;
	unsigned char	buf[BUFSIZ];
	int		len;
	FILE		*fp;
	struct stat	stats;
	int		size;
	unsigned char	elf[4];
	long		check;
	int		iself;
	int		ispjl;

	while ( (c = getopt(argc, argv, "D:?h")) != EOF)
		switch (c)
		{
		case 'D':
			Debug = atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}

	argc -= optind;
	argv += optind;

	if (argc != 1)
	    usage();

	/*
	 * Open the file and figure out if its an ELF file
	 * by reading the first 4 bytes.
	 */
	fp = fopen(argv[0], "r");
	if (!fp)
	    error(1, "Can't open '%s'\n", argv[0]);

	len = fread(elf, 1, sizeof(elf), fp);
	if (len != 4)
	    error(1, "Premature EOF on '%s'\n", argv[0]);

	iself = 0;
	ispjl = 0;
	check = 0;
	if (memcmp(elf, "\177ELF", 4) == 0)
	{
	    /*
	     * Its an ELF executable file
	     */
	    unsigned char	filhdr[17] =
	    {
		0xbe, 0xef, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		0, 0, 0, 0,	/* size goes here */
		0, 0, 0,
	    };
	    unsigned char	sechdr[12] =
	    {
		0xc0, 0xde, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 0, 0
	    };

	    iself = 1;

	    /*
	     * Create and write the file header
	     */
	    rc = stat(argv[0], &stats);
	    if (rc < 0)
		error(1, "Can't stat '%s'\n", argv[0]);

	    size = stats.st_size + 12 + 4;

	    filhdr[10] = size>>24;
	    filhdr[11] = size>>16;
	    filhdr[12] = size>> 8;
	    filhdr[13] = size>> 0;

	    rc = fwrite(filhdr, 1, sizeof(filhdr), stdout);

	    /*
	     * Create and write the section header
	     */
	    //memset(sechdr+2, 0, sizeof(sechdr)-2);

	    check = docheck(check, sechdr, sizeof(sechdr));
	    rc = fwrite(sechdr, 1, sizeof(sechdr), stdout);
	}
	else if (memcmp(elf, "\276\357AB", 4) == 0)
	{
	    /*
	     * This file already has an HP download header.
	     * Don't change it.
	     */
	    if (Debug)
		error(0, "This file already has an HP header.  "
		    "I will just copy it to stdout.\n");
	}
	else if (memcmp(elf, "20", 2) == 0)
	{
	    unsigned char hdr[8];
	    
	    ispjl = 1;
	    printf("\033%%-12345X@PJL ENTER LANGUAGE=ACL\r\n");

	    rc = stat(argv[0], &stats);
	    if (rc < 0)
		error(1, "Can't stat '%s'\n", argv[0]);

	    size = stats.st_size - 8;

	    hdr[0] = 0x00;
	    hdr[1] = 0xac;
	    hdr[2] = 0xc0;
	    hdr[3] = 0xde;
	    hdr[4] = size>>24;
	    hdr[5] = size>>16;
	    hdr[6] = size>> 8;
	    hdr[7] = size>> 0;

	    rc = fwrite(hdr, 1, sizeof(hdr), stdout);
	}
	else
	{
	    error(1, "I don't understand this file at all!\n");
	}

	/*
	 * Write out the 4 bytes we read earlier
	 */
	if (iself)
	    check = docheck(check, elf, sizeof(elf));
	rc = fwrite(elf, 1, sizeof(elf), stdout);

	/*
	 * Write out the remainder of the file
	 */
	while ( (len = fread(buf, 1, sizeof(buf), fp)) )
	{
	    if (iself)
		check = docheck(check, buf, len);
	    rc = fwrite(buf, 1, len, stdout);
	}

	fclose(fp);

	/*
	 * Add the file trailer
	 */
	if (iself)
	{
	    /*
	     * Add in the checksum carries and complement it
	     */
	    while (check >> 16)
		check = (check&0xffff) + (check>>16);
	    check = ~check;

	    putchar(0xff);
	    putchar(0xff);
	    putchar((check >> 0) & 0xff);
	    putchar((check >> 8) & 0xff);
	    debug(1, "checksum = %lx\n", check);
	}
	if (ispjl)
	    printf("\033%%-12345X");

	exit(0);
}
