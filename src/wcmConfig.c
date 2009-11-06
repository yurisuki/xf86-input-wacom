/*
 * Copyright 1995-2002 by Frederic Lepied, France. <Lepied@XFree86.org>
 * Copyright 2002-2009 by Ping Cheng, Wacom. <pingc@wacom.com>
 *                                                                            
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xf86Wacom.h"
#include "wcmFilter.h"
#include <sys/stat.h>
#include <fcntl.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

extern Bool xf86WcmIsWacomDevice (char* fname, struct input_id* id);

static int xf86WcmAllocate(LocalDevicePtr local, char* name, int flag);

/*****************************************************************************
 * xf86WcmAllocate --
 ****************************************************************************/

static int xf86WcmAllocate(LocalDevicePtr local, char* type_name, int flag)
{
	WacomDevicePtr   priv   = NULL;
	WacomCommonPtr   common = NULL;
	WacomToolPtr     tool   = NULL;
	WacomToolAreaPtr area   = NULL;
	int i, j;

	priv = xcalloc(1, sizeof(WacomDeviceRec));
	if (!priv)
		goto error;

	common = xcalloc(1, sizeof(WacomCommonRec));
	if (!common)
		goto error;

	tool = xcalloc(1, sizeof(WacomTool));
	if(!tool)
		goto error;

	area = xcalloc(1, sizeof(WacomToolArea));
	if (!area)
		goto error;

	local->type_name = type_name;
	local->flags = 0;
	local->device_control = gWacomModule.DevProc;
	local->read_input = gWacomModule.DevReadInput;
	local->control_proc = gWacomModule.DevChangeControl;
	local->close_proc = gWacomModule.DevClose;
	local->switch_mode = gWacomModule.DevSwitchMode;
	local->conversion_proc = gWacomModule.DevConvert;
	local->reverse_conversion_proc = gWacomModule.DevReverseConvert;
	local->fd = -1;
	local->atom = 0;
	local->dev = NULL;
	local->private = priv;
	local->private_flags = 0;
	local->old_x = -1;
	local->old_y = -1;

	priv->next = NULL;
	priv->local = local;
	priv->flags = flag;          /* various flags (device type, absolute, first touch...) */
	priv->oldX = 0;             /* previous X position */
	priv->oldY = 0;             /* previous Y position */
	priv->oldZ = 0;             /* previous pressure */
	priv->oldTiltX = 0;         /* previous tilt in x direction */
	priv->oldTiltY = 0;         /* previous tilt in y direction */
	priv->oldStripX = 0;	    /* previous left strip value */
	priv->oldStripY = 0;	    /* previous right strip value */
	priv->oldButtons = 0;        /* previous buttons state */
	priv->oldWheel = 0;          /* previous wheel */
	priv->topX = 0;              /* X top */
	priv->topY = 0;              /* Y top */
	priv->bottomX = 0;           /* X bottom */
	priv->bottomY = 0;           /* Y bottom */
	priv->wcmMaxX = 0;           /* max tool logical X value */
	priv->wcmMaxY = 0;           /* max tool logical Y value */
	priv->wcmResolX = 0;         /* tool X resolution in points/inch */
	priv->wcmResolY = 0;         /* tool Y resolution in points/inch */
	priv->sizeX = 0;	     /* active X size */
	priv->sizeY = 0;	     /* active Y size */
	priv->factorX = 0.0;         /* X factor */
	priv->factorY = 0.0;         /* Y factor */
	priv->common = common;       /* common info pointer */
	priv->oldProximity = 0;      /* previous proximity */
	priv->hardProx = 1;	     /* previous hardware proximity */
	priv->old_serial = 0;	     /* last active tool's serial */
	priv->old_device_id = IsStylus(priv) ? STYLUS_DEVICE_ID :
		(IsEraser(priv) ? ERASER_DEVICE_ID : 
		(IsCursor(priv) ? CURSOR_DEVICE_ID : 
		(IsTouch(priv) ? TOUCH_DEVICE_ID :
		PAD_DEVICE_ID)));

	priv->devReverseCount = 0;   /* flag for relative Reverse call */
	priv->serial = 0;            /* serial number */
	priv->screen_no = -1;        /* associated screen */
	priv->speed = DEFAULT_SPEED; /* rel. mode speed */
	priv->accel = 0;	     /* rel. mode acceleration */
	priv->nPressCtrl [0] = 0;    /* pressure curve x0 */
	priv->nPressCtrl [1] = 0;    /* pressure curve y0 */
	priv->nPressCtrl [2] = 100;  /* pressure curve x1 */
	priv->nPressCtrl [3] = 100;  /* pressure curve y1 */

	/* Default button and expresskey values */
	for (i=0; i<WCM_MAX_BUTTONS; i++)
		priv->button[i] = (AC_BUTTON | (i + 1));

	for (i=0; i<WCM_MAX_BUTTONS; i++)
		for (j=0; j<256; j++)
			priv->keys[i][j] = 0;

	priv->nbuttons = WCM_MAX_BUTTONS;		/* Default number of buttons */
	priv->relup = 5;			/* Default relative wheel up event */
	priv->reldn = 4;			/* Default relative wheel down event */
	
	priv->wheelup = IsPad (priv) ? 4 : 0;	/* Default absolute wheel up event */
	priv->wheeldn = IsPad (priv) ? 5 : 0;	/* Default absolute wheel down event */
	priv->striplup = 4;			/* Default left strip up event */
	priv->stripldn = 5;			/* Default left strip down event */
	priv->striprup = 4;			/* Default right strip up event */
	priv->striprdn = 5;			/* Default right strip down event */
	priv->naxes = 6;			/* Default number of axes */
	priv->debugLevel = 0;			/* debug level */
	priv->numScreen = screenInfo.numScreens; /* configured screens count */
	priv->currentScreen = -1;                /* current screen in display */

	priv->maxWidth = 0;			/* max active screen width */
	priv->maxHeight = 0;			/* max active screen height */
	priv->leftPadding = 0;			/* left padding for virtual tablet */
	priv->topPadding = 0;			/* top padding for virtual tablet */
	priv->twinview = TV_NONE;		/* not using twinview gfx */
	priv->tvoffsetX = 0;			/* none X edge offset for TwinView setup */
	priv->tvoffsetY = 0;			/* none Y edge offset for TwinView setup */
	for (i=0; i<4; i++)
		priv->tvResolution[i] = 0;	/* unconfigured twinview resolution */
	priv->wcmMMonitor = 1;			/* enabled (=1) to support multi-monitor desktop. */
						/* disabled (=0) when user doesn't want to move the */
						/* cursor from one screen to another screen */

	/* JEJ - throttle sampling code */
	priv->throttleValue = 0;
	priv->throttleStart = 0;
	priv->throttleLimit = -1;
	
	common->wcmDevice = "";                  /* device file name */
	common->min_maj = 0;			 /* device major and minor */
	common->wcmFlags = RAW_FILTERING_FLAG;   /* various flags */
	common->wcmDevices = priv;
	common->npadkeys = 0;		   /* Default number of pad keys */
	common->wcmProtocolLevel = 4;      /* protocol level */
	common->wcmThreshold = 0;       /* unconfigured threshold */
	common->wcmISDV4Speed = 38400;  /* serial ISDV4 link speed */
	common->debugLevel = 0;         /* shared debug level can only 
					 * be changed though xsetwacom */

	common->wcmDevCls = &gWacomUSBDevice; /* device-specific functions */
	common->wcmModel = NULL;                 /* model-specific functions */
	common->wcmEraserID = 0;	 /* eraser id associated with the stylus */
	common->wcmTPCButtonDefault = 0; /* default Tablet PC button support is off */
	common->wcmTPCButton = 
		common->wcmTPCButtonDefault; /* set Tablet PC button on/off */
	common->wcmTouch = 0;              /* touch is disabled */
	common->wcmTouchDefault = 0; 	   /* default to disable when touch isn't supported */
	common->wcmCapacity = -1;          /* Capacity is disabled */
	common->wcmCapacityDefault = -1;    /* default to -1 when capacity isn't supported */
					   /* 3 when capacity is supported */
	common->wcmRotate = ROTATE_NONE;   /* default tablet rotation to off */
	common->wcmMaxX = 0;               /* max digitizer logical X value */
	common->wcmMaxY = 0;               /* max digitizer logical Y value */
	common->wcmMaxTouchX = 1024;       /* max touch X value */
	common->wcmMaxTouchY = 1024;       /* max touch Y value */
        common->wcmMaxZ = 0;               /* max Z value */
        common->wcmMaxCapacity = 0;        /* max capacity value */
 	common->wcmMaxDist = 0;            /* max distance value */
	common->wcmResolX = 0;             /* digitizer X resolution in points/inch */
	common->wcmResolY = 0;             /* digitizer Y resolution in points/inch */
	common->wcmTouchResolX = 0;        /* touch X resolution in points/mm */
	common->wcmTouchResolY = 0;        /* touch Y resolution in points/mm */
	common->wcmMaxStripX = 4096;       /* Max fingerstrip X */
	common->wcmMaxStripY = 4096;       /* Max fingerstrip Y */
	common->wcmMaxtiltX = 128;	   /* Max tilt in X directory */
	common->wcmMaxtiltY = 128;	   /* Max tilt in Y directory */
	common->wcmMaxCursorDist = 0;	/* Max distance received so far */
	common->wcmCursorProxoutDist = 0;
			/* Max mouse distance for proxy-out max/256 units */
	common->wcmCursorProxoutDistDefault = PROXOUT_INTUOS_DISTANCE; 
			/* default to Intuos */
	common->wcmSuppress = DEFAULT_SUPPRESS;    
			/* transmit position if increment is superior */
	common->wcmRawSample = DEFAULT_SAMPLES;    
			/* number of raw data to be used to for filtering */

	/* tool */
	priv->tool = tool;
	common->wcmTool = tool;
	tool->next = NULL;          /* next tool in list */
	tool->typeid = DEVICE_ID(flag); /* tool type (stylus/touch/eraser/cursor/pad) */
	tool->serial = 0;           /* serial id */
	tool->current = NULL;       /* current area in-prox */
	tool->arealist = area;      /* list of defined areas */
	/* tool area */
	priv->toolarea = area;
	area->next = NULL;    /* next area in list */
	area->topX = 0;       /* X top */
	area->topY = 0;       /* Y top */
	area->bottomX = 0;    /* X bottom */
	area->bottomY = 0;    /* Y bottom */
	area->device = local; /* associated WacomDevice */

	return 1;

error:
	xfree(area);
	xfree(tool);
	xfree(common);
	xfree(priv);
	return 0;
}

static int xf86WcmAllocateByType(LocalDevicePtr local, char *type)
{
	int rc = 0;

	if (!type)
	{
		xf86Msg(X_ERROR, "%s: No type or invalid type specified.\n"
				"Must be one of stylus, touch, cursor, eraser, or pad\n",
				local->name);
		return rc;
	}

	if (xf86NameCmp(type, "stylus") == 0)
		rc = xf86WcmAllocate(local, XI_STYLUS, ABSOLUTE_FLAG|STYLUS_ID);
	else if (xf86NameCmp(type, "touch") == 0)
		rc = xf86WcmAllocate(local, XI_TOUCH, ABSOLUTE_FLAG|TOUCH_ID);
	else if (xf86NameCmp(type, "cursor") == 0)
		rc = xf86WcmAllocate(local, XI_CURSOR, CURSOR_ID);
	else if (xf86NameCmp(type, "eraser") == 0)
		rc = xf86WcmAllocate(local, XI_ERASER, ABSOLUTE_FLAG|ERASER_ID);
	else if (xf86NameCmp(type, "pad") == 0)
		rc = xf86WcmAllocate(local, XI_PAD, PAD_ID);

	return rc;
}


/* xf86WcmPointInArea - check whether the point is within the area */

Bool xf86WcmPointInArea(WacomToolAreaPtr area, int x, int y)
{
	if (area->topX <= x && x <= area->bottomX &&
	    area->topY <= y && y <= area->bottomY)
		return 1;
	return 0;
}

/* xf86WcmAreasOverlap - check if two areas are overlapping */

static Bool xf86WcmAreasOverlap(WacomToolAreaPtr area1, WacomToolAreaPtr area2)
{
	if (xf86WcmPointInArea(area1, area2->topX, area2->topY) ||
	    xf86WcmPointInArea(area1, area2->topX, area2->bottomY) ||
	    xf86WcmPointInArea(area1, area2->bottomX, area2->topY) ||
	    xf86WcmPointInArea(area1, area2->bottomX, area2->bottomY))
		return 1;
	if (xf86WcmPointInArea(area2, area1->topX, area1->topY) ||
	    xf86WcmPointInArea(area2, area1->topX, area1->bottomY) ||
	    xf86WcmPointInArea(area2, area1->bottomX, area1->topY) ||
	    xf86WcmPointInArea(area2, area1->bottomX, area1->bottomY))
	        return 1;
	return 0;
}

/* xf86WcmAreaListOverlaps - check if the area overlaps any area in the list */
Bool xf86WcmAreaListOverlap(WacomToolAreaPtr area, WacomToolAreaPtr list)
{
	for (; list; list=list->next)	
		if (area != list && xf86WcmAreasOverlap(list, area))
			return 1;
	return 0;
}

/* 
 * Be sure to set vmin appropriately for your device's protocol. You want to
 * read a full packet before returning
 *
 * JEJ - Actually, anything other than 1 is probably a bad idea since packet
 * errors can occur.  When that happens, bytes are read individually until it
 * starts making sense again.
 */

static const char *default_options[] =
{
	"StopBits",    "1",
	"DataBits",    "8",
	"Parity",      "None",
	"Vmin",        "1",
	"Vtime",       "10",
	"FlowControl", "Xoff",
	NULL
};

/* xf86WcmUninit - called when the device is no longer needed. */

static void xf86WcmUninit(InputDriverPtr drv, LocalDevicePtr local, int flags)
{
	WacomDevicePtr priv = (WacomDevicePtr) local->private;
	WacomDevicePtr dev;
	WacomDevicePtr *prev;

	DBG(1, priv->debugLevel, ErrorF("xf86WcmUninit\n"));

	if (priv->isParent)
	{
		/* HAL removal sees the parent device removed first. */
		WacomDevicePtr next;
		dev = priv->common->wcmDevices;

		xf86Msg(X_INFO, "%s: removing automatically added devices.\n",
			local->name);

		while(dev)
		{
			next = dev->next;
			if (!dev->isParent && dev->uniq == priv->uniq)
			{
				xf86Msg(X_INFO, "%s: removing dependent device '%s'\n",
					local->name, dev->local->name);
				DeleteInputDeviceRequest(dev->local->dev);
			}
			dev = next;
		}
	}

	prev = &priv->common->wcmDevices;
	dev = *prev;
	while(dev)
	{
		if (dev == priv)
		{
			*prev = dev->next;
			break;
		}
		prev = &dev->next;
		dev = dev->next;
	}

	/* free pressure curve */
	xfree(priv->pPressCurve);

	xfree(priv);
	local->private = NULL;


	xf86DeleteInput(local, 0);    
}

/* xf86WcmMatchDevice - locate matching device and merge common structure */

static Bool xf86WcmMatchDevice(LocalDevicePtr pMatch, LocalDevicePtr pLocal)
{
	WacomDevicePtr privMatch = (WacomDevicePtr)pMatch->private;
	WacomDevicePtr priv = (WacomDevicePtr)pLocal->private;
	WacomCommonPtr common = priv->common;
	char * type;

	if ((pLocal != pMatch) &&
		strstr(pMatch->drv->driverName, "wacom") &&
		!strcmp(privMatch->common->wcmDevice, common->wcmDevice))
	{
		DBG(2, priv->debugLevel, ErrorF(
			"xf86WcmInit wacom port share between"
			" %s and %s\n", pLocal->name, pMatch->name));
		type = xf86FindOptionValue(pMatch->options, "Type");
		if ( type && (strstr(type, "eraser")) )
			privMatch->common->wcmEraserID=pMatch->name;
		else
		{
			type = xf86FindOptionValue(pLocal->options, "Type");
			if ( type && (strstr(type, "eraser")) )
			{
				privMatch->common->wcmEraserID=pLocal->name;
			}
		}
		xfree(common);
		common = priv->common = privMatch->common;
		priv->next = common->wcmDevices;
		common->wcmDevices = priv;
		return 1;
	}
	return 0;
}

/* xf86WcmCheckTypeAndSource - Check if both devices have the same type OR
 * the device has been used in xorg.conf: don't add the tool by hal/udev
 * if user has defined at least one tool for the device in xorg.conf */
static Bool xf86WcmCheckTypeAndSource(LocalDevicePtr local, LocalDevicePtr pLocal)
{
	int match = 1;
	char* fsource = xf86CheckStrOption(local->options, "_source", "");
	char* psource = xf86CheckStrOption(pLocal->options, "_source", "");
	char* type = xf86FindOptionValue(local->options, "Type");
#ifdef DEBUG
	WacomDevicePtr priv = (WacomDevicePtr) pLocal->private;
#endif

	/* only add the new tool if the matching major/minor
	 * was from the same source */
	if (!strcmp(fsource, psource))
	{
		/* and the tools have different types */
		if (strcmp(type, xf86FindOptionValue(pLocal->options, "Type")))
			match = 0;
	}
	DBG(2, priv->debugLevel, xf86Msg(X_INFO, "xf86WcmCheckTypeAndSource "
		"device %s from %s %s \n", local->name, fsource,
		match ? "will be added" : "will be ignored"));

	return match;
}

/* check if the device has been added.
 * Open the device and check it's major/minor, then compare this with every
 * other wacom device listed in the config. If they share the same
 * major/minor and the same source/type, fail.
 */
static int wcmIsDuplicate(char* device, LocalDevicePtr local)
{
	struct stat st;
	int isInUse = 0;
	LocalDevicePtr localDevices = NULL;
	WacomCommonPtr common = NULL;

	/* open the port */
        SYSCALL(local->fd = open(device, O_RDONLY, 0));

	if (local->fd < 0)
	{
		/* can not open the device */
		xf86Msg(X_ERROR, "%s: Unable to open Wacom device \"%s\".\n",
			local->name, device);
		isInUse = 2;
		goto ret;
	}

	if (fstat(local->fd, &st) == -1)
	{
		/* can not access major/minor to check device duplication */
		xf86Msg(X_ERROR, "%s: stat failed (%s). cannot check for duplicates.\n",
				local->name, strerror(errno));

		/* older systems don't support the required ioctl.  let it pass */
		goto ret;
	}

	if (st.st_rdev)
	{
		localDevices = xf86FirstLocalDevice();

		for (; localDevices != NULL; localDevices = localDevices->next)
		{
			device = xf86CheckStrOption(localDevices->options, "Device", NULL);

			/* device can be NULL on some distros */
			if (!device || !strstr(localDevices->drv->driverName, "wacom"))
				continue;

			if (local == localDevices)
				continue;

			common = ((WacomDevicePtr)localDevices->private)->common;
			if (common->min_maj && common->min_maj == st.st_rdev)
			{
				/* device matches with another added port */
				if (xf86WcmCheckTypeAndSource(local, localDevices))
				{
					xf86Msg(X_WARNING, "%s: device file already in use by %s. "
						"Ignoring.\n", local->name, localDevices->name);
					isInUse = 4;
					goto ret;
				}
			}
		}
	}
	else
	{
		/* major/minor can never be 0, right? */
		xf86Msg(X_ERROR, "%s: device opened with a major/minor of 0. "
			"Something was wrong.\n", local->name);
		isInUse = 5;
	}
ret:
	if (local->fd >= 0)
	{
		close(local->fd);
		local->fd = -1;
	}
	return isInUse;
}

static struct wcmProduct
{
	__u16 productID;
	__u16 flags;
} validType [] =
{
	{ 0x00, STYLUS_ID }, /* PenPartner */
	{ 0x10, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Graphire */
	{ 0x11, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Graphire2 4x5 */
	{ 0x12, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Graphire2 5x7 */

	{ 0x13, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Graphire3 4x5 */
	{ 0x14, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Graphire3 6x8 */

	{ 0x15, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Graphire4 4x5 */
	{ 0x16, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Graphire4 6x8 */
	{ 0x17, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* BambooFun 4x5 */
	{ 0x18, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* BambooFun 6x8 */
	{ 0x19, STYLUS_ID | ERASER_ID                      }, /* Bamboo1 Medium*/
	{ 0x81, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Graphire4 6x8 BlueTooth */

	{ 0x20, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos 4x5 */
	{ 0x21, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos 6x8 */
	{ 0x22, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos 9x12 */
	{ 0x23, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos 12x12 */
	{ 0x24, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos 12x18 */

	{ 0x03, STYLUS_ID | ERASER_ID }, /* PTU600 */
	{ 0x30, STYLUS_ID | ERASER_ID }, /* PL400 */
	{ 0x31, STYLUS_ID | ERASER_ID }, /* PL500 */
	{ 0x32, STYLUS_ID | ERASER_ID }, /* PL600 */
	{ 0x33, STYLUS_ID | ERASER_ID }, /* PL600SX */
	{ 0x34, STYLUS_ID | ERASER_ID }, /* PL550 */
	{ 0x35, STYLUS_ID | ERASER_ID }, /* PL800 */
	{ 0x37, STYLUS_ID | ERASER_ID }, /* PL700 */
	{ 0x38, STYLUS_ID | ERASER_ID }, /* PL510 */
	{ 0x39, STYLUS_ID | ERASER_ID }, /* PL710 */
	{ 0xC0, STYLUS_ID | ERASER_ID }, /* DTF720 */
	{ 0xC2, STYLUS_ID | ERASER_ID }, /* DTF720a */
	{ 0xC4, STYLUS_ID | ERASER_ID }, /* DTF521 */
	{ 0xC7, STYLUS_ID | ERASER_ID }, /* DTU1931 */

	{ 0x41, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos2 4x5 */
	{ 0x42, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos2 6x8 */
	{ 0x43, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos2 9x12 */
	{ 0x44, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos2 12x12 */
	{ 0x45, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos2 12x18 */
	{ 0x47, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos2 6x8  */

	{ 0x60, STYLUS_ID }, /* Volito */
	{ 0x61, STYLUS_ID }, /* PenStation */
	{ 0x62, STYLUS_ID }, /* Volito2 4x5 */
	{ 0x63, STYLUS_ID }, /* Volito2 2x3 */
	{ 0x64, STYLUS_ID }, /* PenPartner2 */

	{ 0x65, STYLUS_ID | ERASER_ID | CURSOR_ID |  PAD_ID }, /* Bamboo */
	{ 0x69, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Bamboo1 */

	{ 0xB0, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 4x5 */
	{ 0xB1, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 6x8 */
	{ 0xB2, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 9x12 */
	{ 0xB3, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 12x12 */
	{ 0xB4, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 12x19 */
	{ 0xB5, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 6x11 */
	{ 0xB7, STYLUS_ID | ERASER_ID | CURSOR_ID }, /* Intuos3 4x6 */

	{ 0xB8, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Intuos4 4x6 */
	{ 0xB9, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Intuos4 6x9 */
	{ 0xBA, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Intuos4 8x13 */
	{ 0xBB, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Intuos4 12x19*/

	{ 0x3F, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Cintiq 21UX */
	{ 0xC5, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Cintiq 20WSX */
	{ 0xC6, STYLUS_ID | ERASER_ID | CURSOR_ID | PAD_ID }, /* Cintiq 12WX */

	{ 0x90, STYLUS_ID | ERASER_ID }, /* TabletPC 0x90 */
	{ 0x93, STYLUS_ID | ERASER_ID  | TOUCH_ID }, /* TabletPC 0x93 */
	{ 0x9A, STYLUS_ID | ERASER_ID  | TOUCH_ID }, /* TabletPC 0x9A */
	{ 0x9F, TOUCH_ID }, /* CapPlus  0x9F */
	{ 0xE2, TOUCH_ID }, /* TabletPC 0xE2 */
	{ 0xE3, STYLUS_ID | ERASER_ID | TOUCH_ID }  /* TabletPC 0xE3 */
};

static struct
{
	const char* type;
	__u16 id;
} wcmTypeAndID [] =
{
	{ "stylus", STYLUS_ID },
	{ "eraser", ERASER_ID },
	{ "cursor", CURSOR_ID },
	{ "touch",  TOUCH_ID  },
	{ "pad",    PAD_ID    }
};

static struct wcmProduct* wcmFindProduct(unsigned short id)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(validType); i++)
		if (validType[i].productID == id)
			return &validType[i];
	return NULL;
}

/* validate tool type for device/product */
static int wcmIsAValidType(char* device, LocalDevicePtr local,
			   unsigned short id, char* type)
{
	int j, ret = 0;
	struct wcmProduct *product;

	if (!type)
	    return ret;

	product = wcmFindProduct(id);
	if (!product)
		return ret;

	/* walkthrough all types */
	for (j = 0; j < ARRAY_SIZE(wcmTypeAndID); j++)
		if (!strcmp(wcmTypeAndID[j].type, type))
			if (wcmTypeAndID[j].id & product->flags)
			{
				ret = 1;
				break;
			}

	return ret;
}

/**
 * Duplicate xf86 options, replace the "type" option with the given type
 * (and the name with "$name $type" and convert them to InputOption */
static InputOption *wcmOptionDupConvert(LocalDevicePtr local, const char *type)
{
	pointer original = local->options;
	InputOption *iopts = NULL, *new;
	InputInfoRec dummy;
	char *name;

	memset(&dummy, 0, sizeof(dummy));
	xf86CollectInputOptions(&dummy, NULL, original);

	name = xcalloc(strlen(local->name) + strlen(type) + 2, 1);
	sprintf(name, "%s %s", local->name, type);

	dummy.options = xf86ReplaceStrOption(dummy.options, "Type", type);
	dummy.options = xf86ReplaceStrOption(dummy.options, "Name", name);
	xfree(name);

	while(dummy.options)
	{
		new = xcalloc(1, sizeof(InputOption));

		new->key = xf86OptionName(dummy.options);
		new->value = xf86OptionValue(dummy.options);
		new->next = iopts;
		iopts = new;
		dummy.options = xf86NextOption(dummy.options);
	}
	return iopts;
}

static void wcmFreeInputOpts(InputOption* opts)
{
	InputOption *tmp = opts;
	while(opts)
	{
		tmp = opts->next;
		xfree(opts->key);
		xfree(opts->value);
		xfree(opts);
		opts = tmp;
	}
}

/**
 * Hotplug one device of the given type.
 * Device has the same options as the "parent" device, type is one of
 * erasor, stylus, pad, etc.
 * Name of the new device is set automatically to "<device name> <type>".
 */
static void wcmHotplug(LocalDevicePtr local, const char *type)
{
	DeviceIntPtr dev; /* dummy */
	InputOption *input_options;

	input_options = wcmOptionDupConvert(local, type);

	NewInputDeviceRequest(input_options, &dev);
	wcmFreeInputOpts(input_options);
}

static void wcmHotplugOthers(LocalDevicePtr local, unsigned short id)
{
	int i, skip = 1;
	struct wcmProduct *product;

	product = wcmFindProduct(id);
	if (!product)
		return;

        xf86Msg(X_INFO, "%s: hotplugging dependent devices.\n", local->name);
        /* same loop is used to init the first device, if we get here we
         * need to start at the second one */
	for (i = 0; i < ARRAY_SIZE(wcmTypeAndID); i++)
	{
		if (wcmTypeAndID[i].id & product->flags)
		{
			if (skip)
				skip = 0;
			else
				wcmHotplug(local, wcmTypeAndID[i].type);
		}
	}
        xf86Msg(X_INFO, "%s: hotplugging completed.\n", local->name);
}

/**
 * Return 1 if the device needs auto-hotplugging from within the driver.
 * This is the case if we don't get passed a "type" option (invalid in
 * xorg.conf configurations) and we come from HAL or whatever future config
 * backend.
 *
 * This changes the source to _driver/wacom, all auto-hotplugged devices
 * will have the same source.
 */
static int wcmNeedAutoHotplug(LocalDevicePtr local, char **type,
			      unsigned short id)
{
	char *source = xf86CheckStrOption(local->options, "_source", "");
	struct wcmProduct *product;
	int i;

	if (*type) /* type specified, don't hotplug */
		return 0;

	/* Only supporting HAL so far */
	if (strcmp(source, "server/hal"))
		return 0;

	/* no type specified, so we need to pick the first one applicable
	 * for our product */
	product = wcmFindProduct(id);
	if (!product)
		return 0;

	for (i = 0; i < ARRAY_SIZE(wcmTypeAndID); i++)
	{
		if (wcmTypeAndID[i].id & product->flags)
		{
			*type = strdup(wcmTypeAndID[i].type);
			break;
		}
	}

	xf86Msg(X_INFO, "%s: type not specified, assuming '%s'.\n", local->name, *type);
	xf86Msg(X_INFO, "%s: other types will be automatically added.\n", local->name);

	local->options = xf86AddNewOption(local->options, "Type", *type);
	local->options = xf86ReplaceStrOption(local->options, "_source", "_driver/wacom");

	return 1;
}

static int wcmParseOptions(LocalDevicePtr local)
{
	WacomDevicePtr  priv = (WacomDevicePtr)local->private;
	WacomCommonPtr  common = priv->common;
	char            *s, b[12];
	int		i, oldButton;
	WacomToolPtr    tool = NULL;
	WacomToolAreaPtr area = NULL;


	/* Special option set for auto-hotplugged devices only */
	priv->uniq = xf86CheckIntOption(local->options, "_wacom uniq", 0);

	/* Optional configuration */
	priv->debugLevel = xf86SetIntOption(local->options,
					    "DebugLevel", priv->debugLevel);
	common->debugLevel = xf86SetIntOption(local->options,
					      "CommonDBG", common->debugLevel);
	s = xf86SetStrOption(local->options, "Mode", NULL);

	if (s && (xf86NameCmp(s, "absolute") == 0))
		priv->flags |= ABSOLUTE_FLAG;
	else if (s && (xf86NameCmp(s, "relative") == 0))
		priv->flags &= ~ABSOLUTE_FLAG;
	else if (s)
	{
		xf86Msg(X_ERROR, "%s: invalid Mode (should be absolute or "
			"relative). Using default.\n", local->name);

		/* stylus/eraser defaults to absolute mode 
		 * cursor defaults to relative mode 
		 */
		if (IsCursor(priv)) 
			priv->flags &= ~ABSOLUTE_FLAG;
		else 
			priv->flags |= ABSOLUTE_FLAG;
	}

	/* Pad is always in relative mode when it's a core device.
	 * Always in absolute mode when it is not a core device.
	 */
	if (IsPad(priv))
		xf86WcmSetPadCoreMode(local);

	/* Store original local Core flag so it can be changed later */
	if (local->flags & (XI86_ALWAYS_CORE | XI86_CORE_POINTER))
		priv->flags |= COREEVENT_FLAG;

	/* ISDV4 support */
	s = xf86SetStrOption(local->options, "ForceDevice", NULL);

	if (s)
	{
		if (xf86NameCmp(s, "ISDV4") == 0)
		{
			common->wcmForceDevice=DEVICE_ISDV4;
			common->wcmDevCls = &gWacomISDV4Device;
			common->wcmTPCButtonDefault = 1; /* Tablet PC buttons on by default */
		} else
		{
			xf86Msg(X_ERROR, "%s: invalid ForceDevice option '%s'.\n",
				local->name, s);
			goto error;
		}
	}

	s = xf86SetStrOption(local->options, "Rotate", NULL);

	if (s)
	{
		if (xf86NameCmp(s, "CW") == 0)
			common->wcmRotate=ROTATE_CW;
		else if (xf86NameCmp(s, "CCW") ==0)
			common->wcmRotate=ROTATE_CCW;
		else if (xf86NameCmp(s, "HALF") ==0)
			common->wcmRotate=ROTATE_HALF;
		else if (xf86NameCmp(s, "NONE") !=0)
		{
			xf86Msg(X_ERROR, "%s: invalid Rotate option '%s'.\n",
				local->name, s);
			goto error;
		}
	}

	common->wcmSuppress = xf86SetIntOption(local->options, "Suppress",
			common->wcmSuppress);
	if (common->wcmSuppress != 0) /* 0 disables suppression */
	{
		if (common->wcmSuppress > MAX_SUPPRESS)
			common->wcmSuppress = MAX_SUPPRESS;
		if (common->wcmSuppress < DEFAULT_SUPPRESS)
			common->wcmSuppress = DEFAULT_SUPPRESS;
	}

	if (xf86SetBoolOption(local->options, "Tilt",
			(common->wcmFlags & TILT_REQUEST_FLAG)))
	{
		common->wcmFlags |= TILT_REQUEST_FLAG;
	}

	if (xf86SetBoolOption(local->options, "RawFilter",
			(common->wcmFlags & RAW_FILTERING_FLAG)))
	{
		common->wcmFlags |= RAW_FILTERING_FLAG;
	}

	if (xf86SetBoolOption(local->options, "USB",
			(common->wcmDevCls == &gWacomUSBDevice)))
		common->wcmDevCls = &gWacomUSBDevice;

	/* pressure curve takes control points x1,y1,x2,y2
	 * values in range from 0..100.
	 * Linear curve is 0,0,100,100
	 * Slightly depressed curve might be 5,0,100,95
	 * Slightly raised curve might be 0,5,95,100
	 */
	s = xf86SetStrOption(local->options, "PressCurve", NULL);
	if (s && !IsCursor(priv) && !IsTouch(priv)) 
	{
		int a,b,c,d;
		if ((sscanf(s,"%d,%d,%d,%d",&a,&b,&c,&d) != 4) ||
			(a < 0) || (a > 100) || (b < 0) || (b > 100) ||
			(c < 0) || (c > 100) || (d < 0) || (d > 100))
			xf86Msg(X_CONFIG, "%s: PressCurve not valid\n",
				local->name);
		else
		{
			xf86WcmSetPressureCurve(priv,a,b,c,d);
		}
	}

	if (IsCursor(priv))
	{
		common->wcmCursorProxoutDist = xf86SetIntOption(local->options, "CursorProx", 0);
		if (common->wcmCursorProxoutDist < 0 || common->wcmCursorProxoutDist > 255)
			xf86Msg(X_CONFIG, "%s: CursorProx invalid %d \n",
				local->name, common->wcmCursorProxoutDist);
	}

	/* Configure Monitors' resoluiton in TwinView setup.
	 * The value is in the form of "1024x768,1280x1024" 
	 * for a desktop of monitor 1 at 1024x768 and 
	 * monitor 2 at 1280x1024
	 */
	s = xf86SetStrOption(local->options, "TVResolution", NULL);
	if (s)
	{
		int a,b,c,d;
		if ((sscanf(s,"%dx%d,%dx%d",&a,&b,&c,&d) != 4) ||
			(a <= 0) || (b <= 0) || (c <= 0) || (d <= 0))
			xf86Msg(X_CONFIG, "%s: TVResolution not valid\n",
				local->name);
		else
		{
			priv->tvResolution[0] = a;
			priv->tvResolution[1] = b;
			priv->tvResolution[2] = c;
			priv->tvResolution[3] = d;
		}
	}
    
	priv->screen_no = xf86SetIntOption(local->options, "ScreenNo", -1);
 
	if (xf86SetBoolOption(local->options, "KeepShape", 0))
		priv->flags |= KEEP_SHAPE_FLAG;

	priv->topX = xf86SetIntOption(local->options, "TopX", 0);
	priv->topY = xf86SetIntOption(local->options, "TopY", 0);
	priv->bottomX = xf86SetIntOption(local->options, "BottomX", 0);
	priv->bottomY = xf86SetIntOption(local->options, "BottomY", 0);
	priv->serial = xf86SetIntOption(local->options, "Serial", 0);

	tool = priv->tool;
	area = priv->toolarea;
	area->topX = priv->topX;
	area->topY = priv->topY;
	area->bottomX = priv->bottomX;
	area->bottomY = priv->bottomY;
	tool->serial = priv->serial;

	/* The first device doesn't need to add any tools/areas as it
	 * will be the first anyway. So if different, add tool
	 * and/or area to the existing lists 
	 */
	if(tool != common->wcmTool)
	{
		WacomToolPtr toollist = NULL;
		for(toollist = common->wcmTool; toollist; toollist = toollist->next)
			if(tool->typeid == toollist->typeid && tool->serial == toollist->serial) 
				break;

		if(toollist) /* Already have a tool with the same type/serial */
		{
			WacomToolAreaPtr arealist;

			xfree(tool);
			priv->tool = tool = toollist;
			arealist = toollist->arealist;

			/* Add the area to the end of the list */
			while(arealist->next)
				arealist = arealist->next;
			arealist->next = area;
		}
		else /* No match on existing tool/serial, add tool to the end of the list */
		{
			toollist = common->wcmTool;
			while(toollist->next)
				toollist = toollist->next;
			toollist->next = tool;
		}
	}

	common->wcmScaling = 0;

	common->wcmThreshold = xf86SetIntOption(local->options, "Threshold",
			common->wcmThreshold);

	priv->wcmMaxX = xf86SetIntOption(local->options, "MaxX",
					 common->wcmMaxX);

	/* Update tablet logical max X */
	if (!IsTouch(priv)) common->wcmMaxX = priv->wcmMaxX;

	priv->wcmMaxY = xf86SetIntOption(local->options, "MaxY",
					 common->wcmMaxY);

	/* Update tablet logical max Y */
	if (!IsTouch(priv)) common->wcmMaxY = priv->wcmMaxY;

	common->wcmMaxZ = xf86SetIntOption(local->options, "MaxZ",
					   common->wcmMaxZ);
	common->wcmUserResolX = xf86SetIntOption(local->options, "ResolutionX",
						 common->wcmUserResolX);
	common->wcmUserResolY = xf86SetIntOption(local->options, "ResolutionY",
						 common->wcmUserResolY);
	common->wcmUserResolZ = xf86SetIntOption(local->options, "ResolutionZ",
						 common->wcmUserResolZ);
	if (xf86SetBoolOption(local->options, "ButtonsOnly", 0))
		priv->flags |= BUTTONS_ONLY_FLAG;

	/* Tablet PC button applied to the whole tablet. Not just one tool */
	if ( priv->flags & STYLUS_ID )
		common->wcmTPCButton = xf86SetBoolOption(local->options,
							 "TPCButton",
							 common->wcmTPCButtonDefault);

	/* Touch applies to the whole tablet */
	common->wcmTouch = xf86SetBoolOption(local->options, "Touch", common->wcmTouchDefault);

	/* Touch capacity applies to the whole tablet */
	common->wcmCapacity = xf86SetBoolOption(local->options, "Capacity", common->wcmCapacityDefault);

	/* Mouse cursor stays in one monitor in a multimonitor setup */
	if ( !priv->wcmMMonitor )
		priv->wcmMMonitor = xf86SetBoolOption(local->options, "MMonitor", 1);

	for (i=0; i<WCM_MAX_BUTTONS; i++)
	{
		sprintf(b, "Button%d", i+1);
		s = xf86SetStrOption(local->options, b, NULL);
		if (s)
		{
			oldButton = priv->button[i];
			priv->button[i] = xf86SetIntOption(local->options, b, priv->button[i]);
		}
	}

	if (common->wcmForceDevice == DEVICE_ISDV4)
        {
		int val;
		val = xf86SetIntOption(local->options, "BaudRate", 9600);

		switch(val)
		{
			case 38400:
			case 19200:
			case 9600:
				common->wcmISDV4Speed = val;
				break;
			default:
				xf86Msg(X_ERROR, "%s: Illegal speed value "
					"(must be 9600 or 19200 or 38400).",
					local->name);
				break;
		}
	}

	priv->speed = xf86SetRealOption(local->options, "Speed", DEFAULT_SPEED);
	priv->accel = xf86SetIntOption(local->options, "Accel", 0);

	s = xf86SetStrOption(local->options, "Twinview", NULL);
	if (s && xf86NameCmp(s, "none") == 0) 
		priv->twinview = TV_NONE;
	else if ((s && xf86NameCmp(s, "horizontal") == 0) ||
			(s && xf86NameCmp(s, "rightof") == 0)) 
		priv->twinview = TV_LEFT_RIGHT;
	else if ((s && xf86NameCmp(s, "vertical") == 0) ||
			(s && xf86NameCmp(s, "belowof") == 0)) 
		priv->twinview = TV_ABOVE_BELOW;
	else if (s && xf86NameCmp(s, "leftof") == 0) 
		priv->twinview = TV_RIGHT_LEFT;
	else if (s && xf86NameCmp(s, "aboveof") == 0) 
		priv->twinview = TV_BELOW_ABOVE;
	else if (s) 
	{
		xf86Msg(X_ERROR, "%s: invalid Twinview (should be none, vertical (belowof), "
			"horizontal (rightof), aboveof, or leftof). Using none.\n",
			local->name);
		priv->twinview = TV_NONE;
	}

	return 1;
error:
	xfree(area);
	xfree(tool);
	return 0;
}


/* xf86WcmInit - called for each input devices with the driver set to
 * "wacom" */
static LocalDevicePtr xf86WcmInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
	LocalDevicePtr local = NULL;
	WacomDevicePtr priv = NULL;
	WacomCommonPtr common = NULL;
	char		*type;
	char*		device;
	static int	numberWacom = 0;
	struct input_id id;
	int		need_hotplug = 0;

	gWacomModule.wcmDrv = drv;

	if (!(local = xf86AllocateInput(drv, 0)))
		goto SetupProc_fail;

	local->conf_idev = dev;
	local->name = dev->identifier;

	/* Force default port options to exist because the init
	 * phase is based on those values.
	 */
	xf86CollectInputOptions(local, default_options, NULL);

	device = xf86CheckStrOption(local->options, "Device", NULL);

	if(device && !xf86WcmIsWacomDevice(device, &id))
		goto SetupProc_fail;

	type = xf86FindOptionValue(local->options, "Type");
	need_hotplug = wcmNeedAutoHotplug(local, &type, id.product);

	/* If a device is hotplugged, the current time is taken as uniq
	 * stamp for this group of devices. On removal, this helps us
	 * identify which other devices need to be removed. */
	if (need_hotplug)
		local->options = xf86ReplaceIntOption(local->options,"_wacom uniq",
						      currentTime.milliseconds);

	/* leave the undefined for auto-dev (if enabled) to deal with */
	if(device)
	{
		/* check if the type is valid for the device */
		if(!wcmIsAValidType(device, local, id.product, type))
			goto SetupProc_fail;

		/* check if the device has been added */
		if (wcmIsDuplicate(device, local))
			goto SetupProc_fail;
	}

	if (!xf86WcmAllocateByType(local, type))
		goto SetupProc_fail;

	priv = (WacomDevicePtr) local->private;
	common = priv->common;

	common->wcmDevice = xf86SetStrOption(local->options, "Device", NULL);

	/* Autoprobe if not given
	 * Hotplugging is supported. Do we still need this?
	 * Maybe add an ifdef for hal/udev?
	 * Only use this once since AutoDevProbe doesn't check the existing tools
	 */
	if ((!common->wcmDevice || !strcmp (common->wcmDevice, "auto-dev")) && numberWacom)
	{
		common->wcmFlags |= AUTODEV_FLAG;
		if (! (common->wcmDevice = xf86WcmEventAutoDevProbe (local)))
		{
			xf86Msg(X_ERROR, "%s: unable to probe device\n",
				local->name);
			goto SetupProc_fail;
		}
	}

	/* Lookup to see if there is another wacom device sharing the same port */
	if (common->wcmDevice)
	{
		LocalDevicePtr localDevices = xf86FirstLocalDevice();
		for (; localDevices != NULL; localDevices = localDevices->next)
		{
			if (xf86WcmMatchDevice(localDevices,local))
			{
				common = priv->common;
				break;
			}
		}
	}

	/* Process the common options. */
	xf86ProcessCommonOptions(local, local->options);
	if (!wcmParseOptions(local))
		goto SetupProc_fail;

	/* mark the device configured */
	local->flags |= XI86_POINTER_CAPABLE | XI86_CONFIGURED;

	/* keep a local count so we know if "auto-dev" is necessary or not */
	numberWacom++;

	if (need_hotplug)
	{
		priv->isParent = 1;
		wcmHotplugOthers(local, id.product);
	}

	/* return the LocalDevice */
	return (local);

SetupProc_fail:
	xfree(common);
	xfree(priv);
	if (local)
	    xf86DeleteInput(local, 0);
	return NULL;
}

InputDriverRec WACOM =
{
	1,             /* driver version */
	"wacom",       /* driver name */
	NULL,          /* identify */
	xf86WcmInit,   /* pre-init */
	xf86WcmUninit, /* un-init */
	NULL,          /* module */
	0              /* ref count */
};


/* xf86WcmUnplug - Uninitialize the device */

static void xf86WcmUnplug(pointer p)
{
	xf86Msg(X_INFO, "xf86WcmUnplug\n");
}

/* xf86WcmPlug - called by the module loader */

static pointer xf86WcmPlug(pointer module, pointer options, int* errmaj,
		int* errmin)
{
	xf86AddInputDriver(&WACOM, module, 0);
	return module;
}

static XF86ModuleVersionInfo xf86WcmVersionRec =
{
	"wacom",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}  /* signature, to be patched into the file by a tool */
};

_X_EXPORT XF86ModuleData wacomModuleData =
{
	&xf86WcmVersionRec,
	xf86WcmPlug,
	xf86WcmUnplug
};

/* vim: set noexpandtab shiftwidth=8: */
