#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <pwd.h> /* getpwdid */
#include <dirent.h>
#include <assert.h>
#include "highgui.h"
#include "cv.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include "pam_face_defines.h"
#include <sys/time.h>
#include <time.h>

extern void  featureDctMod2(IplImage * img,float*  features_final);
extern void  featureLBPHist(IplImage * img,float*  features_final);
extern void allocateMemory();
extern void intializeGtkIconView();
int numberOfFaces=0;
extern void intialize();
extern int faceDetect(IplImage*,CvPoint *,CvPoint *);
extern int  preprocess(IplImage*,CvPoint ,CvPoint ,IplImage*);
void on_gtkSave_clicked  (GtkButton *button,gpointer user_data);
IplImage *frame,*frameNew, *frame_copy = 0;
CvPoint pLeftEye;
CvPoint pRightEye;
CvCapture* capture;

enum
{
    COL_DISPLAY_NAME,
    COL_PIXBUF,
    NUM_COLS
};

GtkBuilder *builder;
GtkWidget *window;
GtkWidget *gtkWelcome;
GtkWidget *gtkIconView;
GtkWidget *gtkWebcamImage;
GtkWidget *gtkCountFace;
void loadCVPIXBUF(GtkWidget *imgCapturedFace,IplImage* image)
{
    unsigned char *gdataUserFace;
    GdkPixbuf *pixbufUserFace= NULL;

    pixbufUserFace = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,8,image->width,image->height);
    gdataUserFace = gdk_pixbuf_get_pixels(pixbufUserFace);

    int m,n;

    for (n=0;n<image->height;n++)
    {
        for (m= 0;m<image->width;m++)
        {
            CvScalar s;
            s=cvGet2D(image,n,m);
            gdataUserFace[n*image->width*3 + m*3 +0]=(uchar)s.val[2];
            gdataUserFace[n*image->width*3 + m*3 +1]=(uchar)s.val[1];
            gdataUserFace[n*image->width*3 + m*3 +2]=(uchar)s.val[0];
        }
    }

    gtk_image_set_from_pixbuf((GtkImage*)imgCapturedFace, pixbufUserFace);
    g_object_unref (pixbufUserFace);
}
void
on_window_destroy (GtkObject *object, gpointer user_data)
{
    gtk_main_quit();
}

GtkTreeModel *
readFilesAndLoadGtkIconView()
{

    GtkListStore *list_store;
    GdkPixbuf *p1;
    GtkTreeIter iter;
    GError *err = NULL;
    struct dirent *de=NULL;
    DIR *d=NULL;
    char dirpath[120];
    char fullimagepath[150];
    struct passwd *passwd;
    passwd = getpwuid (getuid());
    int status;
    sprintf(dirpath,"%s/.pam-face-authentication", passwd->pw_dir);
    status = mkdir(dirpath, S_IRWXU );
    sprintf(dirpath,"%s/.pam-face-authentication/train", passwd->pw_dir);
    status = mkdir(dirpath, S_IRWXU );
    sprintf(dirpath,"%s/.pam-face-authentication/train/", passwd->pw_dir);
    list_store = gtk_list_store_new(NUM_COLS,G_TYPE_STRING, GDK_TYPE_PIXBUF);
    d=opendir(dirpath);
    while (de = readdir(d))
    {
        if (strcmp(de->d_name+strlen(de->d_name)-3, "pgm")==0)
        {
            sprintf(fullimagepath,"%s/.pam-face-authentication/train/%s", passwd->pw_dir,de->d_name);
            // printf("%s",fullimagepath);
            p1 =  gdk_pixbuf_new_from_file_at_size  (fullimagepath,60,80,&err);

            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter, COL_DISPLAY_NAME,de->d_name, COL_PIXBUF, p1, -1);
        }
    }
    closedir(d);
    return GTK_TREE_MODEL(list_store);
}


void
view_popup_menu_onDelete (GtkWidget *menuitem, gpointer userdata)
{

    char imagepath[150];
    struct passwd *passwd;
    passwd = getpwuid (getuid());
    sprintf(imagepath,"%s/.pam-face-authentication/train/%s", passwd->pw_dir,userdata);
    remove(imagepath);
    intializeGtkIconView();

}



void
view_popup_menu (GtkWidget *iconview,GdkEventButton *event, char * filename)
{

    GtkWidget *menu, *menuitem;
    menu = gtk_menu_new();
    menuitem = gtk_menu_item_new_with_label("Delete Face");
    g_signal_connect(menuitem, "activate",(GCallback) view_popup_menu_onDelete, filename);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,(event != NULL) ? event->button : 0,gdk_event_get_time((GdkEvent*)event));

}



gboolean
view_onButtonPressed (GtkWidget *iconview, GdkEventButton *event, gpointer userdata)
{  gtk_icon_view_unselect_all ((GtkIconView*)iconview);
    GtkTreePath *path;
    GtkTreePath *cell;
    if (gtk_icon_view_get_item_at_pos((GtkIconView*)iconview,(gint) event->x,event->y,(GtkTreePath**)&path,(GtkCellRenderer**)&cell))
    {
        gtk_icon_view_set_cursor((GtkIconView*)iconview,(GtkTreePath*)path,(GtkCellRenderer*)cell,FALSE);

        GtkTreeModel *model;
        GtkTreeIter   iter;
        char         *file;
        model = gtk_icon_view_get_model (GTK_ICON_VIEW (iconview));
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_model_get (model, &iter, COL_DISPLAY_NAME, &file, -1);
        // printf("%s \n",file);
        view_popup_menu(iconview,event,file);
    }
return FALSE;
}
void intializeGtkIconView()
{
    gtk_icon_view_set_model (GTK_ICON_VIEW(gtkIconView),readFilesAndLoadGtkIconView());
    // gtk_icon_view_set_text_column(GTK_ICON_VIEW(gtkIconView),COL_DISPLAY_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(gtkIconView),COL_PIXBUF);

}


static gboolean time_handler(GtkWidget *widget)
{
    ;
    if (widget->window == NULL) return FALSE;
    if ( !cvGrabFrame( capture ))
        return FALSE;

    // printf("aaa \n");
    frame = cvRetrieveFrame( capture );
    if ( !frame )
        return FALSE;
    if ( !frame_copy )
    {
        frame_copy = cvCreateImage( cvSize(frame->width,frame->height),IPL_DEPTH_8U, frame->nChannels );
        frameNew = cvCreateImage( cvSize(frame->width,frame->height),IPL_DEPTH_8U, frame->nChannels );
    }
    if ( frame->origin == IPL_ORIGIN_TL )
        cvCopy( frame, frame_copy, 0 );
    else
        cvFlip( frame, frame_copy, 0 );

    cvCopy( frame_copy, frameNew, 0 );
    //printf("aaa 1\n");
    allocateMemory();
    int k= faceDetect(frame_copy,&pLeftEye,&pRightEye);
    if (k==1 && numberOfFaces>0)
    {
        numberOfFaces--;
        IplImage *face;
        face = cvCreateImage( cvSize(120,140),8,1);
        int j= preprocess(frameNew,pLeftEye,pRightEye,face);

        char buffer[30];
        struct timeval tv;
        time_t curtime;
        gettimeofday(&tv, NULL);
        curtime=tv.tv_sec;
        strftime(buffer,30,"%m%d%y%H%M%S",localtime(&curtime));

        char imagepath[150];
        struct passwd *passwd;
        passwd = getpwuid (getuid());
        sprintf(imagepath,"%s/.pam-face-authentication/train/%s%d.pgm", passwd->pw_dir,buffer,numberOfFaces+1);
        // printf("%s \n",imagepath);
        cvSaveImage(imagepath,face);
        intializeGtkIconView();

    }

    loadCVPIXBUF(gtkWebcamImage,frame_copy);




    if ( cvWaitKey( 4 ) >= 0 )
        {}

    return TRUE;
}
void
on_gtkAbout_clicked  (GtkButton *button,gpointer user_data)
{

    static const char *authors[] =
    {
        "Rohan Anil <rohan.anil@gmail.com>",
        "Birla Institute of Technology & Science Pilani - Goa Campus",
        " ",
        "Alex Lau <avengermojo@gmail.com>",
        "Novell, China",
        " ",
        "Nickolay V. Shmyrev <nshmyrev@yandex.ru>",
        "GNOME",

        NULL
    };

    static const char *artists[] = {"Anyone Interested? :) ",
                                    NULL
                                   };

    static const char *documenters[] =
    {
        "Anyone Interested? :) ",
        NULL
    };

    const char *translators;

    translators = "translator-credits";

    const char *license[] =
    {
        "This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 3 of the License, or "
        "(at your option) any later version.\n",
        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
        "GNU General Public License for more details.\n",
        "You should have received a copy of the GNU General Public License "
        "along with this program. If not, see <http://www.gnu.org/licenses/>."
    };

    char *license_trans;

    license_trans = g_strconcat (license[0], "\n", license[1], "\n", license[2], "\n", NULL);

    gtk_show_about_dialog (GTK_WINDOW (window),
                           "version", VERSION,
                           "comments", "Pluggable Face Authentication module for your favorite distro.",
                           "authors", authors,
                           "translator-credits", translators,
                           "artists", artists,
                           "documenters", documenters,
                           "website", "http://code.google.com/p/pam-face-authentication/",
                           "website-label", "pam-face-authentication",
                           "logo-icon-name", "pam-face-authentication",
                           "wrap-license", TRUE,
                           "license", license_trans,
                           NULL);

    g_free (license_trans);
}
void
on_gtkSave_clicked  (GtkButton *button,gpointer user_data)
{
    struct dirent *de=NULL;
    DIR *d=NULL;
    char dirpath[120];
    char temp[200];
    char featuresDCTpath[120];
    char featuresLBPpath[120];

    char fullimagepath[150];
    struct passwd *passwd;
    passwd = getpwuid (getuid());
    int status;
    sprintf(dirpath,"%s/.pam-face-authentication/features", passwd->pw_dir);
    status = mkdir(dirpath, S_IRWXU );
    sprintf(featuresDCTpath,"%s/.pam-face-authentication/features/featuresDCT", passwd->pw_dir);
    sprintf(featuresLBPpath,"%s/.pam-face-authentication/features/featuresLBP", passwd->pw_dir);


    FILE *f1 =fopen(featuresDCTpath,"w");
    FILE *f2 =fopen(featuresLBPpath,"w");

    sprintf(dirpath,"%s/.pam-face-authentication/train/", passwd->pw_dir);

    d=opendir(dirpath);
    while (de = readdir(d))
    {
        if (strcmp(de->d_name+strlen(de->d_name)-3, "pgm")==0)
        {
            sprintf(fullimagepath,"%s/.pam-face-authentication/train/%s", passwd->pw_dir,de->d_name);
            IplImage * img = cvLoadImage(fullimagepath,0);
            int Nx = floor((img->width - 4)/4);
            int Ny= floor((img->height - 4)/4);
            float* features = (float*)malloc(  Nx*Ny * 18 * sizeof(float) );


            featureDctMod2(img,features);

            int Nx1 = floor((img->width - 10)/10);
            int Ny1= floor((img->height - 10)/10);
            float* featuresLBP = (float*)malloc(  Nx1*Ny1 * 59 * sizeof(float) );
            featureLBPHist(img,featuresLBP);

            int i,j,k=0;
            sprintf(temp,"%d",getuid());
            fputs(temp,f1);
            fputs(" ",f1);
            fputs(temp,f2);
            fputs(" ",f2);
            for (i=0;i<Ny1;i++)
            {
                for (j=0;j<Nx1;j++)
                {
                    for (k=0;k<59;k++)
                    {
                        sprintf(temp,"%d", (i*59*Nx1 + j*59 +k));
                        fputs(temp,f2);
                        fputs(":",f2);
                        sprintf(temp,"%f",featuresLBP[i*59*Nx1 + j*59 +k]);
                        fputs(temp,f2);
                        fputs(" ",f2);
                    }
                }
            }

            for (i=0;i<Ny;i++)
            {
                for (j=0;j<Nx;j++)
                {
                    for (k=0;k<6;k++)
                    {
                        sprintf(temp,"%d", (i*18*Nx + j*18 +k));
                        fputs(temp,f1);
                        fputs(":",f1);
                        sprintf(temp,"%f",features[i*18*Nx + j*18 +k]);
                        fputs(temp,f1);
                        fputs(" ",f1);
                    }

                }
            }
            fputs("\n",f1);
            fputs("\n",f2);


        }
    }


    fclose(f1);
    fclose(f2);

/*
Append feature directory to /etc/pam-face-authentication/db.lst
*/

FILE *fileKey;
char path[250];
int ifExist=0;
sprintf(dirpath,"%s/.pam-face-authentication/features", passwd->pw_dir);

    if ( !(fileKey = fopen("/etc/pam-face-authentication/db.lst", "r")) )
    {
        fprintf(stderr, "Error 1 Occurred Accessing db.lst\n");
        exit(0);
    }
 while (fscanf(fileKey,"%s", path)!=EOF )
    {
        if (strcmp(path,dirpath)==0)
        {
        ifExist=1;
        }
    }
    fclose(fileKey);
if(ifExist==0)
{
    char temptrainer[300];
    sprintf(temptrainer,"%s/pfatrainer %s",BINDIR,dirpath);
    system(temptrainer);
}
else
system(BINDIR "/pfatrainer");

/* NEED PROGRESS BAR */

}
/*

*/
void
gtkCaptureFace_clicked_cb  (GtkButton *button,gpointer user_data)
{
    double val;
    val=gtk_spin_button_get_value ((GtkSpinButton*)gtkCountFace);
    numberOfFaces=(int)val;
    //ŗ  printf("%d \n",numberOfFaces);

}
int
main (int argc, char *argv[])
{

    intialize();
    capture = cvCaptureFromCAM(0);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH,IMAGE_WIDTH);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT,IMAGE_HEIGHT);

    char welcomeMessage[100];
    struct passwd *passwd;
    passwd = getpwuid ( getuid());
    sprintf(welcomeMessage,"Welcome %s!", passwd->pw_gecos);
    gtk_init (&argc, &argv);
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, PKGDATADIR "/gtk-facemanagerU.xml", NULL);
    window = GTK_WIDGET (gtk_builder_get_object (builder, "gtk-facemanager"));
    gtkWelcome= (GTK_WIDGET (gtk_builder_get_object (builder, "gtkWelcome")));
    gtkIconView=(GTK_WIDGET (gtk_builder_get_object (builder, "gtkIconView")));
    gtkWebcamImage = GTK_WIDGET (gtk_builder_get_object (builder, "gtkWebcamImage"));
    gtkCountFace = (GTK_WIDGET (gtk_builder_get_object (builder, "gtkCountFace")));

    intializeGtkIconView();
    gtk_label_set_label (GTK_LABEL(gtkWelcome),welcomeMessage);

    g_timeout_add(40, (GSourceFunc) time_handler, (gpointer) window);
    gtk_builder_connect_signals (builder, NULL);
    g_object_unref (G_OBJECT (builder));
    gtk_widget_show (window);
    gtk_main ();

    return 0;
}