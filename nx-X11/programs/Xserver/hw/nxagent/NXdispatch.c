/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/************************************************************

Copyright 1987, 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/


#ifdef __sun
#define False 0
#define True 1
#endif

#include <X11/fonts/fontstruct.h>

#define GC XlibGC
#include <nx-X11/Xlib.h>
#undef GC

#include "../../dix/dispatch.c"

#include "windowstr.h"

#include "Atoms.h"
#include "Splash.h"
#include "Xdmcp.h"
#include "Client.h"
#include "Clipboard.h"
#include "Reconnect.h"
#include "Millis.h"
#include "Font.h"
#include <nx/Shadow.h>
#include "Handlers.h"
#include "Keyboard.h"
#include "Init.h"
#include "Utils.h"

const int nxagentMaxFontNames = 10000;

/*
 * This allows the agent to exit if no
 * client is connected within a timeout.
 */

int nxagentClients = 0;

void nxagentWaitDisplay(void);

void nxagentListRemoteFonts(const char *, int);

#include "Xatom.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  WATCH

/*
 * Log begin and end of the important handlers.
 */

#undef  BLOCKS

#ifdef WATCH
#include "unistd.h"
#endif

#include "Literals.h"

#ifdef VIEWPORT_FRAME

extern WindowPtr nxagentViewportFrameLeft;
extern WindowPtr nxagentViewportFrameRight;
extern WindowPtr nxagentViewportFrameAbove;
extern WindowPtr nxagentViewportFrameBelow;

#define IsViewportFrame(pWin) ((pWin) == nxagentViewportFrameLeft || \
                                   (pWin) == nxagentViewportFrameRight || \
                                       (pWin) == nxagentViewportFrameAbove || \
                                           (pWin) == nxagentViewportFrameBelow)

#else

#define IsViewportFrame(pWin) (0)

#endif /* #ifdef VIEWPORT_FRAME */

extern int nxagentMaxAllowedResets;

extern int nxagentFindClientResource(int, RESTYPE, void *);

#ifdef NXAGENT_CLIPBOARD
extern int nxagentPrimarySelection;
extern int nxagentClipboardSelection;
extern int nxagentMaxSelections;
#endif

extern int nxOpenFont(ClientPtr, XID, Mask, unsigned, char*);


/*
 * This used to be a dix variable used only by XPRINT, so xorg dropped it.
 */
ClientPtr nxagentRequestingClient;

void
InitSelections(void)
{
    xorg_InitSelections();

#ifdef NXAGENT_CLIPBOARD
    {
      Selection *newsels;
      newsels = (Selection *)malloc(nxagentMaxSelections * sizeof(Selection));
      if (!newsels)
        return;
      NumCurrentSelections += nxagentMaxSelections;
      CurrentSelections = newsels;

      /* Note: these are the same values that will be set on a SelectionClear event */

      CurrentSelections[nxagentPrimarySelection].selection = XA_PRIMARY;
      CurrentSelections[nxagentPrimarySelection].lastTimeChanged = ClientTimeToServerTime(CurrentTime);
      CurrentSelections[nxagentPrimarySelection].window = screenInfo.screens[0]->root->drawable.id;
      CurrentSelections[nxagentPrimarySelection].pWin = NULL;
      CurrentSelections[nxagentPrimarySelection].client = NullClient;

      CurrentSelections[nxagentClipboardSelection].selection = MakeAtom("CLIPBOARD", 9, 1);
      CurrentSelections[nxagentClipboardSelection].lastTimeChanged = ClientTimeToServerTime(CurrentTime);
      CurrentSelections[nxagentClipboardSelection].window = screenInfo.screens[0]->root->drawable.id;
      CurrentSelections[nxagentClipboardSelection].pWin = NULL;
      CurrentSelections[nxagentClipboardSelection].client = NullClient;
    }
#endif
}

#define MAJOROP ((xReq *)client->requestBuffer)->reqType

void
Dispatch(void)
{
    register int        *clientReady;     /* array of request ready clients */
    register int	result;
    register ClientPtr	client;
    register int	nready;
    register HWEventQueuePtr* icheck = checkForInput;
    long			start_tick;

    nextFreeClientID = 1;
    InitSelections();
    nClients = 0;

#ifdef NXAGENT_SERVER
    /*
     * The agent initialization was successfully
     * completed. We can now handle our clients.
     */

    #ifdef XKB

    nxagentInitXkbWrapper();

    nxagentTuneXkbWrapper();

    #endif

    #ifdef NXAGENT_ONSTART

    /*
     * Set NX_WM property (used by NX client to identify the agent's
     * window) three seconds since the first client connects.
     */

    unsigned int nxagentWMtimeout = GetTimeInMillis() + 3000;

    #endif

#endif /* NXAGENT_SERVER */
    clientReady = (int *) malloc(sizeof(int) * MaxClients);
    if (!clientReady)
	return;

#ifdef NXAGENT_SERVER
    #ifdef WATCH

    fprintf(stderr, "Dispatch: Watchpoint 12.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#3      1		352 bits (0 KB) ->	236 bits (0 KB) ->	352/1 -> 236/1	= 1.492:1
#14     1		256 bits (0 KB) ->	101 bits (0 KB) ->	256/1 -> 101/1	= 2.535:1
#16     1		256 bits (0 KB) ->	26 bits (0 KB) ->	256/1 -> 26/1	= 9.846:1
#20     2	2	12256 bits (1 KB) ->	56 bits (0 KB) ->	6128/1 -> 28/1	= 218.857:1
#43     1		256 bits (0 KB) ->	45 bits (0 KB) ->	256/1 -> 45/1	= 5.689:1
#47     2	2	42304 bits (5 KB) ->	49 bits (0 KB) ->	21152/1 -> 24/1	= 863.347:1
#98     1		256 bits (0 KB) ->	34 bits (0 KB) ->	256/1 -> 34/1	= 7.529:1
*/

    sleep(30);

    #endif

    #ifdef TEST
    fprintf(stderr, "Dispatch: Value of dispatchException is [%x].\n",
                dispatchException);

    fprintf(stderr, "Dispatch: Value of dispatchExceptionAtReset is [%x].\n",
                dispatchExceptionAtReset);
    #endif

    if (!(dispatchException & DE_TERMINATE))
        dispatchException = 0;

    /* Init TimeoutTimer if requested */
    nxagentSetTimeoutTimer(0);
#endif /* NXAGENT_SERVER */

    while (!dispatchException)
    {
        if (*icheck[0] != *icheck[1])
	{
	    ProcessInputEvents();
	    FlushIfCriticalOutputPending();
	}

#ifdef NXAGENT_SERVER
        /*
         * Ensure we remove the splash after the timeout.
         * Initializing clientReady[0] to -1 will tell
         * WaitForSomething() to yield control after the
         * timeout set in clientReady[1].
         */

        clientReady[0] = 0;

        if (nxagentHaveSplashWindow() || (nxagentOption(Xdmcp) && !nxagentXdmcpUp))
        {
          #ifdef TEST
          fprintf(stderr, "******Dispatch: Requesting a timeout of [%d] Ms.\n",
                      NXAGENT_WAKEUP);
          #endif

          clientReady[0] = -1;
          clientReady[1] = NXAGENT_WAKEUP;
        }

        if (serverGeneration > nxagentMaxAllowedResets &&
                nxagentSessionState == SESSION_STARTING &&
                    (!nxagentOption(Xdmcp) || nxagentXdmcpUp))
        {
          #ifdef NX_DEBUG_INPUT
          fprintf(stderr, "Session: Session started at '%s' timestamp [%u].\n",
                      GetTimeAsString(), GetTimeInMillis());
          #else
          fprintf(stderr, "Session: Session started at '%s'.\n",
                      GetTimeAsString());
          #endif

          nxagentSessionState = SESSION_UP;
          saveAgentState("RUNNING");
        }

        #ifdef BLOCKS
        fprintf(stderr, "[End dispatch]\n");
        #endif
#endif /* NXAGENT_SERVER */

	nready = WaitForSomething(clientReady);

#ifdef NXAGENT_SERVER
        #ifdef BLOCKS
        fprintf(stderr, "[Begin dispatch]\n");
        #endif

        #ifdef TEST
        fprintf(stderr, "******Dispatch: Running with [%d] clients ready.\n",
                    nready);
        #endif
        
        #ifdef NXAGENT_ONSTART

	/*
	 * If the timeout is expired set the selection informing the
	 * NX client that the agent is ready.
	 */

	if (nxagentWMtimeout < GetTimeInMillis())
	{
	  nxagentRemoveSplashWindow();
	}

        nxagentClients = nClients;

        #endif
#endif /* NXAGENT_SERVER */

	if (nready)
	{
	    clientReady[0] = SmartScheduleClient (clientReady, nready);
	    nready = 1;
	}
       /***************** 
	*  Handle events in round robin fashion, doing input between 
	*  each round 
	*****************/

	while (!dispatchException && (--nready >= 0))
	{
	    client = clients[clientReady[nready]];
	    if (! client)
	    {
		/* KillClient can cause this to happen */
		continue;
	    }
	    /* GrabServer activation can cause this to be true */
	    if (grabState == GrabKickout)
	    {
		grabState = GrabActive;
		break;
	    }
	    isItTimeToYield = FALSE;
 
#ifdef NXAGENT_SERVER
	    nxagentRequestingClient = client;
#endif
	    start_tick = SmartScheduleTime;
	    while (!isItTimeToYield)
	    {
	        if (*icheck[0] != *icheck[1])
		{
		    ProcessInputEvents();
		    FlushIfCriticalOutputPending();
		}
		if ((SmartScheduleTime - start_tick) >= SmartScheduleSlice)
		{
		    /* Penalize clients which consume ticks */
		    if (client->smart_priority > SMART_MIN_PRIORITY)
			client->smart_priority--;
		    break;
		}
		/* now, finally, deal with client requests */

		/* Update currentTime so request time checks, such as for input
		 * device grabs, are calculated correctly */
		UpdateCurrentTimeIf();
#ifdef NXAGENT_SERVER
                #ifdef TEST
                fprintf(stderr, "******Dispatch: Reading request from client [%d].\n",
                            client->index);
                #endif
#endif /* NXAGENT_SERVER */

	        result = ReadRequestFromClient(client);
	        if (result <= 0) 
	        {
		    if (result < 0)
			CloseDownClient(client);
		    break;
	        }

#ifdef NXAGENT_SERVER
                #ifdef TEST

                else
                {

                    if (MAJOROP > 127)
                    {
                      fprintf(stderr, "******Dispatch: Read [Extension] request OPCODE#%d MINOR#%d "
                                  "size [%d] client [%d].\n", MAJOROP, *((char *) client->requestBuffer + 1),
                                      client->req_len << 2, client->index);
                    }
                    else
                    {
                      fprintf(stderr, "******Dispatch: Read [%s] request OPCODE#%d size [%d] client [%d].\n",
                                  nxagentRequestLiteral[MAJOROP], MAJOROP, client->req_len << 2,
                                      client->index);
                    }
                }

                #endif
#endif

		client->sequence++;
		if (result > (maxBigRequestSize << 2))
		    result = BadLength;
		else
                {
                    result = (* client->requestVector[MAJOROP])(client);
#ifdef NXAGENT_SERVER
                    #ifdef TEST

                    if (MAJOROP > 127)
                    {
                      fprintf(stderr, "******Dispatch: Handled [Extension] request OPCODE#%d MINOR#%d "
                                  "size [%d] client [%d] result [%d].\n", MAJOROP,
                                      *((char *) client->requestBuffer + 1), client->req_len << 2,
                                          client->index, result);
                    }
                    else
                    {
                      fprintf(stderr, "******Dispatch: Handled [%s] request OPCODE#%d size [%d] client [%d] "
                                  "result [%d].\n", nxagentRequestLiteral[MAJOROP], MAJOROP,
                                      client->req_len << 2, client->index, result);
                    }

                    #endif

                    /*
                     * Can set isItTimeToYield to force
                     * the dispatcher to pay attention
                     * to another client.
                     */

                    nxagentDispatchHandler(client, client->req_len << 2, 0);
#endif
                }

                if (!SmartScheduleSignalEnable)
                    SmartScheduleTime = GetTimeInMillis();

		if (result != Success) 
		{
		    if (client->noClientException != Success)
                        CloseDownClient(client);
                    else
		        SendErrorToClient(client, MAJOROP,
					  MinorOpcodeOfRequest(client),
					  client->errorValue, result);
		    break;
	        }
#ifdef DAMAGEEXT
		FlushIfCriticalOutputPending ();
#endif
	    }
	    FlushAllOutput();
	    client = clients[clientReady[nready]];
	    if (client)
		client->smart_stop_tick = SmartScheduleTime;
#ifdef NXAGENT_SERVER
	    nxagentRequestingClient = NULL;
#endif
	}
	dispatchException &= ~DE_PRIORITYCHANGE;
    }
#if defined(DDXBEFORERESET)
    ddxBeforeReset ();
#endif

#ifdef NXAGENT_SERVER
    nxagentFreeTimeoutTimer();

    /* FIXME: maybe move the code up to the KillAllClients() call to ddxBeforeReset? */
    if ((dispatchException & DE_RESET) && 
            (serverGeneration > nxagentMaxAllowedResets))
    {
        dispatchException &= ~DE_RESET;
        dispatchException |= DE_TERMINATE;

        fprintf(stderr, "Info: Reached threshold of maximum allowed resets.\n");
    }

    nxagentResetAtomMap();

    if (serverGeneration > nxagentMaxAllowedResets)
    {
      /*
       * The session is terminating. Force an I/O
       * error on the display and wait until the
       * NX transport is gone.
       */
  
      fprintf(stderr, "Session: Terminating session at '%s'.\n", GetTimeAsString());
      saveAgentState("TERMINATING");

      nxagentWaitDisplay();

      fprintf(stderr, "Session: Session terminated at '%s'.\n", GetTimeAsString());
    }

    if (nxagentOption(Shadow))
    {
      NXShadowDestroy();
    }
    saveAgentState("TERMINATED");

    nxagentFreeFontData();
#endif /* NXAGENT_SERVER */

    nxagentFreeAtomMap();

    KillAllClients();
    free(clientReady);
    dispatchException &= ~DE_RESET;
}

#undef MAJOROP

int
ProcReparentWindow(register ClientPtr client)
{
    register WindowPtr pWin, pParent;
    REQUEST(xReparentWindowReq);
    register int result;

    REQUEST_SIZE_MATCH(xReparentWindowReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   DixWriteAccess);
    if (!pWin)
        return(BadWindow);

#ifdef NXAGENT_SERVER
    nxagentRemoveSplashWindow();
#endif

    pParent = (WindowPtr)SecurityLookupWindow(stuff->parent, client,
					      DixWriteAccess);
    if (!pParent)
        return(BadWindow);
    if (SAME_SCREENS(pWin->drawable, pParent->drawable))
    {
        if ((pWin->backgroundState == ParentRelative) &&
            (pParent->drawable.depth != pWin->drawable.depth))
            return BadMatch;
	if ((pWin->drawable.class != InputOnly) &&
	    (pParent->drawable.class == InputOnly))
	    return BadMatch;
        result =  ReparentWindow(pWin, pParent, 
			 (short)stuff->x, (short)stuff->y, client);
	if (client->noClientException != Success)
            return(client->noClientException);
	else
            return(result);
    }
    else 
        return (BadMatch);
}


int
ProcQueryTree(register ClientPtr client)
{
    xQueryTreeReply reply = {0};
    int numChildren = 0;
    register WindowPtr pChild, pWin, pHead;
    Window  *childIDs = (Window *)NULL;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->id, client,
					   DixReadAccess);
    if (!pWin)
        return(BadWindow);
    reply.type = X_Reply;
    reply.root = pWin->drawable.pScreen->root->drawable.id;
    reply.sequenceNumber = client->sequence;
    if (pWin->parent)
	reply.parent = pWin->parent->drawable.id;
    else
        reply.parent = (Window)None;
    pHead = RealChildHead(pWin);
    for (pChild = pWin->lastChild; pChild != pHead; pChild = pChild->prevSib)
#ifdef NXAGENT_SERVER
    {
        if (!IsViewportFrame(pChild))
        {
            numChildren++;
        }

    }
#else
        numChildren++;
#endif
    if (numChildren)
    {
	int curChild = 0;

	childIDs = (Window *) malloc(numChildren * sizeof(Window));
	if (!childIDs)
	    return BadAlloc;
	for (pChild = pWin->lastChild; pChild != pHead; pChild = pChild->prevSib)
#ifdef NXAGENT_SERVER
	{
            if (!IsViewportFrame(pChild))
            {
	        childIDs[curChild++] = pChild->drawable.id;
            }
        }
#else
            childIDs[curChild++] = pChild->drawable.id;
#endif
    }
    
    reply.nChildren = numChildren;
    reply.length = (numChildren * sizeof(Window)) >> 2;
    
    WriteReplyToClient(client, sizeof(xQueryTreeReply), &reply);
    if (numChildren)
    {
    	client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
	WriteSwappedDataToClient(client, numChildren * sizeof(Window), childIDs);
	free(childIDs);
    }

    return(client->noClientException);
}


int
ProcConvertSelection(register ClientPtr client)
{
    Bool paramsOkay;
    xEvent event;
    WindowPtr pWin;
    REQUEST(xConvertSelectionReq);

    REQUEST_SIZE_MATCH(xConvertSelectionReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->requestor, client,
					   DixReadAccess);
    if (!pWin)
        return(BadWindow);

#ifdef NXAGENT_CLIPBOARD
    if (((stuff->selection == XA_PRIMARY) ||
           (stuff->selection == MakeAtom("CLIPBOARD", 9, 0))) &&
               nxagentOption(Clipboard) != ClipboardNone)
    {
      int index = nxagentFindCurrentSelectionIndex(stuff->selection);
      if ((index != -1) && (CurrentSelections[index].window != None))
      {
        if (nxagentConvertSelection(client, pWin, stuff->selection, stuff->requestor,
                                       stuff->property, stuff->target, stuff->time))
        {
          return (client->noClientException);
        }
      }
    }
#endif

    paramsOkay = (ValidAtom(stuff->selection) && ValidAtom(stuff->target));
    if (stuff->property != None)
	paramsOkay &= ValidAtom(stuff->property);
    if (paramsOkay)
    {
	int i;

	i = 0;
	while ((i < NumCurrentSelections) && 
	       CurrentSelections[i].selection != stuff->selection) i++;
	if ((i < NumCurrentSelections) &&
	    (CurrentSelections[i].window != None)
#ifdef NXAGENT_SERVER
            /*
             * .window can be set and pointing to our server window to
             * signal the clipboard owner being on the real X
             * server. Therefore we need to check .client in addition
             * to ensure having a local owner.
             */
	    && (CurrentSelections[i].client != NullClient)
#endif
#ifdef XCSECURITY
	    && (!client->CheckAccess ||
		(* client->CheckAccess)(client, CurrentSelections[i].window,
					RT_WINDOW, DixReadAccess,
					CurrentSelections[i].pWin))
#endif
	    )
	{        
	    memset(&event, 0, sizeof(xEvent));
	    event.u.u.type = SelectionRequest;
	    event.u.selectionRequest.time = stuff->time;
	    event.u.selectionRequest.owner = 
			CurrentSelections[i].window;
	    event.u.selectionRequest.requestor = stuff->requestor;
	    event.u.selectionRequest.selection = stuff->selection;
	    event.u.selectionRequest.target = stuff->target;
	    event.u.selectionRequest.property = stuff->property;
	    if (TryClientEvents(
		CurrentSelections[i].client, &event, 1, NoEventMask,
		NoEventMask /* CantBeFiltered */, NullGrab))
		return (client->noClientException);
	}
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = SelectionNotify;
	event.u.selectionNotify.time = stuff->time;
	event.u.selectionNotify.requestor = stuff->requestor;
	event.u.selectionNotify.selection = stuff->selection;
	event.u.selectionNotify.target = stuff->target;
	event.u.selectionNotify.property = None;
	(void) TryClientEvents(client, &event, 1, NoEventMask,
			       NoEventMask /* CantBeFiltered */, NullGrab);
	return (client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->property;
        return (BadAtom);
    }
}


int
ProcOpenFont(register ClientPtr client)
{
    int	err;
    REQUEST(xOpenFontReq);

    REQUEST_FIXED_SIZE(xOpenFontReq, stuff->nbytes);
    client->errorValue = stuff->fid;
    LEGAL_NEW_RESOURCE(stuff->fid, client);

#ifdef NXAGENT_SERVER
    char fontReq[256];
    memcpy(fontReq,(char *)&stuff[1], (stuff->nbytes < 256) ? stuff->nbytes : 255);
    fontReq[stuff->nbytes] = '\0';
    if (strchr(fontReq, '*') || strchr(fontReq, '?'))
    {
#ifdef NXAGENT_FONTMATCH_DEBUG
       fprintf(stderr, "%s: try to find a common font with font pattern [%s]\n", __func__, fontReq);
#endif
       nxagentListRemoteFonts(fontReq, nxagentMaxFontNames);
       err = nxOpenFont(client, stuff->fid, (Mask) 0,
		stuff->nbytes, (char *)&stuff[1]);
    }
    else
#endif
    err = OpenFont(client, stuff->fid, (Mask) 0,
		stuff->nbytes, (char *)&stuff[1]);
    if (err == Success)
    {
	return(client->noClientException);
    }
    else
	return err;
}

int
ProcCloseFont(register ClientPtr client)
{
    FontPtr pFont;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pFont = (FontPtr)SecurityLookupIDByType(client, stuff->id, RT_FONT,
					    DixDestroyAccess);
    if ( pFont != (FontPtr)NULL)	/* id was valid */
    {
        #ifdef NXAGENT_SERVER

        /*
         * When a client closes a font the resource should not be lost
         * if the reference counter is not 0, otherwise the server
         * will not be able to find this font looping through the
         * resources.
         */

        if (pFont -> refcnt > 0)
        {
          if (nxagentFindClientResource(serverClient -> index, RT_NX_FONT, pFont) == 0)
          {
            #ifdef TEST
            fprintf(stderr, "%s: Switching resource for font at [%p].\n", __func__,
                        (void *) pFont);
            #endif

            nxagentFontPriv(pFont) -> mirrorID = FakeClientID(serverClient -> index);

            AddResource(nxagentFontPriv(pFont) -> mirrorID, RT_NX_FONT, pFont);

          }
          #ifdef TEST
          else
          {
            fprintf(stderr, "%s: Found duplicated font at [%p], "
                        "resource switching skipped.\n", __func__, (void *) pFont);
          }
          #endif
        }

        #endif

        FreeResource(stuff->id, RT_NONE);
	return(client->noClientException);
    }
    else
    {
	client->errorValue = stuff->id;
        return (BadFont);
    }
}


int
ProcListFonts(register ClientPtr client)
{
    REQUEST(xListFontsReq);

    REQUEST_FIXED_SIZE(xListFontsReq, stuff->nbytes);

#ifdef NXAGENT_SERVER
    char tmp[256];
    memcpy(tmp, (unsigned char *) &stuff[1], (stuff->nbytes < 256) ? stuff->nbytes : 255);
    tmp[stuff->nbytes] = '\0';

#ifdef NXAGENT_FONTMATCH_DEBUG
    fprintf(stderr, "%s: ListFont request with pattern [%s] max_names [%d]\n", __func__, tmp, stuff->maxNames);
#endif
    nxagentListRemoteFonts(tmp, stuff -> maxNames < nxagentMaxFontNames ? nxagentMaxFontNames : stuff->maxNames);
#endif

    return ListFonts(client, (unsigned char *) &stuff[1], stuff->nbytes, 
	stuff->maxNames);
}

int
ProcListFontsWithInfo(register ClientPtr client)
{
    REQUEST(xListFontsWithInfoReq);

    REQUEST_FIXED_SIZE(xListFontsWithInfoReq, stuff->nbytes);

#ifdef NXAGENT_SERVER
    char tmp[256];
    memcpy(tmp, (unsigned char *) &stuff[1], (stuff->nbytes < 256) ? stuff->nbytes : 255);
    tmp[stuff->nbytes] = '\0';
#ifdef NXAGENT_FONTMATCH_DEBUG
    fprintf(stderr, "%s: ListFont with info request with pattern [%s] max_names [%d]\n", __func__, tmp, stuff->maxNames);
#endif
    nxagentListRemoteFonts(tmp, stuff -> maxNames < nxagentMaxFontNames ? nxagentMaxFontNames : stuff->maxNames);
#endif

    return StartListFontsWithInfo(client, stuff->nbytes,
				  (unsigned char *) &stuff[1], stuff->maxNames);
}

int
ProcFreePixmap(register ClientPtr client)
{
    PixmapPtr pMap;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pMap = (PixmapPtr)SecurityLookupIDByType(client, stuff->id, RT_PIXMAP,
					     DixDestroyAccess);
    if (pMap) 
    {
        #ifdef NXAGENT_SERVER

        /*
         * When a client releases a pixmap the resource should not be
         * lost if the reference counter is not 0, otherwise the
         * server will not be able to find this pixmap looping through
         * the resources.
         */

        if (pMap -> refcnt > 0)
        {
          if (nxagentFindClientResource(serverClient -> index, RT_NX_PIXMAP, pMap) == 0)
          {
            #ifdef TEST
            fprintf(stderr, "ProcFreePixmap: Switching resource for pixmap at [%p].\n",
                       (void *) pMap);
            #endif

            nxagentPixmapPriv(pMap) -> mid = FakeClientID(serverClient -> index);

            AddResource(nxagentPixmapPriv(pMap) -> mid, RT_NX_PIXMAP, pMap);
          }
          #ifdef TEST
          else
          {
            fprintf(stderr, "%s: Found duplicated pixmap at [%p], "
                        "resource switching skipped.\n", __func__, (void *) pMap);
          }
          #endif
        }

        #endif

	FreeResource(stuff->id, RT_NONE);
	return(client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->id;
	return (BadPixmap);
    }
}

/**********************
 * CloseDownClient
 *
 *  Client can either mark his resources destroy or retain.  If retained and
 *  then killed again, the client is really destroyed.
 *********************/

void
CloseDownClient(register ClientPtr client)
{
    #ifdef DEBUG
    fprintf(stderr, "%s: [%d]\n", __func__, client->clientState);
    #endif

#ifdef NXAGENT_SERVER
    /*
     * Need to reset the karma counter and get rid of the pending sync
     * replies.
     */

    nxagentWakeupByReset(client);
#endif

    xorg_CloseDownClient(client);
}
