//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//
// npshell.cpp
//
// This file defines a "shell" plugin that plugin developers can use
// as the basis for a real plugin.  This shell just provides empty
// implementations of all functions that the plugin can implement
// that will be called by Netscape (the NPP_xxx methods defined in 
// npapi.h). 
//
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#ifndef _NPAPI_H_
#include "npapi.h"
#endif


//
// Instance state information about the plugin.
//
// *Developers*: Use this struct to hold per-instance
//				 information that youÕll need in the
//				 various functions in this file.
//
typedef struct _PluginInstance
{
	NPWindow*		fWindow;
	uint16			fMode;
	
} PluginInstance;



//------------------------------------------------------------------------------------
// NPP_Initialize:
//------------------------------------------------------------------------------------
NPError NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}


//------------------------------------------------------------------------------------
// NPP_Shutdown:
//------------------------------------------------------------------------------------
void NPP_Shutdown(void)
{

}


//------------------------------------------------------------------------------------
// NPP_New:
//------------------------------------------------------------------------------------
NPError NP_LOADDS
NPP_New(NPMIMEType pluginType,
				NPP instance,
				uint16 mode,
				int16 argc,
				char* argn[],
				char* argv[],
				NPSavedData* saved)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
		
	instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
	PluginInstance* This = (PluginInstance*) instance->pdata;
	if (This != NULL)
	{
		This->fWindow = NULL;
		This->fMode = mode;    // Mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h)

		//
		// *Developers*: Initialize fields of your plugin
		// instance data here.  If the NPSavedData is non-
		// NULL, you can use that data (returned by you from
		// NPP_Destroy to set up the new plugin instance.
		//
		
		return NPERR_NO_ERROR;
	}
	else
		return NPERR_OUT_OF_MEMORY_ERROR;
}




//------------------------------------------------------------------------------------
// NPP_Destroy:
//------------------------------------------------------------------------------------
NPError NP_LOADDS
NPP_Destroy(NPP instance, NPSavedData** save)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	PluginInstance* This = (PluginInstance*) instance->pdata;

	//
	// *Developers*: If desired, call NP_MemAlloc to create a
	// NPSavedDate structure containing any state information
	// that you want restored if this plugin instance is later
	// recreated.
	//

	if (This != NULL)
	{
		NPN_MemFree(instance->pdata);
		instance->pdata = NULL;
	}

	return NPERR_NO_ERROR;
}




//------------------------------------------------------------------------------------
// NPP_SetWindow:
//------------------------------------------------------------------------------------
NPError NP_LOADDS
NPP_SetWindow(NPP instance, NPWindow* window)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	PluginInstance* This = (PluginInstance*) instance->pdata;

	//
	// *Developers*: Before setting fWindow to point to the
	// new window, you may wish to compare the new window
	// info to the previous window (if any) to note window
	// size changes, etc.
	//
	
	This->fWindow = window;

	return NPERR_NO_ERROR;
}



//------------------------------------------------------------------------------------
// NPP_NewStream:
//------------------------------------------------------------------------------------
NPError NP_LOADDS
NPP_NewStream(NPP instance,
							NPMIMEType type,
							NPStream *stream, 
							NPBool seekable,
							uint16 *stype)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	PluginInstance* This = (PluginInstance*) instance->pdata;

	return NPERR_NO_ERROR;
}




//
// *Developers*: 
// These next 2 functions are directly relevant in a plug-in which handles the
// data in a streaming manner.  If you want zero bytes because no buffer space
// is YET available, return 0.  As long as the stream has not been written
// to the plugin, Navigator will continue trying to send bytes.  If the plugin
// doesn't want them, just return some large number from NPP_WriteReady(), and
// ignore them in NPP_Write().  For a NP_ASFILE stream, they are still called
// but can safely be ignored using this strategy.
//

int32 STREAMBUFSIZE = 0X0FFFFFFF;   // If we are reading from a file in NPAsFile
                                    // mode so we can take any size stream in our
                                    // write call (since we ignore it)

//------------------------------------------------------------------------------------
// NPP_WriteReady:
//------------------------------------------------------------------------------------
int32 NP_LOADDS
NPP_WriteReady(NPP instance, NPStream *stream)
{
	if (instance != NULL)
	{
		PluginInstance* This = (PluginInstance*) instance->pdata;
	
	}

	return STREAMBUFSIZE;   // Number of bytes ready to accept in NPP_Write()
}



//------------------------------------------------------------------------------------
// NPP_Write:
//------------------------------------------------------------------------------------
int32 NP_LOADDS
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	if (instance != NULL)
	{
		PluginInstance* This = (PluginInstance*) instance->pdata;
	
	}

	return len; 			// The number of bytes accepted
}



//------------------------------------------------------------------------------------
// NPP_DestroyStream:
//------------------------------------------------------------------------------------
NPError NP_LOADDS
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	PluginInstance* This = (PluginInstance*) instance->pdata;

	return NPERR_NO_ERROR;
}


//------------------------------------------------------------------------------------
// NPP_StreamAsFile:
//------------------------------------------------------------------------------------
void NP_LOADDS
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	if (instance != NULL)
	{
		PluginInstance* This = (PluginInstance*) instance->pdata;
	
	}
}



//------------------------------------------------------------------------------------
// NPP_Print:
//------------------------------------------------------------------------------------
void NP_LOADDS
NPP_Print(NPP instance, NPPrint* printInfo)
{
    if(printInfo == NULL)   // trap invalid parm
        return;

	if (instance != NULL)
	{
		PluginInstance* This = (PluginInstance*) instance->pdata;
	
		if (printInfo->mode == NP_FULL)
		{
			//
			// *Developers*: If your plugin would like to take over
			// printing completely when it is in full-screen mode,
			// set printInfo->pluginPrinted to TRUE and print your
			// plugin as you see fit.  If your plugin wants Netscape
			// to handle printing in this case, set printInfo->pluginPrinted
			// to FALSE (the default) and do nothing.  If you do want
			// to handle printing yourself, printOne is true if the
			// print button (as opposed to the print menu) was clicked.
			// On the Macintosh, platformPrint is a THPrint; on Windows,
			// platformPrint is a structure (defined in npapi.h) containing
			// the printer name, port, etc.
			//
			void* platformPrint = printInfo->print.fullPrint.platformPrint;
			NPBool printOne = printInfo->print.fullPrint.printOne;
			
			printInfo->print.fullPrint.pluginPrinted = FALSE; // Do the default
		
		}
		else	// If not fullscreen, we must be embedded
		{
			//
			// *Developers*: If your plugin is embedded, or is full-screen
			// but you returned false in pluginPrinted above, NPP_Print
			// will be called with mode == NP_EMBED.  The NPWindow
			// in the printInfo gives the location and dimensions of
			// the embedded plugin on the printed page.  On the Macintosh,
			// platformPrint is the printer port; on Windows, platformPrint
			// is the handle to the printing device context.
			//
			NPWindow* printWindow = &(printInfo->print.embedPrint.window);
			void* platformPrint = printInfo->print.embedPrint.platformPrint;
		}
	}

}


//------------------------------------------------------------------------------------
// NPP_HandleEvent:
// Mac-only.
//------------------------------------------------------------------------------------
int16 NPP_HandleEvent(NPP instance, void* event)
{
	NPBool eventHandled = FALSE;
	if (instance == NULL)
		return eventHandled;
		
	PluginInstance* This = (PluginInstance*) instance->pdata;
	
	//
	// *Developers*: The "event" passed in is a Macintosh
	// EventRecord*.  The event.what field can be any of the
	// normal Mac event types, or one of the following additional
	// types defined in npapi.h: getFocusEvent, loseFocusEvent,
	// adjustCursorEvent.  The focus events inform your plugin
	// that it will become, or is no longer, the recepient of
	// key events.  If your plugin doesn't want to receive key
	// events, return false when passed at getFocusEvent.  The
	// adjustCursorEvent is passed repeatedly when the mouse is
	// over your plugin; if your plugin doesn't want to set the
	// cursor, return false.  Handle the standard Mac events as
	// normal.  The return value for all standard events is currently
	// ignored except for the key event: for key events, only return
	// true if your plugin has handled that particular key event. 
	//
	
	return eventHandled;
}

