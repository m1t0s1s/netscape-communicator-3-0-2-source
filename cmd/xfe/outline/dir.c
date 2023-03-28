/* dir.c: Demo for the Outline widget class. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>


#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/ArrowB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include "Outline.h"
#include "Handle.h"
#include <X11/Xmu/Editres.h>

/* bitmaps */

#define folder_width 16
#define folder_height 16
static char folder_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x84, 0x00, 0xfe, 0x3f, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00};

#define folder_minus_width 16
#define folder_minus_height 16
static char folder_minus_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x84, 0x00, 0xfe, 0x3f, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0xf2, 0x47, 0x02, 0x40, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00};

#define folder_plus_width 16
#define folder_plus_height 16
static char folder_plus_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x84, 0x00, 0xfe, 0x3f, 0x02, 0x40,
   0x82, 0x40, 0x82, 0x40, 0x82, 0x40, 0xf2, 0x47, 0x82, 0x40, 0x82, 0x40,
   0x82, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00};

String  translations = "#override\n\
	<Key>Down:	ManagerGadgetTraverseRight()\n\
	<Key>Up:	ManagerGadgetTraverseLeft()\n\
	<Key>End:	ManagerGadgetTraverseHome() \
			ManagerGadgetTraverseLeft()\n";


typedef struct folder {
  char *name;
  char *path;
  struct folder *up, *sub, *last, *next;
  int showall;
  Widget w;
} Node;


void		buildTree();
void		PushButtonHandleCB(); 
static XImage	*CreateDefaultImage();
void		traverseCB();
void		toggleCB();


extern int		errno;

static char		path[MAXPATHLEN] = "";
static char		statpath[MAXPATHLEN] = "";
static char		tmppath[MAXPATHLEN] = "";
static struct stat	statbuf;
static int		access_ok;


/* The following code will change in the near future. Believe me! */

static struct folder *searchfolder(struct folder *upfolder)
{
  /* this is OS dependent, it might be separated into its own file */

  struct dirent *dp;
  DIR *dirp;
  int error;
  ushort filemode;

  struct folder *newfolder;
  struct folder *lastfolder = NULL;
  struct folder *firstfolder = NULL;
  
  strcpy(path, upfolder->path);
  printf("now reading: '%s'\n", path);
  
  if (access (path, F_OK) && access(path, R_OK)) {
    if (errno != EACCES) {
      /*fprintf(stderr, "error access-testing file: %s, aborting!\n", path);*/
      /*exit(1);*/
    } else
      return NULL; /* could not read directory */
  }

  /* open directory for reading */
  dirp = opendir(path);
  
  /* find first entry */
  dp = readdir(dirp);
  
  while (dp != NULL) {
    
    strcat(strcat(strcpy(statpath, path), "/"), dp->d_name);
    if ((error = stat(statpath, &statbuf)) && (error != ENOENT)) {
      fprintf(stderr, "error stat'ing file: %s, aborting!\n", statpath);
      /*exit(1); */
    }
    
    filemode = (statbuf.st_mode & S_IFMT);
    
    if ((filemode & S_IFLNK) == S_IFLNK)
      printf("%s is a link\n", statpath); 
    
    if (filemode == S_IFDIR && filemode != S_IFLNK 
	&& strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {
      
      if ((newfolder = (struct folder *) malloc(sizeof(struct folder))) 
	  == NULL) {
	fprintf(stderr, "error while allocating memory, aborting.\n");
	exit(1);
      }
      
      newfolder->name = malloc(sizeof(char)*(strlen(dp->d_name)+1));
      strcpy(newfolder->name, dp->d_name);
      
      strcat(strcat(strcpy(tmppath, upfolder->path),"/"), newfolder->name);
      
      /* newfolder->path = upfolder->path + "/" + newfolder->name */
      newfolder->path = malloc(sizeof(char)*(strlen(tmppath)+1));
      newfolder->path = strcpy(newfolder->path, tmppath);
      newfolder->showall = True;
      
      if (lastfolder == NULL) firstfolder = newfolder;
      else lastfolder->next = newfolder;
      
      newfolder->last = lastfolder;
      newfolder->up = upfolder;
      lastfolder = newfolder;
      newfolder->next = NULL;
      newfolder->sub = NULL;
    }
    /* find next entry */
    dp = readdir(dirp);
  }
  
  /* close directory just read */
  closedir(dirp);
  
  newfolder = firstfolder;
  
  while (newfolder != NULL) {
    newfolder->sub = searchfolder(newfolder);
    newfolder = newfolder->next;
  }
	
  return firstfolder;
}

struct folder *searchtree(char *rootdir)
{
  struct folder *root;
  
  if (	((root = (struct folder *) malloc(sizeof(struct folder))) != 0) &&
      ((root->path = malloc(sizeof(char)*(strlen(rootdir)+1))) != 0) &&
      ((root->name = malloc(sizeof(char)*(strlen(rootdir)+1))) != 0)) {
    
    strcpy(root->path, rootdir);
    strcpy(root->name, rootdir);
    
    root->up = NULL;
    root->last = NULL;
    root->next = NULL;
    root->sub = searchfolder(root);
    root->showall = True;
  } else {
    fprintf(stderr, "error while allocating memory, aborting.\n");
    exit(1);
  }
  return root;
}

static XImage *CreateDefaultImage(char *bits, int width, int height)
{
  XImage *image;
  
  image = (XImage *) XtMalloc (sizeof (XImage));
  image->width = width;
  image->height = height;
  image->data = bits;
  image->depth = 1;
  image->xoffset = 0;
  image->format = XYBitmap;
  image->byte_order = LSBFirst;
  image->bitmap_unit = 8;
  image->bitmap_bit_order = LSBFirst;
  image->bitmap_pad = 8;
  image->bytes_per_line = (width+7)/8;
  return (image);
}

void main(int argc, char **argv)
{
  Widget        shell, scroller, outline, manager, toggle;
  XtAppContext  app;
  struct folder *root = NULL;
  void          _XEditResCheckMessages();
  XImage	*folder_image, *folder_minus_image, *folder_plus_image;
  Display	*dpy;
  int		screen;
  XmString	xmstr;
  
  shell = XtAppInitialize(&app, "Folders", NULL, 0, 
			  &argc, argv, NULL, NULL, 0);
  
  dpy = XtDisplay(shell);
  screen = DefaultScreen(dpy); 

  XtAddEventHandler(shell, (EventMask) 0, True, 
		    _XEditResCheckMessages, NULL);
  

  manager = XtVaCreateManagedWidget("manager", xmFormWidgetClass,
				    shell,
				    NULL);

  xmstr = XmStringCreate("Draw Outline", "");

  toggle = XtVaCreateManagedWidget("toggle", xmToggleButtonWidgetClass,
				   manager,
				   XmNset, TRUE,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNrightAttachment, XmATTACH_FORM,
				   XmNlabelString, xmstr,
				   NULL);
  XtAddCallback(toggle, XmNvalueChangedCallback, toggleCB, NULL);
  XmStringFree(xmstr);
  
  scroller = XtVaCreateManagedWidget("scroller", xmScrolledWindowWidgetClass,
				     manager,
                                     XmNscrollingPolicy, XmAUTOMATIC,
				     XmNscrollBarDisplayPolicy, XmSTATIC,
				     XmNtopAttachment, XmATTACH_WIDGET,
				     XmNtopWidget, toggle,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNrightAttachment, XmATTACH_FORM,
				     XmNbottomAttachment, XmATTACH_FORM,
				     NULL);

  XtAddCallback(scroller, XmNtraverseObscuredCallback, traverseCB, NULL);

  outline = XtVaCreateManagedWidget("outline", xmOutlineWidgetClass,
				    scroller,
				    XmNindentation, 0,
				    XmNoutline, FALSE,
				    XmNnavigationType, XmTAB_GROUP,
				    XmNtranslations, 
				    XtParseTranslationTable(translations),
				    NULL);

  folder_image = CreateDefaultImage(folder_bits, 
				   folder_width, 
				   folder_height);
  XmInstallImage(folder_image, "folder_pxm");
  folder_minus_image = CreateDefaultImage(folder_minus_bits, 
					  folder_minus_width, 
					  folder_minus_height);
  XmInstallImage(folder_minus_image, "folder_minus_pxm");
  folder_plus_image = CreateDefaultImage(folder_plus_bits, 
					 folder_plus_width, 
					 folder_plus_height);
  XmInstallImage(folder_plus_image, "folder_plus_pxm");

  if (argc == 2)
    root = searchtree(argv[1]);
  if (argc == 1)
    root = searchtree(".");
  if (argc > 2) {
    fprintf(stderr, "usage: %s [directory]\n", argv[0]);
    exit(1);
  }
  
  buildTree (outline, root);
  XtRealizeWidget (shell);
  
  XtAppMainLoop(app);
}


void buildTree(Widget parent, 
	       struct folder *branch)
{

  Widget handle, button, label, outline;
  XmString labelname;
  Pixmap folder_pxm, folder_minus_pxm;
  Boolean collapsible = FALSE;
  Pixel foreground, background;
  
  if (!branch) 
    return;
  
  while (branch) {
    labelname = XmStringCreateSimple(branch->name);
    
    handle = XtVaCreateManagedWidget(branch->name, xmHandleWidgetClass,
				     parent,
				     XmNnavigationType, XmNONE,
				     XmNtranslations, 
				     XtParseTranslationTable(translations),
				     NULL);

    label = XtVaCreateManagedWidget("label", xmLabelWidgetClass,
				    handle,
				    XmNlabelType, XmPIXMAP,
				    NULL);
    
    XtVaGetValues(label, 
		  XmNforeground, &foreground, 
		  XmNbackground, &background, 
		  NULL);

    folder_pxm = XmGetPixmap(XtScreen(parent), 
			     "folder_pxm", foreground, background);
    folder_minus_pxm = XmGetPixmap(XtScreen(parent), 
				   "folder_minus_pxm", foreground, background);
    
    
    if (branch->sub)
      XtVaSetValues(label, XmNlabelPixmap, folder_minus_pxm, NULL);
    else 
      XtVaSetValues(label, XmNlabelPixmap, folder_pxm, NULL);
    
    button = XtVaCreateManagedWidget("button", 
				     xmPushButtonWidgetClass,
				     handle,
				     XmNlabelString, labelname,
				     XmNshadowThickness, 0,
				     XmNmultiClick, (branch->sub) ?
				     XmMULTICLICK_KEEP : XmMULTICLICK_DISCARD,
				     NULL);
    
    outline = XtVaCreateManagedWidget("outline", xmOutlineWidgetClass,
				      handle,
				      XmNindentation, 20,
				      XmNoutline, TRUE,
				      XmNnavigationType, XmNONE,
				      XmNtranslations, 
				      XtParseTranslationTable(translations),
				      NULL);

    XtVaSetValues(handle, XmNsubWidget, outline, NULL);
    
    XtAddCallback(button, 
		  XmNactivateCallback, PushButtonHandleCB, 
		  (XtPointer) label);

    XmStringFree(labelname);
    
    branch->w = handle;
    
    buildTree(outline, branch->sub);
    branch = branch->next;
  }
}


void toggleCB(Widget w,
			  XtPointer closure,
			  XtPointer call_data)
{
  XmToggleButtonCallbackStruct *tb = 
    (XmToggleButtonCallbackStruct *) call_data;

  if (tb->set) {
    printf("draw outlines... \n");
  }
}

void PushButtonHandleCB(Widget w,
			XtPointer closure,
			XtPointer call_data)
{
  XmPushButtonCallbackStruct *pb = 
    (XmPushButtonCallbackStruct *) call_data;

  Widget label = (Widget) closure, parent, outline;
  Pixmap folder_minus_pxm, folder_plus_pxm;
  Pixel  foreground, background;

  if (pb->click_count == 2) {
    XtVaGetValues(label, 
		  XmNforeground, &foreground, 
		  XmNbackground, &background, 
		  NULL);
  
    folder_minus_pxm = XmGetPixmap(XtScreen(label), "folder_minus_pxm", 
				   foreground, background);
    folder_plus_pxm = XmGetPixmap(XtScreen(label), "folder_plus_pxm", 
				  foreground, background);

    /* Lookup the parents outline widget. */

    parent = XtParent(w);
    
    XtVaGetValues(parent, XmNsubWidget, &outline, NULL);
    
    if ((outline != NULL) && XtIsManaged(outline)) {
      XtUnmanageChild(outline);
      XtVaSetValues(label, XmNlabelPixmap, folder_plus_pxm, NULL);
    } else if (outline != NULL) {
      XtManageChild(outline);
      XtVaSetValues(label, XmNlabelPixmap, folder_minus_pxm, NULL);
    }
  }
}

void traverseCB(Widget w,
	   XtPointer closure,
	   XtPointer call_data)
{
  XmTraverseObscuredCallbackStruct *tocs = 
    (XmTraverseObscuredCallbackStruct *) call_data;
  
  XmScrollVisible(w, tocs->traversal_destination, 2, 2);
}




