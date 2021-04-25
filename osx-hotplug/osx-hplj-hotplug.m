/*
AUTHORS
	Anon		<neobisc8@gmail.com>
	Rick Richardson <rick.richardson@comcast.net>
	From: /Developer/Examples/IOKit/usb

LICENSE
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

static char Version[] = "$Id: osx-hplj-hotplug.m,v 1.16 2020/06/10 20:24:09 rick Exp $";

#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <time.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

/*
 * Command line options
 */
int	Debug = 0;

typedef struct
{
    long	vid, pid;
    char	*path;
} HOTPLUG;

HOTPLUG HotPlug[] =
{
    0x03f0,	0x0517,		PREFIX "/share/foo2zjs/firmware/sihp1000.dl",
    0x03f0,	0x1317,		PREFIX "/share/foo2zjs/firmware/sihp1005.dl",
    0x03f0,	0x4117,		PREFIX "/share/foo2zjs/firmware/sihp1018.dl",
    0x03f0,	0x2b17,		PREFIX "/share/foo2zjs/firmware/sihp1020.dl",

    0x03f0,	0x3d17,		PREFIX "/share/foo2xqx/firmware/sihpP1005.dl",
    0x03f0,	0x3e17,		PREFIX "/share/foo2xqx/firmware/sihpP1006.dl",
    0x03f0,	0x4817,		PREFIX "/share/foo2xqx/firmware/sihpP1005.dl",
    0x03f0,	0x4917,		PREFIX "/share/foo2xqx/firmware/sihpP1006.dl",
    0x03f0,	0x3f17,		PREFIX "/share/foo2xqx/firmware/sihpP1505.dl",
    0,		0,		NULL,	// Must be last
};

#define waitTime 8		// time to wait until firmware loads after
				// printer plugged in.

typedef struct MyPrivateData {
    io_object_t			notification;
    IOUSBDeviceInterface	**deviceInterface;
    CFStringRef			deviceName;
    HOTPLUG			*hp;
} MyPrivateData;

static IONotificationPortRef	gNotifyPort;
static io_iterator_t		gAddedIter;
static CFRunLoopRef		gRunLoop;

void
usage(void)
{
    fprintf(stderr,
"Usage:\n"
"	osx-hplj-hotplug [options]\n"
"\n"
"	Daemon which watches for HP LJ 1000, 1005, 1018, 1020, P1005, P1006,\n"
"	P1007, P1008, and P1505 being plugged in.  If so, then the firmware\n"
"	is downloaded to it.\n"
    );

    exit(1);
}

void
debug(int level, char *fmt, ...)
{
    va_list ap;

    if (Debug < level)
        return;

    setvbuf(stderr, (char *) NULL, _IOLBF, BUFSIZ);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void
error(int fatal, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fatal)
        exit(fatal);
}

void
MyCallBackFunction(void *dummy, IOReturn result, void *arg0)
{
    //  UInt8       inPipeRef = (UInt32)dummy;

    debug(1, "MyCallbackfunction: %ld, %d, %ld\n",
	(long)dummy, (int)result, (long)arg0);
    CFRunLoopStop(CFRunLoopGetCurrent());
}

void
transferData(IOUSBInterfaceInterface **intf,
		    UInt8 inPipeRef, UInt8 outPipeRef, MyPrivateData *mpd)
{
    IOReturn                    err;
    CFRunLoopSourceRef          cfSource;
    int                         i;
    char                    outBuf[8096];
    FILE *fp;

    err = (*intf)->CreateInterfaceAsyncEventSource(intf, &cfSource);
    if (err)
    {
        error(0, "transferData: can't create event source, err = %08x\n", err);
        return;
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), cfSource, kCFRunLoopDefaultMode);

    fp = fopen(mpd->hp->path, "r");
    if (fp)
    {
	char buf[256*1024];
	int len;

	while ( (len = fread(buf, 1, sizeof(buf), fp)) != 0)
	{
	     err = (*intf)->WritePipeAsync(intf, outPipeRef, buf, len,
		(IOAsyncCallback1)MyCallBackFunction, (void*)(long)inPipeRef);

	    if (err)
	    {
		printf("transferData: WritePipeAsyncFailed, err = %08x\n", err);
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), cfSource,
		    kCFRunLoopDefaultMode);
		return;
	    }
	}
	fclose(fp);
    }
    debug(1, "transferData: calling CFRunLoopRun\n");
    CFRunLoopRun();
    debug(1, "transferData: returned from  CFRunLoopRun\n");
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), cfSource,
	kCFRunLoopDefaultMode);
}

void
dealWithPipes(IOUSBInterfaceInterface **intf, UInt8 numPipes,
		MyPrivateData  *mpd)
{
    int		i;
    IOReturn    err;
    UInt8       inPipeRef = 0;
    UInt8       outPipeRef = 0;
    UInt8       direction, number, transferType, interval;
    UInt16      maxPacketSize;
   
    // pipes are one based, since zero is the default control pipe
    for (i=1; i <= numPipes; i++)
    {
        err = (*intf)->GetPipeProperties(intf, i,
		&direction, &number, &transferType, &maxPacketSize, &interval);
        if (err)
        {
            error(0, "dealWithPipes: can't get pipe properties for pipe %d,"
		    " err = %08x\n", i, err);
            return;
        }
        if (transferType != kUSBBulk)
        {
            printf("dealWithPipes: skipping pipe %d: it is not a bulk pipe\n",
		    i);
            continue;
        }
        if ((direction == kUSBIn) && !inPipeRef)
        {
            debug(1, "dealWithPipes: grabbing BULK IN pipe index %d, num %d\n",
		    i, number);
            inPipeRef = i;
        }
        if ((direction == kUSBOut) && !outPipeRef)
        {
            debug(1, "dealWithPipes: grabbing BULK OUT pipe index %d, num %d\n",
		    i, number);
            outPipeRef = i;
        }
    }
    if (inPipeRef && outPipeRef)
      transferData(intf, inPipeRef, outPipeRef, mpd);
}

IOReturn
GetDeviceID(IOUSBInterfaceInterface **intf, char *buf, int maxlen)
{
    IOUSBDevRequest req;
    IOReturn err;
    int	length;

    // This class-specific request returns a device ID string that is
    // compatible with IEEE 1284. See IEEE 1284 for syntax and formatting
    // information. A printer with multiple configurations, interfaces, or
    // alternate settings may contain multiple IEEE 1284 device ID strings.
    // The wValue field is used to specify a zero-based configuration index.
    // The high-byte of the wIndex field is used to specify the zero-based
    // interface index. The low-byte of the wIndex field is used to specify
    // the zero-based alternate setting. The device ID string is returned
    // in the following format:
    // IEEE 1284 device ID string (including length in the first two bytes
    // in big endian format).

    req.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    req.bRequest = 0;		// GetDeviceID
    req.wValue = 0;		// see above
    req.wIndex = 0;		// see above
    req.wLength = maxlen;
    req.pData = buf;

    err = (*intf)->ControlRequest(intf, 0, &req);
    if (err)
    {
	buf[0] = 0;
	return(err);
    }

    length = (((unsigned)buf[0] & 255) << 8) + ((unsigned)buf[1] & 255);

    /*
     * Check to see if the length is larger than our buffer; first
     * assume that the vendor incorrectly implemented the 1284 spec,
     * and then limit the length to the size of our buffer...
     */
    if (length > (maxlen - 2))
	length = (((unsigned)buf[1] & 255) << 8) + ((unsigned)buf[0] & 255);

    if (length > (maxlen - 2))
	length = maxlen - 2;

    memmove(buf, buf + 2, length);
    buf[length] = '\0';

    return (err);
}

void
dealWithInterface(io_service_t usbInterfaceRef, MyPrivateData *mpd)
{
    IOReturn                    err;
    IOCFPlugInInterface         **iodev;	// requires <IOKit/IOCFPlugIn.h>
    IOUSBInterfaceInterface	**intf;
    SInt32                      score;
    UInt8                       numPipes;
    char			buf[256];

    err = IOCreatePlugInInterfaceForService(usbInterfaceRef,
	kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID,
	&iodev, &score);
    if (err || !iodev)
    {
	error(0, "dealWithIface: can't create plugin. ret = %08x, iodev = %p\n",
	    err, iodev);
	return;
    }
    err = (*iodev)->QueryInterface(iodev,
	CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID)&intf);
    IODestroyPlugInInterface(iodev);	// done with this

    if (err || !intf)
    {
	error(0, "dealWithIface: can't create an iface. ret=%08x, intf=%p\n",
		err, intf);
	return;
    }

    err = (*intf)->USBInterfaceOpen(intf);
    if (err)
    {
	error(0, "dealWithInterface: can't open interface. ret = %08x\n", err);
	return;
    }

    err = (*intf)->GetNumEndpoints(intf, &numPipes);
    if (err)
    {
	error(0, "dealWithInterface: can't get number of endpoints. rc=%08x\n",
		 err);
	(*intf)->USBInterfaceClose(intf);
	(*intf)->Release(intf);
	return;
    }

    err = GetDeviceID(intf, buf, sizeof(buf));
    if (err)
    {
	error(0, "dealWithInterface: can't get device id. ret = %08x\n",
		 err);
	(*intf)->USBInterfaceClose(intf);
	(*intf)->Release(intf);
	return;
    }

    // If FWVER is present, then no download is needed
    printf("GetDeviceID: %s\n", buf);
    if (strstr(buf, ";FWVER:"))
    {
	(*intf)->USBInterfaceClose(intf);
	(*intf)->Release(intf);
	return;
    }

    debug(1, "dealWithInterface: found %d pipes\n", numPipes);
    if (numPipes == 0)
    {
	// try alternate setting 1
	err = (*intf)->SetAlternateInterface(intf, 1);
	if (err)
	{
	    error(0, "dealWithInterface: can't set alt interface 1. rc=%08x\n",
		 err);
	    (*intf)->USBInterfaceClose(intf);
	    (*intf)->Release(intf);
	    return;
	}
	err = (*intf)->GetNumEndpoints(intf, &numPipes);
	if (err)
	{
	    error(0, "dealWithInterface: can't get number of endpoints - "
		    "alt setting 1. rc = %08x\n", err);
	    (*intf)->USBInterfaceClose(intf);
	    (*intf)->Release(intf);
	    return;
	}
	// workaround. GetNumEndpoints does not work after SetAlternateInterface
	numPipes = 13;
    }

    if (numPipes)
        dealWithPipes(intf, numPipes, mpd);

    err = (*intf)->USBInterfaceClose(intf);
    if (err)
    {
	error(0, "dealWithInterface: can't close interface. rc=%08x\n", err);
	return;
    }
    err = (*intf)->Release(intf);
    if (err)
    {
	error(0, "dealWithInterface: can't release interface. rc=%08x\n", err);
	return;
    }
}


IOReturn
ConfigureDevice(IOUSBDeviceInterface **dev)
{
    UInt8                               numConf;
    IOReturn                            rc;
    IOUSBConfigurationDescriptorPtr     confDesc;
   
    rc = (*dev)->GetNumberOfConfigurations(dev, &numConf);
    if (!numConf)
        return -1;

    // get the configuration descriptor for index 0
    rc = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);
    if (rc)
    {
        error(0, "unable to get config descriptor for index %d (rc=%08x)\n",
	     0, rc);
        return -1;
    }

    rc = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
    if (rc)
    {
        error(0, "unable to set configuration to value %d (err=%08x)\n", 0, rc);
        return -1;
    }
   
    return kIOReturnSuccess;
}

// ============================================================================
// 
// DeviceNotification
// 
// This routine will get called whenever any kIOGeneralInterest notification
// happens.  We are
// interested in the kIOMessageServiceIsTerminated message so that's what we
// look for.  Other
// messages are defined in IOMessage.h.
// 
// ============================================================================
void
DeviceNotification(void *refCon, io_service_t service, natural_t messageType,
		   void *messageArgument)
{
    kern_return_t   rc;
    MyPrivateData  *privateDataRef = (MyPrivateData *) refCon;

    if (messageType == kIOMessageServiceIsTerminated)
    {
	printf("LaserJet unplugged...\n");

	// Dump our private data to stderr just to see what it looks like.
	CFShow(privateDataRef->deviceName);

	// Free the data we're no longer using now that the device is going away
	CFRelease(privateDataRef->deviceName);

	if (privateDataRef->deviceInterface)
	{
	    rc = (*privateDataRef->deviceInterface)->
			    Release(privateDataRef->deviceInterface);
	}

	rc = IOObjectRelease(privateDataRef->notification);

	free(privateDataRef);
    }
}

// ============================================================================
// 
// DeviceAdded
// 
// This routine is the callback for our IOServiceAddMatchingNotification.  When 
// we get called
// we will look at all the devices that were added and we will:
// 
// 1.  Create some private data to relate to each device (in this case we use
// the service's name
// and the location ID of the device
// 2.  Submit an IOServiceAddInterestNotification of type kIOGeneralInterest
// for this device,
// using the refCon field to store a pointer to our private data.  When we get
// called with
// this interest notification, we can grab the refCon and access our private
// data.
// 
// ============================================================================
void
DeviceFound(void *refCon, io_iterator_t iterator)
{
    kern_return_t	kr;
    io_service_t	usbDevice;
    IOCFPlugInInterface **plugInInterface = NULL;
    SInt32		score;
    HRESULT		res;
    char		buf[512];
    HOTPLUG		*hp = (HOTPLUG *) refCon;

    while ((usbDevice = IOIteratorNext(iterator)))
    {
	io_name_t       deviceName;
	CFStringRef     deviceNameAsCFString;
	MyPrivateData  *privateDataRef = NULL;
	UInt32          locationID;
	int             t1 = 0;
	int             t2 = 0;
	int             timer = 0;

	printf("LaserJet %s Found!\n", hp->path);
	sleep(waitTime);

	// Add some app-specific information about this device.
	// Create a buffer to hold the data.
	privateDataRef = malloc(sizeof(MyPrivateData));
	bzero(privateDataRef, sizeof(MyPrivateData));

	// Save the HOTPLUG entry
	privateDataRef->hp = hp;

	// Get the USB device's name.
	kr = IORegistryEntryGetName(usbDevice, deviceName);
	if (KERN_SUCCESS != kr)
	    deviceName[0] = '\0';

	deviceNameAsCFString =
	    CFStringCreateWithCString(kCFAllocatorDefault, deviceName,
				      kCFStringEncodingASCII);

	// Dump our data to stderr just to see what it looks like.
	// fprintf(stderr, "deviceName: ");
	CFShow(deviceNameAsCFString);

	// Save the device's name to our private data.  
	privateDataRef->deviceName = deviceNameAsCFString;

	// Now, get the locationID of this device. In order to do this, we need 
	// to create an IOUSBDeviceInterface 
	// for our device. This will create the necessary connections between
	// our userland application and the 
	// kernel object for the USB Device.
	kr = IOCreatePlugInInterfaceForService(usbDevice,
					       kIOUSBDeviceUserClientTypeID,
					       kIOCFPlugInInterfaceID,
					       &plugInInterface, &score);

	if ((kIOReturnSuccess != kr) || !plugInInterface)
	{
	    // fprintf(stderr, "IOCreatePlugInInterfaceForService returned
	    // 0x%08x.\n", kr);
	    continue;
	}

	// Use the plugin interface to retrieve the device interface.
	res = (*plugInInterface)->QueryInterface(
		    plugInInterface,
		    CFUUIDGetUUIDBytes
		    (kIOUSBDeviceInterfaceID),
		    (LPVOID *) & privateDataRef->deviceInterface);

	// Now done with the plugin interface.
	(*plugInInterface)->Release(plugInInterface);

	if (res || privateDataRef->deviceInterface == NULL)
	{
	    // fprintf(stderr, "QueryInterface returned %d.\n", (int) res);
	    continue;
	}

	{
	    IOUSBFindInterfaceRequest           interfaceRequest;
	    IOReturn                                            err;
	    io_iterator_t                                       iterator;
	    io_service_t                                        usbInterfaceRef;
	    IOUSBDeviceInterface        **dev = NULL;
	    dev = privateDataRef->deviceInterface;

	    interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	    interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	    err = (*dev)->CreateInterfaceIterator(dev, &interfaceRequest,
			    &iterator);
	    if (err)
	    {
		error(0, "dealWithDevice: can't create interface iterator\n");
		(*dev)->USBDeviceClose(dev);
		(*dev)->Release(dev);
		return;
	    }
	   
	    while ( (usbInterfaceRef = IOIteratorNext(iterator)) )
	    {
		debug(1, "found interface: %ld\n", (long)usbInterfaceRef);
		dealWithInterface(usbInterfaceRef, privateDataRef);
		IOObjectRelease(usbInterfaceRef);
	    }
	   
	    IOObjectRelease(iterator);
	    iterator = 0;
	}

	// Register for an interest notification of this device being removed.
	// Use a reference to our
	// private data as the refCon which will be passed to the notification
	// callback.
	kr = IOServiceAddInterestNotification(
		    gNotifyPort,			// notifyPort
		    usbDevice,				// service
		    kIOGeneralInterest,			// interestType
		    DeviceNotification,			// callback
		    privateDataRef,			// refCon
		    &(privateDataRef->notification)	// notification
	    );

	if (KERN_SUCCESS != kr)
	{
	    // printf("IOServiceAddInterestNotification returned 0x%08x.\n",
	    // kr);
	}

	// Done with this USB device; release the reference added by
	// IOIteratorNext
	kr = IOObjectRelease(usbDevice);
    }
}

// ============================================================================
// main
// ============================================================================

int
main(int argc, char *argv[])
{
    CFMutableDictionaryRef	matchingDict;
    CFRunLoopSourceRef		runLoopSource;
    CFNumberRef			numberRef;
    kern_return_t		kr;
    HOTPLUG			*hp;
    int				c;

    while ( (c = getopt(argc, argv, "D:V:?h")) != EOF)
	switch (c)
	{
        case 'D':       Debug = atoi(optarg); break;
        case 'V':       printf("%s\n", Version); exit(0);
        default:        usage(); exit(1);
	}

    argc -= optind;
    argv += optind;

    for (hp = &HotPlug[0]; hp->path != NULL; ++hp)
    {
	// Interested in instances of class
	matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
     
	// IOUSBDevice and its subclasses
	if (matchingDict == NULL)
	{
	    // fprintf(stderr, "IOServiceMatching returned NULL.\n");
	    return -1;
	}

	// We are interested in all USB devices (as opposed to USB interfaces).
	// The Common Class Specification
	// tells us that we need to specify the idVendor, idProduct, and
	// bcdDevice fields, or, if we're not interested
	// in particular bcdDevices, just the idVendor and idProduct.
	// Note that if we were trying to match an 
	// IOUSBInterface, we would need to set more values in the matching
	// dictionary (e.g. idVendor, idProduct, bInterfaceNumber,
	// and bConfigurationValue.

	// Create a CFNumber for the VID and set the value in the dictionary
	numberRef =
	    CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &hp->vid);
	CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID), numberRef);
	CFRelease(numberRef);

	// Create a CFNumber for the PID and set the value in the dictionary
	numberRef =
	    CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &hp->pid);
	CFDictionarySetValue(matchingDict, CFSTR(kUSBProductID), numberRef);
	CFRelease(numberRef);
	numberRef = NULL;

	// Create a notification port and add its run loop event source to our
	// run loop.  This is how async notifications get set up.
	gNotifyPort = IONotificationPortCreate(kIOMasterPortDefault);
	runLoopSource = IONotificationPortGetRunLoopSource(gNotifyPort);

	gRunLoop = CFRunLoopGetCurrent();
	CFRunLoopAddSource(gRunLoop, runLoopSource, kCFRunLoopDefaultMode);

	// Now set up a notification to be called when a device is first
	// matched by I/O Kit.
	kr = IOServiceAddMatchingNotification(
		gNotifyPort,			// notifyPort
		kIOFirstMatchNotification,	// notificationType
		matchingDict,			// matching
		DeviceFound,			// callback
		hp,				// refCon
		&gAddedIter			// notification
	    );

	// Iterate once to get already-present devices and arm the notification 
	DeviceFound(hp, gAddedIter);
    }

    // Start the run loop. Now we'll receive notifications.
    printf("Waiting for LaserJet Printer Firmware DL...\n\n");
    CFRunLoopRun();

    // We should never get here
    error(0, "Unexpectedly back from CFRunLoopRun()!\n");
    return 0;
}
