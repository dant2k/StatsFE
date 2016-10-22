#include "Gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "rrAtomics.h"
#include "rrThreads.h"

static void* RADLINK Alloc(UINTa bytes, U32 should_zero, char const* file, U32 line)
{
    void* ptr = malloc(bytes);
    if (should_zero)
        memset(ptr, 0, bytes);
    return ptr;
}

static void RADLINK Free(void* ptr, char const* file, U32 line)
{
    free(ptr);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RADLINK GuiLinkTick()
{
    
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RADLINK GuiLinkGetParams(GuiInitParams* io_Params)
{
    io_Params->AllocFn = Alloc;
    io_Params->FreeFn = Free;
    io_Params->WindowTitle = "Stats Front End";
    io_Params->MinWindowWidth = 800;
    io_Params->MinWindowHeight = 440;
    io_Params->IconResourceName = "";

    io_Params->NoDebugConsole = 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RADLINK GuiLinkStartup()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RADLINK GuiLinkShutdown()
{

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static S32 RADLINK GuiLinkOnQuit()
{
    return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static S32 RADLINK GuiLinkConsoleMain()
{
    return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void RADLINK GuiLinkGetScheme(GuiSchemeParams* Params)
{
    Params->PaneSelectionTexture = ~0U;
    Params->PaneSelectionColor = 0xffffF4DD;
    Params->PaneSelectionColorFocused = 0xffFFD4AA;
}

#include <windows.h>
DECLARE_STARTUP(GuiLinkStartup, GuiLinkShutdown, GuiLinkTick, GuiLinkGetParams, GuiLinkGetScheme, GuiLinkOnQuit, GuiLinkConsoleMain);
