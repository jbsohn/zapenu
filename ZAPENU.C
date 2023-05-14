/*
 * FILE: z14_1.c
 *   DATE: 1/2/92
 *   AUTHOR: John Sohn
 *
 * Version 1.0
 * ===========
 *   initial version written between approx. 10/1/91 - 11/30/91
 *
 * Version 1.2
 * ===========
 *   added these items between 12/20/91 - 12/31/91 :
 *    -now checks the border size of current resolution and allows the
 *        dialog menu panel to be moved by clicking on its box
 *    -documents can now be put in menu and called from other entries
 *    -three resource files are now used for differences in resolution
 *
 * Version 1.4
 * ===========
 *    -new Icons
 *    -one Resource File with two ZAPenu definitions:
 *      one for low resololutions, one for high resolutions
 *
 */

#include <stdio.h>
#include <gemfast.h>
#include <osbind.h>
#include <zapenu_h.h>
#include <string.h>

#define void /* */
#define bool unsigned int

#define GEM 1
#define TOS 2
#define TTP 3
#define GTP 4
#define ZAP 5

#define HIDE_MOUSE graf_mouse ( 256, 0L )
#define SHOW_MOUSE graf_mouse ( 257, 0L )
#define BUSY_MOUSE graf_mouse ( BUSY_BEE, 0L )
#define AROW_MOUSE graf_mouse ( ARROW, 0L )
#define HAND_MOUSE graf_mouse ( FLAT_HAND, 0L )

/*-----------------------------------------------------------------*/
/* Redeclaration of functions :                                    */
bool get_ext();

/*-----------------------------------------------------------------*/
/* Integers to store the current resolutions x,y screen length     */
int xrez, yrez;
int maxColors;

/*-----------------------------------------------------------------*/
/* the GEM Global arrays                                           */
int contrl[12], intin[256], ptsin[256], intout[256], ptsout[256];

int handle;  /* the vdi workstation handle */

OBJECT *menu_panel;
OBJECT *file_info;
OBJECT *about;
OBJECT *blank_info;
OBJECT *cmd_dlg;
OBJECT *more_info;

/*-----------------------------------------------------------------*/
/* setup a structure to handle an entry in the menu :              */
typedef struct prg_type
{
     char filename[12];
     int space1;
     char name[20];
     int space2;
     char respond[35];
     int space3;
     char path[128];
     int gemtos;
} PRG_TYPE;

PRG_TYPE prg_info[38]; /* setup an array of 38 PRG_TYPES, approx.
                        * one for each item in the menu.  This is
                        * so each item in the menu in the resource file
                        * has a match in this array of structures.
                        */


FILE *fpsavedat;    /* file for saving the menu data */
FILE *fpreaddat;    /* file for reading the menu data */

char savepath[128]; /* the path the program was executed from, used
                     *   to make sure file data will be saved in the
                     *   directory that this program is executed from.
                     */

/*---------------------------------------------------------------*/
/* the dialog menu panel positions:                              */
int panel_x, panel_y, panel_w, panel_h;

main()
{
     int work_in[11], work_out[57];
     int i, cx, cy, cw, ch;
     char zap_dir[140];

     appl_init();               /* Set the system up for GEM calls */

     handle = graf_handle ( &cx, &cy, &cw, &ch );

     work_in[0] = Getrez() + 2; /* open the VDI workstation */
     for ( i = 1; i < 10; ++i )
          work_in[i] = 1;
     work_in[10] = 2;
     v_opnvwk ( work_in, &handle, work_out );

     xrez = work_out[0];  /* get current resolution x-length */
     yrez = work_out[1];  /* get current resolution y-length */
     maxColors = work_out[13]; /* Number of colors available on screen */

     prg_startup();

     do_cleanup( GEM );

     strcpy ( zap_dir, savepath );
     strcat ( zap_dir, "zapenu.zap" );

     readmenu_data( zap_dir );

     set_savepath();

     handle_menu();
}

set_savepath()
/* set the variable savepath the the current directory */
{
     char path[128];
     int drive;

     drive = Dgetdrv();

     Dgetpath ( path, drive+1 );

     savepath[0] = drive + 65;
     savepath[1] = ':';

     strcpy ( &savepath[2], path );
     strcat ( savepath, "\\" );
}

void object_to_background ( new_object );
OBJECT *new_object;
{
     int x,y,w,h;

     wind_get       ( 0, WF_WORKXYWH, &x, &y, &w, &h );
     objc_change    ( new_object, 0, 0, x, y, w, h, 0, TRUE );
     wind_set       ( 0, WF_NEWDESK, new_object, 0L ); 
}

void prg_startup()
/*
 *   If the resource file is available load it, otherwise
 *   show an alert box and exit.  Put the dialog boxes into their
 *   appropriate object structure.
 *
 *   Load the appropriate resource for the current resolution.
 */
{
     int icon_length_divisor;

     if ( xrez > 320 )
     {
          if ( !rsrc_load("ZAPENU_H.RSC") )
               error_exit();
     }
     else
     if ( xrez <= 319 )
     {
          if ( !rsrc_load("ZAPENU_L.RSC") )
               error_exit();
     }
     else
          form_alert( 1, "[1][|No resource for|this resolution!|][OK]");

     rsrc_gaddr ( 0, MENUH, &menu_panel );
     rsrc_gaddr ( 0, PIDLG, &file_info );
     rsrc_gaddr ( 0, ABOUTDLG, &about );
     rsrc_gaddr ( 0, BIDLG, &blank_info );
     rsrc_gaddr ( 0, CMDDLG, &cmd_dlg );
     rsrc_gaddr ( 0, MOREDLG, &more_info );

     icon_length_divisor = ( ((yrez+1) - 200) / 100 ) ;

     if (icon_length_divisor != 0) /* If y length > 200 */
         menu_panel[ICONBOX].ob_height =(menu_panel[ICONBOX].ob_height / 2);

     if ( maxColors <=2 )  /* If we are on a monochrome screen */
         menu_panel[MENUH].ob_spec = IP_1PTRN; /* No Fill for Menu panel */

     object_to_background ( menu_panel );
}

void error_exit()
/* exit if resource file is not available */
{
     form_alert ( 1, "[1][| Cannot load Resource! | ][ Abort ]" );
     appl_exit();
     Pterm (1);
}

void prg_exit()
/*
 *   Free up memory allocated by the resources, free up GEM and
 *   exit this application.
 */
{
     int alb;

     alb = form_alert ( 1, "[3][Exit this program?][OK|Cancel]" );
     if ( alb == 1 )
     {
          restore_screen ( xrez, yrez );
          v_clsvwk ( handle );

          rsrc_free();
          appl_exit();
          Pterm(0);
     }
     else
          menu_panel[EXITH].ob_state = NORMAL;
}

bool isselected ( item )
unsigned int item;
/*
 *   Check an object state from an object structure.  If the
 *   item specified is selected return TRUE.
 */
{
     if ( ( item & SELECTED ) == SELECTED )
          return ( TRUE );
          else
               return ( FALSE );
}

int show_dialog ( dlg )
OBJECT *dlg;
/*
 *   Draw a dialog box in the center of the screen and restore the
 *   the area of the screen that was written to.  Return the
 *   exit value of the dialog box.
 */
{
     int x, y, w, h;
     int exit;

     /* position the dialog box in the center of the screen */
     form_center( dlg, &x, &y, &w, &h );

     /* save the screen area  the dialog box will cover     */
     form_dial ( 0, 0, 0, 0, 0, x, y, w, h );

     /* display the dialog box                              */
     objc_draw( dlg, 0,10, x, y, w, h );

     /* allow the dialog box to be edited                   */
     exit = form_do ( dlg, 0 );

     /* restore the screen area covered by the dialog box   */
     form_dial ( 3, 0, 0, 0, 0, x, y, w, h );

     /* return the dialog box exit value                    */
     return ( exit );
}

void move_panel()
/*
 * Use the graf_dragbox function to move the panel on the screen.  When
 *   the dragbox function exits, get the new x,y locations and set
 *   panel_x and panel_y equal to their new location.
 */
{
     int new_x, new_y;
     int x,y,w,h;

     HAND_MOUSE;

     wind_get (0, WF_WORKXYWH, &x, &y, &w, &h );

     graf_dragbox ( panel_w, panel_h, panel_x, panel_y, x, y,
                    w, h, &new_x, &new_y );

     form_dial ( 3, 0, 0, 0, 0, panel_x, panel_y, panel_w, panel_h );

     panel_x = new_x;
     panel_y = new_y;
     menu_panel[MENUH].ob_x = new_x;
     menu_panel[MENUH].ob_y = new_y;

     AROW_MOUSE;
}

int show_panel()
/*
 * Displays the dialog box and object menu_panel.  Same as show_dialog
 * except that the menu panel should not be centered and cleared after
 * it is exited from.
 */
{
     unsigned int off = 0, exit;

     intin[0] = off;

     showm();

     exit = formZAP_do ( menu_panel, 0 );

     return ( exit );
}

int get_file( tmp_file, tmp_path, text )
char *tmp_file, *tmp_path, *text;
/*
 *   Recieve the file to be selected and the text that is
 *   to be displayed in the file selector box.  Setup a
 *   path and call the fsel_exinput with it.  Return the value
 *   of the file selector exit button.
 */
{
     int drive,butn, index, x;
     char fs_path[128], fs_file[13];

     drive = Dgetdrv();
     fs_path[0] = drive + 65;
     fs_path[1] = ':';

     strcpy ( &fs_path[2], "\\*.*" );
     strcpy ( fs_file,"\0" );

     fsel_exinput ( fs_path, fs_file, &butn, text );

     x = strlen ( fs_path );   /* Remove "\*.EXT" from fs_path */
     index = x - 1;
     while ( fs_path[index] != '\\' && fs_path[index] != ':' && index > 0 )
          --index;
     strcpy ( &fs_path[index+1], "\0" );

     if ( butn )
     {
          strcpy ( tmp_file, fs_file );
          strcpy ( tmp_path, fs_path );
          strcat ( tmp_file, "\0" );
          strcat ( tmp_path, "\0" );
     }

     return ( butn );
}


void save_screen ( xlength, ylength )
int xlength, ylength;
/*
 *   Save an area of the screen about to be covered with
 *   xlength and ylength as the x and y lengths of the screen
 *   respectively.
 */
{
     form_dial ( 0, 0, 0, 0, 0, 0, 0, xlength, ylength );
}


void restore_screen ( xlength, ylength )
int xlength, ylength;
/*
 *   Restore the screen that was previously covered up with
 *   save screen.  Xlength and ylength correspond to the screen
 *   x and y screen position length.
 */
{
     form_dial ( 3, 0, 0, 0, 0, 0, 0, xlength, ylength );
}


long run_prog ( cmd, args )
char *cmd,          /* filespec: path,filename.ext */
     *args;         /* command line call to file   */
/*
 *   Recieve the filename to execute and the command line
 *   arguments.  After execution the function return the exit
 *   status of the called program.
 */
{
     char buf[128];
     int  arglen;
     arglen = strlen (args);
     if ( arglen > 126 )
     {
          return -1;
     }
     strcpy ( buf + 1, args );     /* copy args to second byte */
     buf[0] = arglen;
     return ( Pexec ( 0, cmd, buf, 0l ) );
}

void check_ext ( file, ext )
char *file;
char *ext;
{
     int pos;

     if ( stristr ( file, ext ) == NULL )
     {
          pos = strrpos ( file, '.' );

          if ( pos == -1 )
          {
               strcat ( file, ext );
          }
          else
               strcpy ( file + pos, ext );
     }
}

void readmenu_data( menuname )
char *menuname;
/* read in a menu defintion.  This function does not assume an ansified
 * version of C, fgets assumes the Dlibs version which does not follow
 * the ANSI standard.  When compiling under ANSI C this function will
 * need to be changed.
 */
{
     int i;

     if ( ( fpreaddat = fopen ( menuname, "r" ) ) != NULL )

     {
          for ( i = 0; i <= 37 ; i++ )
          {
               fscanf ( fpreaddat, "%d", &prg_info[i].gemtos );
               fgets ( prg_info[i].filename, 20, fpreaddat );
               fgets ( prg_info[i].name, 25, fpreaddat );
               fgets ( prg_info[i].path, 130, fpreaddat );
               fgets ( prg_info[i].respond, 40, fpreaddat );

               strrpl ( prg_info[i].name, "\n", "\0", -1 );
               strrpl ( prg_info[i].filename, "\n", "\0", -1 );
               strrpl ( prg_info[i].path, "\n", "\0", -1 );
               strrpl ( prg_info[i].respond, "\n", "\0", -1 );
               if ( ( strlen ( prg_info[i].name ) ) > 1 )
                    putinto_panel ( i, prg_info[i].name );
          }

          fclose ( fpreaddat );
     }
}

void handle_menu()
/*
 * Handles all actions and responses in the menu panel.
 *
 * The resource files must all match the High resolution resource file
 * when compiling.  Resources should be sorted before compiling if they
 * have been changed around.
 */
{
     unsigned int item;
     int i;
     char file_cmd[140];

     menu_panel[EXECUTEH].ob_state = SELECTED;

     while ( TRUE )
     {
          item = show_panel();

/*
          if ( item == MENUH )
               move_panel();
          else
 */

          if ( isselected ( menu_panel[EXITH].ob_state ) )
               prg_exit();
          else
          if ( isselected ( menu_panel[ABOUTH].ob_state ) )
               do_about();
          else
          if ( isselected ( menu_panel[ADDH].ob_state ) )
               add_item( item );
          else
          if ( isselected ( menu_panel[REMOVEH].ob_state ))
               remove_item( item );
          else
          if ( isselected ( menu_panel[PIH].ob_state ) )
               get_info( item );
          else
          if ( isselected ( menu_panel[SAVEH].ob_state ) )
          {
               save_menu();
               menu_panel[SAVEH].ob_state = NORMAL;
          }
          else
          if ( isselected ( menu_panel[SAVEASH].ob_state ) )
          {
               savemenu_as();
               menu_panel[SAVEASH].ob_state = NORMAL;
          }
          else
          if ( isselected ( menu_panel[FSH].ob_state ) )
               do_fs();
          else
          if ( isselected ( menu_panel[NEWMH].ob_state  ) )
          {
               new_menu();
               menu_panel[NEWMH].ob_state = NORMAL;
          }
          else
          if ( isselected ( menu_panel[EXECUTEH].ob_state ) )
          {
               if  ( prg_info[item].gemtos )
                    exec_program( item );
               else
               {
                    i = search_respond ( prg_info[item].filename );
                    if ( i > 0 )
                    {
                         strcpy ( file_cmd, prg_info[item].path );
                         strcat ( file_cmd, prg_info[item].filename );
                         execfrom_fs( i, file_cmd );
                    }
               }
          }
     }
}

void add_item( item )
int item;
/* Handling for Adding an item to the menu panel. */
{
     char add_file[12], add_path[128];

     if ( get_file ( add_file, add_path, "ADD PROGRAM TO MENU" ) )
     {
        if (add_file[0] != '\0')
        {
          erase_item ( item );
          strcpy ( prg_info[item].filename, add_file );
          strcpy ( prg_info[item].path, add_path );
          strcpy ( prg_info[item].name, add_file );
          putinto_panel( item, prg_info[item].filename );
          set_prgtype ( item );
        }
     }
}

void set_prgtype( item )
int item;
{
     if ( stristr ( prg_info[item].filename, ".PRG" ) != NULL )
          prg_info[item].gemtos = GEM;
     else
          if ( stristr ( prg_info[item].filename, ".APP" ) != NULL )
               prg_info[item].gemtos = GEM;
     else
          if ( stristr ( prg_info[item].filename, ".TTP" ) != NULL )
               prg_info[item].gemtos = TTP;
     else
          if ( stristr ( prg_info[item].filename, ".TOS" ) != NULL )
               prg_info[item].gemtos = TOS;
     else
          if ( stristr ( prg_info[item].filename, ".ZAP" ) != NULL )
               prg_info[item].gemtos = ZAP;
     else
          if ( stristr ( prg_info[item].filename, ".GTP" ) != NULL )
               prg_info[item].gemtos = GTP;
     else
          prg_info[item].gemtos = 0;
}

void putinto_panel( item, text )
int item;
char *text;
{
     rsc_sstring ( menu_panel, item, text, -1 );
}

void remove_item( item )
int item;
{
     int alb;

     alb = form_alert ( 1, "[3][Remove this item|from the menu?][OK|Cancel]" );
     if ( alb == 1 )
     {
          erase_item( item );
          putinto_panel ( item, " " );
     }
}

void erase_item( item )
int item;
{
     strcpy ( prg_info[item].name, "\0" );
     strcpy ( prg_info[item].filename, "\0" );
     strcpy ( prg_info[item].path, "\0" );
     strcpy ( prg_info[item].respond, "\0" );
     prg_info[item].gemtos = 0;
}

void get_info( item )
int item;
{
     if ( ( prg_info[item].gemtos == 0 ) || ( prg_info[item].gemtos == ZAP ) )
          bslot_info( item );
     else
          slot_info ( item );

     file_info[PIOK].ob_state = NORMAL;
     file_info[PICANCEL].ob_state = NORMAL;
     file_info[PRGGEM].ob_state = NORMAL;
     file_info[PRGTOS].ob_state = NORMAL;
     file_info[PRGTTP].ob_state = NORMAL;
     file_info[PRGGTP].ob_state = NORMAL;
     blank_info[BOK].ob_state = NORMAL;
     blank_info[BCANCEL].ob_state = NORMAL;

     putinto_panel ( item, prg_info[item].name );
}

void slot_info( item )
int item;
{
     char name[20];
     char filename[12];
     char respond[35];

     strcpy ( filename, prg_info[item].filename );
     strcpy ( respond, prg_info[item].respond );
     strcpy ( name, prg_info[item].name );

     rsc_sstring ( file_info, PRGNAME, name, -1 );
     rsc_sstring ( file_info, PRGFILE, filename, -1 );
     rsc_sstring ( file_info, RESPOND2, respond, -1 );

     switch ( prg_info[item].gemtos )
     {
          case GEM:
               file_info[PRGGEM].ob_state = SELECTED;
               break;
          case TOS:
               file_info[PRGTOS].ob_state = SELECTED;
               break;
          case GTP:
               file_info[PRGGTP].ob_state = SELECTED;
               break;
          case TTP:
               file_info[PRGTTP].ob_state = SELECTED;
               break;
     }

     show_dialog ( file_info );

     if ( isselected ( file_info[PIOK].ob_state ) )
     {
          strcpy ( prg_info[item].name, name );
          strcpy ( prg_info[item].respond, respond );

          if ( isselected ( file_info[PRGGEM].ob_state ) )
               prg_info[item].gemtos = GEM;
          else
          if ( isselected ( file_info[PRGTOS].ob_state ) )
               prg_info[item].gemtos = TOS;
          else
          if ( isselected ( file_info[PRGGTP].ob_state ) )
               prg_info[item].gemtos = GTP;
          else
          if ( isselected ( file_info[PRGTTP].ob_state ) )
               prg_info[item].gemtos = TTP;
     }
}

void bslot_info( item )
int item;
{
     char name[20];

     strcpy ( name, prg_info[item].name );

     rsc_sstring ( blank_info, BNAME, name, -1 );
     rsc_sstring ( blank_info, ZAPTEXT, prg_info[item].filename, -1 );

     show_dialog ( blank_info );

     if ( isselected ( blank_info[BOK].ob_state ) )
          strcpy ( prg_info[item].name, name );
}

void writemenu_data( msave )
char *msave;
{
     long i;

     BUSY_MOUSE;

     fpsavedat = fopen ( msave, "w" );

     for ( i = 0; i <= 37; i++ )
     {
          fprintf ( fpsavedat, "%d", prg_info[i].gemtos );
          fprintf ( fpsavedat, "%s\n", prg_info[i].filename );
          fprintf ( fpsavedat, "%s\n", prg_info[i].name );
          fprintf ( fpsavedat, "%s\n", prg_info[i].path );
          fprintf ( fpsavedat, "%s\n", prg_info[i].respond );
     }
     fclose ( fpsavedat );

     menu_panel[SAVEH].ob_state = NORMAL;

     AROW_MOUSE;
}

int exec_program( item )
int item;
{
     char cmdl[50];

     get_cmdl( cmdl, item );

     setup_exec ( prg_info[item].path );

     restore_screen ( xrez, yrez );

     switch ( prg_info[item].gemtos )
     {
          case GEM:
          case GTP:
               BUSY_MOUSE;
               break;
          case TOS:
          case TTP:
               HIDE_MOUSE;
               v_clrwk( handle );
               break;
          case ZAP:
               readmenu_data ( prg_info[item].filename );
               return ( TRUE );
               break;
     }

     run_prog ( prg_info[item].filename, cmdl );
     clear_wind();
     do_cleanup( prg_info[item].gemtos );
}

void get_cmdl( cmdl, item )
char *cmdl;
int item;
{
     char cmd[40];

     strcpy ( cmdl,"\0" );
     strcpy ( cmd,"\0" );
 
     switch ( prg_info[item].gemtos )
     {
          case TTP:
          case GTP:
               rsc_sstring ( cmd_dlg, CMDINF, cmd, -1 );
               show_dialog ( cmd_dlg );
               if ( isselected ( cmd_dlg[CMDOK].ob_state ) )
                    strcpy ( cmdl, cmd );
               cmd_dlg[CMDOK].ob_state = NORMAL;
               break;
     }
}

void do_cleanup( type )
int type;
{
     switch ( type )
     {
          case GEM:
          case GTP:
               HIDE_MOUSE;
     }

     v_clrwk ( handle );

     restore_screen ( xrez, yrez );

     AROW_MOUSE;
     SHOW_MOUSE;

     showm();
}

void setup_exec ( path )
char *path;
{
     int drive;

     drive = path[0];
     drive -= 65;
     Dsetdrv ( drive );
     Dsetpath ( &path[2] );

}

void do_fs()
{
     char file[12];
     int butn;
     char path[128];
     int i;
     char filename[140];

     butn = ( get_old ( file, path ) );

     if ( butn )
     {
          i = search_respond ( file );

          if ( i > 0 )
          {
               strcpy ( filename, path );
               strcat ( filename, file );
               execfrom_fs( i, filename );
          }
          else
          {
               strcpy ( prg_info[1].path, path );
               strcpy ( prg_info[1].filename, file );
               set_prgtype ( 1 );
               if ( prg_info[1].gemtos )
                    exec_program ( 1 );
          }
     }

     menu_panel[FSH].ob_state = NORMAL;
}

int get_old( tmp_file, tmp_path )
char *tmp_file, *tmp_path;
/*
 *   Recieve the file to be selected and the text that is
 *   to be displayed in the file selector box.  Setup a
 *   path and call the fsel_exinput with it.  Return the value
 *   of the file selector exit button.
 */
{
     int drive,butn;
     char fs_path[128];
     int index, x;
     char fs_file[12];

     drive = Dgetdrv();
     fs_path[0] = drive + 65;
     fs_path[1] = ':';

     strcpy ( &fs_path[2], "\\*.*" );
     strcpy ( fs_file, "\0" );

     fsel_input ( fs_path, fs_file, &butn );

     x = strlen ( fs_path );   /* Remove "\*.EXT" from fs_path */
     index = x - 1;
     while ( fs_path[index] != '\\' && fs_path[index] != ':' && index > 0 )
          --index;
     strcpy ( &fs_path[index+1], "\0" );

     if ( butn )
     {
          strcpy ( tmp_file, fs_file );
          strcpy ( tmp_path, fs_path );
          strcat ( tmp_file, "\0" );
          strcat ( tmp_path, "\0" );
     }

     return ( butn );
}

int search_respond( file )
char *file;
{
     int i;
     char *ext;

     if ( ( ext = strchr ( file, '.' ) ) != NULL )
     {
          for ( i = 0; i <= 37; i++ )
               if ( stristr ( prg_info[i].respond, ext ) )
                    return ( i );
     }

     return ( -1 );
}

void execfrom_fs ( item, cmd_file )
int item;
char *cmd_file;
{
     setup_exec ( prg_info[item].path );

     restore_screen ( xrez, yrez );

     switch ( prg_info[item].gemtos )
     {
          case GEM:
          case GTP:
               BUSY_MOUSE;
               break;
          case TOS:
          case TTP:
               HIDE_MOUSE;
               v_clrwk( handle );
               break;
     }

     run_prog ( prg_info[item].filename, cmd_file );
     clear_wind();
     do_cleanup( prg_info[item].gemtos );
}

void clear_wind()
{
     wind_set( 0, WF_NEWDESK, 0L, 0L, 0L, 0L );
}

void new_menu()
{
     int alb;

     alb = form_alert ( 1, "[3][Define new menu?][OK|Cancel]" );
     if ( alb == 1 )
     {
          erase_panel();
          strcpy ( prg_info[2].path, savepath );
          strcpy ( prg_info[2].name, "main menu" );
          strcpy ( prg_info[2].filename, "ZAPENU.ZAP" );
          strcpy ( prg_info[2].respond, "\0" );
          prg_info[2].gemtos = ZAP;
          putinto_panel ( 2, prg_info[2].name );
     }
}

void erase_panel()
{
        int i;

        for ( i = 0; i <= 37; i++ )
                erase_item(i);
}

void save_menu()
{
     char zapfile[140];
     int alb;
     alb = form_alert ( 1, "[3][Save startup menu?][OK|Cancel]" );
     if ( alb == 1 )
     {
          strcpy ( zapfile, savepath );
          strcat ( zapfile, "zapenu.zap" );
          writemenu_data ( zapfile );
     }
}

void savemenu_as()
{
     char g_file[12];
     char zapfile[140];
     char g_path[128];

     if ( get_file ( g_file, g_path, "SAVE MENU SETTINGS" ) )
     {
          check_ext ( g_file, ".ZAP" );
          strcpy ( zapfile, g_path );
          strcat ( zapfile, g_file );
          writemenu_data ( zapfile );
     }
}

do_about()
{
     show_dialog ( about );

     if ( isselected ( about[ABINFO].ob_state ) )
          show_dialog ( more_info );

                                   /*   Restore the exit buttons */
     menu_panel[ABOUTH].ob_state = NORMAL;
     about[ABOK].ob_state = NORMAL;
     about[ABINFO].ob_state = NORMAL;
     more_info[MOK].ob_state = NORMAL;
}

int formZAP_do (tree,start)
OBJECT *tree;
int start;
{
     int     selobj;
     XMULTI  xm;
     short next=start;
     short which,cont;
     short x,y,kr,br,junk;
     short edit,idx;

     edit=0;
     cont=1;

     xm.mflags    = MU_MESAG | MU_BUTTON | MU_KEYBD;
     xm.mbmask    = 1;
     xm.mbclicks  = 2;
     xm.mbstate   = 1;

     while ( cont )
     {
          if(next!=0 && edit!=next)
          {
               edit = next;
               next = 0;
               objc_edit(tree,edit,0,&idx,ED_INIT);
          }

          evnx_multi(&xm);

          if (xm.mwhich & MU_MESAG )
          {
               check_message ( xm.msgbuf );
          }

          if (xm.mwhich & MU_BUTTON) 
          {
               next=objc_find(tree,ROOT,MAX_DEPTH,xm.mmox,xm.mmoy);

               if ( next == -1 )
               {
                    Bconout(2,7);
                    next=0;
               }
               else
                    cont = form_button(tree,next,xm.mbreturn,&next);
          }

          if(!cont || (next!=0 && next!=edit))
          {
               objc_edit(tree,edit,0,&idx,ED_END);
               exit = handle_menu ( next );
          }

     }
     
     return (int) next;
}

void check_message ( msgbuf )
register int *msgbuf;
/*
 *   Check the menu for a selected item.  If the item is selected
 *   act accordingly.
 */
{
     switch ( msgbuf[0] )
     {

          case MN_SELECTED:
               switch ( msgbuf[4] )
               {
                    case ABOUT:
                         do_about();
                         break; 
                    case QUIT:
                         prg_exit();
                         break;
               }

          case WM_REDRAW:
               restore_screen ( msgbuf[6], msgbuf[7] );
               break;
     }
     menu_tnormal ( menu, msgbuf[3], TRUE );
}
 

