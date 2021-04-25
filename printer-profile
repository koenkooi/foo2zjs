#!/bin/bash

PROGNAME="$0"

usage() {
	cat <<EOF
NAME
    `basename $PROGNAME` - printer-profile using X-Rite ColorMunki and Argyll CMS

SYNOPSIS
    `basename $PROGNAME` [options] manuf model [rgb|cmyk] [patches] [ink-limit]

DESCRIPTION
    Prints a test chart, uses the ColorMunki instrument to scan it in, then
    computes an ICM profile using the Argyll Color Management System.

    Manuf is "sam".  Model is "clp-300", "clp-315", "clp-325" or "clp-365".
    Manuf is "hp".  Model is "2600" or "cp1215" or "cp1025".
    Manuf is "km".  Model is "1600" or "2300" or "2530".
    Manuf is "dell". Model is "1355" or "1765"

    "rgb" is the usual setting.  "patches" is a multiple 196 per page.

    Edit the script for additional models.

OPTIONS
    -b 1|2        Bits per pixel ($BPP)
    -g gs-bin     Set ghostscript to "gs-bin", (gs)
    -r XRESxYRES  Resolution. Default=''. ($RES)
    -P rem-print  Remote print (64-bit) machine, or none ($REMPRINT)
    -S rem-scan   Remote scan (ColorMunki) machine, or none ($REMSCAN)
    -D lvl        Debug level

EXAMPLE
    $ printer-profile sam 315 rgb 196

BUGS
    gs 8.64 and before has problems with 32-bit machines and color profile
    data.  Don't use!

    You need Argyll_V1.6.3 or later in $ARGYLL_BIN.

SEE ALSO
    http://www.xritephoto.com/html/colormunkisplash.htm
	from Amazon, \$390 shipped
    http://www.argyllcms.com/
	free!
    http://www.argyllcms.com/Argyll_V${ARGYLL_VER}_src.zip
EOF

	exit 1
}

#
#       Report an error and exit
#
error() {
	echo "`basename $PROGNAME`: $1" >&2
	exit 1
}

debug() {
	if [ $DEBUG -ge $1 ]; then
	    echo "`basename $PROGNAME`: $2" >&2
	fi
}

#
#	Execute a command as root
#
root() {
    if [ -x /usr/bin/root ]; then
        /usr/bin/root $@
    else
        su -c "$*"
    fi
}

#
#	trap on error
#
trap "exit 1" ERR

#
#       Process the options
#
ARGYLL_VER=1.3.2
ARGYLL_VER=1.3.3
ARGYLL_VER=1.3.4
ARGYLL_VER=1.3.5
ARGYLL_VER=1.5.0
ARGYLL_VER=1.5.1
ARGYLL_VER=1.6.3
ARGYLL_VER=2.1.1
ARGYLL_VER=2.1.2	# 01-18-2020
ARGYLL_ROOT=$HOME/src/Argyll_V${ARGYLL_VER}
ARGYLL_REF=$ARGYLL_ROOT/ref
ARGYLL_BIN=$ARGYLL_ROOT/bin
export PATH=$ARGYLL_BIN:$PATH

REMPRINT=amd
REMPRINT=none
REMSCAN=mac
REMSCAN=none
RGB=rgb
PATCHES=196	#Per page!
BPP=1
RES=
DEBUG=0
while getopts "b:g:r:P:S:D:h?" opt
do
	case $opt in
	b)	BPP="$OPTARG";;
	g)	export GSBIN="$OPTARG";;
	r)	RES="$OPTARG";;
	P)	REMPRINT="$OPTARG";;
	S)	REMSCAN="$OPTARG";;
	D)	DEBUG="$OPTARG";;
	h|\?)	usage;;
	esac
done
shift `expr $OPTIND - 1`

if [ ! -x $ARGYLL_BIN/printtarg ]; then
    error "No Argyll bin in $ARGYLL_BIN!"
fi

#
#	Reference ICM for colprof
#
reficm=$ARGYLL_REF/sRGB.icm
if [ ! -r $reficm ]; then
    error "No ref. icm in '$reficm'"
fi

#
#	Main Program
#
if [ $# -lt 2 ]; then
    usage
fi

MANUF="$1"
MODEL="$2"
if [ $# -ge 3 ]; then
    RGB="$3"
fi
if [ $# -ge 4 ]; then
    if [ "$4" -lt 10 ]; then
	PATCHES=`expr $4 \* 196`
    else
	PATCHES="$4"
    fi
fi

if [ $# -ge 5 ]; then
    INK="$5"
else
    INK=250
fi

BPP_b="-b$BPP"
RES_r=
if [ "$RES" != "" ]; then
    RES_r="-r$RES"
fi

case "$MANUF" in
km)
    MANUF=km
    case "$MODEL" in
    *1600*)
	FOO=foo2lava
	WRAPPER="foo2lava-wrapper $RES_r -z2 -c -C10 -Gnone.icm"
	OUT="root cp xxx.prn /dev/usb/lp1"
	;;
    *2530*)
	FOO=foo2lava
	WRAPPER="foo2lava-wrapper $RES_r -z0 -c -C10 -Gnone.icm"
	OUT="nc 192.168.1.13 9100 < xxx.prn"
	;;
    *2300*)
	FOO=foo2zjs
	WRAPPER="foo2zjs-wrapper $RES_r -c -C10 -Gnone.icm"
	OUT="nc 192.168.1.10 9100 < xxx.prn"
	;;
    *)
	usage
	;;
    esac
    ;;
sam*)
    MANUF=sam
    FOO=foo2qpdl
    case "$MODEL" in
    *300*)
	WRAPPER="foo2qpdl-wrapper $RES_r -z0 -c -C10 -Gnone.icm"
	OUT="nc 192.168.1.11 9100 < xxx.prn"
	;;
    *315*|*325*)
	WRAPPER="foo2qpdl-wrapper $RES_r -z2 -c -C10 -Gnone.icm"
	OUT="root cp xxx.prn /dev/usb/lp1"
	;;
    *365*)
	WRAPPER="foo2qpdl-wrapper $RES_r -z3 -c -C10 -Gnone.icm"
	OUT="root cp xxx.prn /dev/usb/lp1"
	;;
    *)
	usage
	;;
    esac
    ;;
hp*)
    MANUF=hp
    case "$MODEL" in
    *2600*)
	FOO=foo2hp
	WRAPPER="foo2hp2600-wrapper $RES_r $BPP_b -z0 -c -C10 -Gnone.icm"
	OUT="nc 192.168.1.12 9100 < xxx.prn"
	;;
    *cp1215*)
	FOO=foo2hp
	WRAPPER="foo2hp2600-wrapper $RES_r $BPP_b -z1 -c -C10 -Gnone.icm"
	OUT="root cp xxx.prn /dev/usb/lp2"
	;;
    *cp1025*)
	FOO=foo2zjs
	WRAPPER="foo2zjs-wrapper $RES_r -z3 -c -C10 -Gnone.icm"
	OUT="root cp xxx.prn /dev/usb/lp2"
	OUT="nc 192.168.1.16 9100 < xxx.prn"
	;;
    *)
	usage
	;;
    esac
    ;;
dell*)
    MANUF=dell
    case "$MODEL" in
    *1355*|*1765*)
        FOO=foo2hbpl2
        WRAPPER="foo2hbpl2-wrapper $RES_r -c -C10 -Gnone.icm"
        OUT="nc 192.168.178.41 9100 < xxx.prn"
        ;;
    *)
        usage
        ;;
    esac
    ;;
*)
    usage
    ;;
esac

mrp="$MANUF-$MODEL-$RGB-$PATCHES"
if [ "$INK" != "" ]; then
    mrp="$mrp-ink$INK"
fi
if [ "$BPP" != "" ]; then
    mrp="$mrp-bpp$BPP"
fi
if [ "$RES" != "" ]; then
    mrp="$mrp-$RES"
fi
echo "$mrp"

targen_opts=
case "$RGB" in
rgb|RGB)	targen_opts="$targen_opts -d2";;
cmyk|CMYK)	targen_opts="$targen_opts -d4";;
*)		error "Parm2: Must be rgb or cmyk";;
esac

targen_opts="$targen_opts -f$PATCHES"

if [ "$INK" != "" ]; then
    targen_opts="$targen_opts -l$INK"
fi

echo
echo "******************************** targen ********************************"
echo "targen $targen_opts $mrp"
if ! targen $targen_opts $mrp >$mrp.err1; then
    cat $mrp.err1
    exit 1
fi

echo
echo "******************************* printtarg ******************************"
# -h	Use double density for CM
# -v	Verbose mode
# -iCM	Select target instrument, CM = ColorMunki
# -pLetter
# -R0	Use given random start number
echo "$ARGYLL_BIN/printtarg -h -v -iCM -p Letter -R0 $mrp"
$ARGYLL_BIN/printtarg -h -v -iCM -p Letter -R0 $mrp

evince $mrp.ps &

echo
echo "******************************* print it *******************************"
echo "$WRAPPER"
case "$REMPRINT" in
''|none)
    $WRAPPER <$mrp.ps >$mrp.prn
    ;;
*)
    ssh $REMPRINT "$WRAPPER" <$mrp.ps >$mrp.prn
    ;;
esac
ls -l $mrp.prn
echo -n "Print it? [y|n]? "
read yes
case "$yes" in
y|Y)
    cp $mrp.prn xxx.prn
    echo "	$OUT"
    eval $OUT
    rm -f xxx.prn
esac

echo
echo "******************************* chartread ******************************"
case "$REMSCAN" in
''|none)
    echo "$ARGYLL_BIN/chartread $mrp"
    $ARGYLL_BIN/chartread $mrp
    ;;
*)
    echo "scp $mrp.ti2 $REMSCAN:"
    scp $mrp.ti2 $REMSCAN:
    echo "$ARGYLL_BIN/chartread $mrp"
    echo  -n "Scanned on remote system $REMSCAN? [y/n]? "
    read yes
    case "$yes" in
    y|Y)
	scp $REMSCAN:$mrp.ti3 .
	;;
    esac
    ;;
esac

echo
echo "******************************** colprof *******************************"
case $RGB in
rgb)
    echo "colprof -v -D\"$mrp\" -S $reficm -qm -cmt -dpp $mrp"
    colprof -v -D"$mrp" -S $reficm -qm -cmt -dpp $mrp
    ;;
cymk)
    echo "colprof -v -D\"$mrp\" -S $reficm -qm  -cmt -dpp -kr $mrp"
    colprof -v -D"$mrp" -S $reficm -qm  -cmt -dpp -kr $mrp
    ;;
esac

mv $mrp.icc $mrp.icm
root cp $mrp.icm /usr/share/$FOO/icm/testing.icm
echo "/usr/share/$FOO/icm/testing.icm created!"
ls -l /usr/share/$FOO/icm/
