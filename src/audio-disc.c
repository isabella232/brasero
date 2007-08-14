/***************************************************************************
 *            audio-disc.c
 *
 *  dim nov 27 15:34:32 2005
 *  Copyright  2005  Rouquier Philippe
 *  brasero-app@wanadoo.fr
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <time.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include <gdk/gdkkeysyms.h>

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrenderer.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeviewcolumn.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkuimanager.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>

#include <libgnomeui/libgnomeui.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-file-info.h>

#include "burn-basics.h"
#include "brasero-disc.h"
#include "brasero-audio-disc.h"
#include "metadata.h"
#include "utils.h"
#include "song-properties.h"
#include "brasero-vfs.h"
#include "eggtreemultidnd.h"

#ifdef BUILD_INOTIFY
#include "inotify.h"
#include "inotify-syscalls.h"
#endif

static void brasero_audio_disc_class_init (BraseroAudioDiscClass *klass);
static void brasero_audio_disc_init (BraseroAudioDisc *sp);
static void brasero_audio_disc_finalize (GObject *object);
static void brasero_audio_disc_iface_disc_init (BraseroDiscIface *iface);

static void brasero_audio_disc_get_property (GObject * object,
					     guint prop_id,
					     GValue * value,
					     GParamSpec * pspec);
static void brasero_audio_disc_set_property (GObject * object,
					     guint prop_id,
					     const GValue * value,
					     GParamSpec * spec);

static BraseroDiscResult
brasero_audio_disc_get_status (BraseroDisc *disc);

static BraseroDiscResult
brasero_audio_disc_get_track (BraseroDisc *disc,
			      BraseroDiscTrack *track);
static BraseroDiscResult
brasero_audio_disc_load_track (BraseroDisc *disc,
			       BraseroDiscTrack *track);

static BraseroDiscResult
brasero_audio_disc_set_session_param (BraseroDisc *disc,
				      BraseroBurnSession *session);
static BraseroDiscResult
brasero_audio_disc_set_session_contents (BraseroDisc *disc,
					 BraseroBurnSession *session);

static BraseroDiscResult
brasero_audio_disc_add_uri (BraseroDisc *disc,
			    const char *uri);
static void
brasero_audio_disc_delete_selected (BraseroDisc *disc);
static void
brasero_audio_disc_clear (BraseroDisc *disc);
static void
brasero_audio_disc_reset (BraseroDisc *disc);

static void
brasero_audio_disc_fill_toolbar (BraseroDisc *disc, GtkBox *toolbar);

static gboolean
brasero_audio_disc_button_pressed_cb (GtkTreeView *tree,
				      GdkEventButton *event,
				      BraseroAudioDisc *disc);
static gboolean
brasero_audio_disc_key_released_cb (GtkTreeView *tree,
				    GdkEventKey *event,
				    BraseroAudioDisc *disc);
static void
brasero_audio_disc_display_edited_cb (GtkCellRendererText *cellrenderertext,
				      gchar *path_string,
				      gchar *text,
				      BraseroAudioDisc *disc);
static void
brasero_audio_disc_display_editing_started_cb (GtkCellRenderer *renderer,
					       GtkCellEditable *editable,
					       gchar *path,
					       BraseroAudioDisc *disc);
static void
brasero_audio_disc_display_editing_canceled_cb (GtkCellRenderer *renderer,
						 BraseroAudioDisc *disc);
static void
brasero_audio_disc_drag_data_received_cb (GtkTreeView *tree,
					  GdkDragContext *drag_context,
					  gint x,
					  gint y,
					  GtkSelectionData *selection_data,
					  guint info,
					  guint time,
					  BraseroAudioDisc *disc);
static gboolean
brasero_audio_disc_drag_drop_cb (GtkWidget *widget,
				 GdkDragContext*drag_context,
				 gint x,
				 gint y,
				 guint time,
				 BraseroAudioDisc *disc);
static gboolean
brasero_audio_disc_drag_motion_cb (GtkWidget *tree,
				   GdkDragContext *drag_context,
				   gint x,
				   gint y,
				   guint time,
				   BraseroAudioDisc *disc);
static void
brasero_audio_disc_drag_leave_cb (GtkWidget *tree,
				  GdkDragContext *drag_context,
				  guint time,
				  BraseroAudioDisc *disc);

static void
brasero_audio_disc_drag_begin_cb (GtkWidget *widget,
				  GdkDragContext *drag_context,
				  BraseroAudioDisc *disc);
static void
brasero_audio_disc_drag_end_cb (GtkWidget *tree,
			        GdkDragContext *drag_context,
			        BraseroAudioDisc *disc);

static void
brasero_audio_disc_row_deleted_cb (GtkTreeModel *model,
				   GtkTreePath *arg1,
				   BraseroAudioDisc *disc);
static void
brasero_audio_disc_row_inserted_cb (GtkTreeModel *model,
				    GtkTreePath *arg1,
				    GtkTreeIter *arg2,
				    BraseroAudioDisc *disc);
static void
brasero_audio_disc_row_changed_cb (GtkTreeModel *model,
				   GtkTreePath *path,
				   GtkTreeIter *iter,
				   BraseroAudioDisc *disc);

static void
brasero_audio_disc_edit_information_cb (GtkAction *action,
					BraseroAudioDisc *disc);
static void
brasero_audio_disc_open_activated_cb (GtkAction *action,
				      BraseroAudioDisc *disc);
static void
brasero_audio_disc_delete_activated_cb (GtkAction *action,
					BraseroDisc *disc);
static void
brasero_audio_disc_paste_activated_cb (GtkAction *action,
				       BraseroAudioDisc *disc);
static void
brasero_audio_disc_decrease_activity_counter (BraseroAudioDisc *disc);

static gchar *
brasero_audio_disc_get_selected_uri (BraseroDisc *disc);

static void
brasero_audio_disc_add_pause_cb (GtkAction *action, BraseroAudioDisc *disc);
static void
brasero_audio_disc_pause_clicked_cb (GtkButton *button, BraseroAudioDisc *disc);

static void
brasero_audio_disc_selection_changed (GtkTreeSelection *selection, GtkWidget *pause_button);

#ifdef BUILD_INOTIFY

typedef struct {
	gchar *uri;
	guint32 cookie;
	gint id;
} BraseroInotifyMovedData;

static gboolean
brasero_audio_disc_inotify_monitor_cb (GIOChannel *channel,
				       GIOCondition condition,
				       BraseroAudioDisc *disc);
static void
brasero_audio_disc_start_monitoring (BraseroAudioDisc *disc,
				     const char *uri);
static void
brasero_audio_disc_cancel_monitoring (BraseroAudioDisc *disc,
				      const char *uri);
#endif

struct _BraseroAudioDiscPrivate {
	BraseroVFS *vfs;
	BraseroVFSDataID attr_changed;
	BraseroVFSDataID add_dir;
	BraseroVFSDataID add_uri;

#ifdef BUILD_PLAYLIST

	BraseroVFSDataID add_playlist;

#endif

	GtkWidget *notebook;
	GtkWidget *tree;

	GtkUIManager *manager;

	GtkTreePath *selected_path;

#ifdef BUILD_INOTIFY

	int notify_id;
	GIOChannel *notify;
	GHashTable *monitored;

	GSList *moved_list;

#endif

	gint64 sectors;

       	GdkDragContext *drag_context;

	int activity_counter;

	int editing:1;
	int dragging:1;
	int reject_files:1;
};

enum {
	TRACK_NUM_COL,
	ICON_COL,
	NAME_COL,
	SIZE_COL,
	ARTIST_COL,
	URI_COL,
	SECTORS_COL,
	COMPOSER_COL,
	ISRC_COL,
	BACKGROUND_COL,
	SONG_COL,
	NB_COL
};

static GtkActionEntry entries[] = {
	{"ContextualMenu", NULL, N_("Menu")},
	{"Open", GTK_STOCK_OPEN, N_("Open"), NULL, NULL,
	 G_CALLBACK (brasero_audio_disc_open_activated_cb)},
	{"Edit", GTK_STOCK_PROPERTIES, N_("Edit information"), NULL, NULL,
	 G_CALLBACK (brasero_audio_disc_edit_information_cb)},
	{"Delete", GTK_STOCK_REMOVE, N_("Remove"), NULL, NULL,
	 G_CALLBACK (brasero_audio_disc_delete_activated_cb)},
	{"Paste", GTK_STOCK_PASTE, N_("Paste"), NULL, NULL,
	 G_CALLBACK (brasero_audio_disc_paste_activated_cb)},
	{"Pause", GTK_STOCK_MEDIA_PAUSE, N_("Add a pause"), NULL, NULL,
	 G_CALLBACK (brasero_audio_disc_add_pause_cb)}
};

static const char *description = {
	"<ui>"
	"<popup action='ContextMenu'>"
		"<menuitem action='Open'/>"
		"<menuitem action='Delete'/>"
		"<menuitem action='Edit'/>"
		"<separator/>"
		"<menuitem action='Paste'/>"
		"<separator/>"
		"<menuitem action='Pause'/>"
	"</popup>"
	"</ui>"
};

enum {
	TREE_MODEL_ROW = 150,
	TARGET_URIS_LIST,
};

static GtkTargetEntry ntables_cd [] = {
	{"GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, TREE_MODEL_ROW},
	{"text/uri-list", 0, TARGET_URIS_LIST}
};
static guint nb_targets_cd = sizeof (ntables_cd) / sizeof (ntables_cd[0]);

static GtkTargetEntry ntables_source [] = {
	{"GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, TREE_MODEL_ROW},
};
static guint nb_targets_source = sizeof (ntables_source) / sizeof (ntables_source[0]);

static GObjectClass *parent_class = NULL;

enum {
	PROP_NONE,
	PROP_REJECT_FILE,
};

/* 1 sec = 75 sectors, len is in nanosecond */
#define BRASERO_TIME_TO_SECTORS(len)		(gint64) (len * 75 / GST_SECOND)
#define BRASERO_SECTORS_TO_TIME(sectors)	(gint64) (sectors * GST_SECOND / 75)
#define COL_KEY "column_key"

GType
brasero_audio_disc_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (BraseroAudioDiscClass),
			NULL,
			NULL,
			(GClassInitFunc)brasero_audio_disc_class_init,
			NULL,
			NULL,
			sizeof (BraseroAudioDisc),
			0,
			(GInstanceInitFunc) brasero_audio_disc_init,
		};

		static const GInterfaceInfo disc_info =
		{
			(GInterfaceInitFunc) brasero_audio_disc_iface_disc_init,
			NULL,
			NULL
		};

		type = g_type_register_static(GTK_TYPE_VBOX, 
					      "BraseroAudioDisc",
					      &our_info,
					      0);

		g_type_add_interface_static (type,
					     BRASERO_TYPE_DISC,
					     &disc_info);
	}

	return type;
}

static void
brasero_audio_disc_class_init (BraseroAudioDiscClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = brasero_audio_disc_finalize;
	object_class->set_property = brasero_audio_disc_set_property;
	object_class->get_property = brasero_audio_disc_get_property;

	g_object_class_install_property (object_class,
					 PROP_REJECT_FILE,
					 g_param_spec_boolean
					 ("reject-file",
					  "Whether it accepts files",
					  "Whether it accepts files",
					  FALSE,
					  G_PARAM_READWRITE));
}

static void
brasero_audio_disc_iface_disc_init (BraseroDiscIface *iface)
{
	iface->add_uri = brasero_audio_disc_add_uri;
	iface->delete_selected = brasero_audio_disc_delete_selected;
	iface->clear = brasero_audio_disc_clear;
	iface->reset = brasero_audio_disc_reset;
	iface->get_track = brasero_audio_disc_get_track;
	iface->set_session_param = brasero_audio_disc_set_session_param;
	iface->set_session_contents = brasero_audio_disc_set_session_contents;
	iface->load_track = brasero_audio_disc_load_track;
	iface->get_status = brasero_audio_disc_get_status;
	iface->get_selected_uri = brasero_audio_disc_get_selected_uri;
	iface->fill_toolbar = brasero_audio_disc_fill_toolbar;
}

static void
brasero_audio_disc_get_property (GObject * object,
				 guint prop_id,
				 GValue * value,
				 GParamSpec * pspec)
{
	BraseroAudioDisc *disc;

	disc = BRASERO_AUDIO_DISC (object);

	switch (prop_id) {
	case PROP_REJECT_FILE:
		g_value_set_boolean (value, disc->priv->reject_files);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
brasero_audio_disc_set_property (GObject * object,
				 guint prop_id,
				 const GValue * value,
				 GParamSpec * pspec)
{
	BraseroAudioDisc *disc;

	disc = BRASERO_AUDIO_DISC (object);

	switch (prop_id) {
	case PROP_REJECT_FILE:
		disc->priv->reject_files = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
brasero_audio_disc_build_context_menu (BraseroAudioDisc *disc)
{
	GtkActionGroup *action_group;
	GError *error = NULL;

	action_group = gtk_action_group_new ("MenuAction");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group, entries,
				      G_N_ELEMENTS (entries),
				      disc);

	disc->priv->manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (disc->priv->manager,
					    action_group,
					    0);

	if (!gtk_ui_manager_add_ui_from_string (disc->priv->manager,
						description,
						-1,
						&error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}
}

static void
brasero_audio_disc_fill_toolbar (BraseroDisc *disc, GtkBox *toolbar)
{
	GtkWidget *button;
	GtkWidget *alignment;
	BraseroAudioDisc *audio_disc;

	audio_disc = BRASERO_AUDIO_DISC (disc);

	/* button to add pauses in between tracks */
	button = brasero_utils_make_button (NULL,
					    GTK_STOCK_MEDIA_PAUSE,
					    NULL,
					    GTK_ICON_SIZE_BUTTON);
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
	gtk_widget_set_sensitive (button, FALSE);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (button),
			  "clicked",
			  G_CALLBACK (brasero_audio_disc_pause_clicked_cb),
			  audio_disc);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (audio_disc->priv->tree)),
			  "changed",
			  G_CALLBACK (brasero_audio_disc_selection_changed),
			  button);
	gtk_widget_set_tooltip_text (button,
			      _("Add a 2 second pause after the track"));
	gtk_widget_show (button);
	
	alignment = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
	gtk_widget_show (alignment);

	gtk_container_add (GTK_CONTAINER (alignment), button);
	gtk_box_pack_start (GTK_BOX (toolbar), alignment, TRUE, TRUE, 0);
}

static void
brasero_audio_disc_init (BraseroAudioDisc *obj)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeModel *model;
	GtkWidget *scroll;

	obj->priv = g_new0 (BraseroAudioDiscPrivate, 1);
	gtk_box_set_spacing (GTK_BOX (obj), 8);


	/* notebook to display information about how to use the tree */
	obj->priv->notebook = brasero_utils_get_use_info_notebook ();
	gtk_box_pack_start (GTK_BOX (obj), obj->priv->notebook, TRUE, TRUE, 0);

	/* Tree */
	obj->priv->tree = gtk_tree_view_new ();
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (obj->priv->tree), TRUE);

	/* This must be before connecting to button press event */
	egg_tree_multi_drag_add_drag_support (GTK_TREE_VIEW (obj->priv->tree));

	gtk_widget_show (obj->priv->tree);
	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "button-press-event",
			  G_CALLBACK (brasero_audio_disc_button_pressed_cb),
			  obj);
	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "key-release-event",
			  G_CALLBACK (brasero_audio_disc_key_released_cb),
			  obj);

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (obj->priv->tree)),
				     GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (obj->priv->tree), TRUE);

	model = (GtkTreeModel*) gtk_list_store_new (NB_COL,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_INT64, 
						    G_TYPE_STRING,
						    G_TYPE_INT,
						    G_TYPE_STRING,
						    G_TYPE_BOOLEAN);

	g_signal_connect (G_OBJECT (model),
			  "row-deleted",
			  G_CALLBACK (brasero_audio_disc_row_deleted_cb),
			  obj);
	g_signal_connect (G_OBJECT (model),
			  "row-inserted",
			  G_CALLBACK (brasero_audio_disc_row_inserted_cb),
			  obj);
	g_signal_connect (G_OBJECT (model),
			  "row-changed",
			  G_CALLBACK (brasero_audio_disc_row_changed_cb),
			  obj);

	gtk_tree_view_set_model (GTK_TREE_VIEW (obj->priv->tree),
				 GTK_TREE_MODEL (model));
	g_object_unref (model);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 200);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "markup", TRACK_NUM_COL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "icon-name", ICON_COL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set_data (G_OBJECT (renderer), COL_KEY, GINT_TO_POINTER (NAME_COL));
	g_object_set (G_OBJECT (renderer),
		      "mode", GTK_CELL_RENDERER_MODE_EDITABLE,
		      "ellipsize-set", TRUE,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);

	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (brasero_audio_disc_display_edited_cb), obj);
	g_signal_connect (G_OBJECT (renderer), "editing-started",
			  G_CALLBACK (brasero_audio_disc_display_editing_started_cb), obj);
	g_signal_connect (G_OBJECT (renderer), "editing-canceled",
			  G_CALLBACK (brasero_audio_disc_display_editing_canceled_cb), obj);

	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "markup", NAME_COL);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "background", BACKGROUND_COL);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "editable", SONG_COL);
	gtk_tree_view_column_set_title (column, _("Title"));
	g_object_set (G_OBJECT (column),
		      "expand", TRUE,
		      "spacing", 4,
		      NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (obj->priv->tree),
				     column);

	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (obj->priv->tree),
					   column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set_data (G_OBJECT (renderer), COL_KEY, GINT_TO_POINTER (ARTIST_COL));
	g_object_set (G_OBJECT (renderer),
/*		      "editable", TRUE, disable this for the time being it doesn't play well with DND and double click */
		      /*"mode", GTK_CELL_RENDERER_MODE_EDITABLE,*/
		      "ellipsize-set", TRUE,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);

	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (brasero_audio_disc_display_edited_cb), obj);
	g_signal_connect (G_OBJECT (renderer), "editing-started",
			  G_CALLBACK (brasero_audio_disc_display_editing_started_cb), obj);
	g_signal_connect (G_OBJECT (renderer), "editing-canceled",
			  G_CALLBACK (brasero_audio_disc_display_editing_canceled_cb), obj);
	column = gtk_tree_view_column_new_with_attributes (_("Artist"), renderer,
							   "text", ARTIST_COL,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (obj->priv->tree),
				     column);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 200);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Length"), renderer,
							   "text", SIZE_COL,
							   "background", BACKGROUND_COL,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (obj->priv->tree),
				     column);
	gtk_tree_view_column_set_resizable (column, FALSE);


	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), obj->priv->tree);

	gtk_notebook_append_page (GTK_NOTEBOOK (obj->priv->notebook),
				  scroll,
				  NULL);

	/* dnd */
	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (obj->priv->tree),
					      ntables_cd,
					      nb_targets_cd,
					      GDK_ACTION_COPY|
					      GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "drag-data-received",
			  G_CALLBACK (brasero_audio_disc_drag_data_received_cb),
			  obj);
	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "drag-drop",
			  G_CALLBACK (brasero_audio_disc_drag_drop_cb),
			  obj);
	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "drag_motion",
			  G_CALLBACK (brasero_audio_disc_drag_motion_cb),
			  obj);
	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "drag_leave",
			  G_CALLBACK (brasero_audio_disc_drag_leave_cb),
			  obj);

	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "drag-begin",
			  G_CALLBACK (brasero_audio_disc_drag_begin_cb),
			  obj);
	g_signal_connect (G_OBJECT (obj->priv->tree),
			  "drag_end",
			  G_CALLBACK (brasero_audio_disc_drag_end_cb),
			  obj);

	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (obj->priv->tree),
						GDK_BUTTON1_MASK,
						ntables_source,
						nb_targets_source,
						GDK_ACTION_COPY |
						GDK_ACTION_MOVE);

	/* create menus */
	brasero_audio_disc_build_context_menu (obj);

#ifdef BUILD_INOTIFY
	int fd;

	fd = inotify_init ();
	if (fd != -1) {
		obj->priv->notify = g_io_channel_unix_new (fd);

		g_io_channel_set_encoding (obj->priv->notify, NULL, NULL);
		g_io_channel_set_close_on_unref (obj->priv->notify, TRUE);
		obj->priv->notify_id = g_io_add_watch (obj->priv->notify,
						       G_IO_IN | G_IO_HUP | G_IO_PRI,
						       (GIOFunc) brasero_audio_disc_inotify_monitor_cb,
						       obj);
		g_io_channel_unref (obj->priv->notify);
	}
	else
		g_warning ("Failed to open /dev/inotify: %s\n",
			   strerror (errno));
#endif
}

static void
brasero_audio_disc_reset_real (BraseroAudioDisc *disc)
{
	if (disc->priv->selected_path) {
		gtk_tree_path_free (disc->priv->selected_path);
		disc->priv->selected_path = NULL;
	}

	if (disc->priv->vfs)
		brasero_vfs_cancel (disc->priv->vfs, G_OBJECT (disc));

#ifdef BUILD_INOTIFY

	GSList *iter;

	/* we remove all the moved events waiting in the list */
	for (iter = disc->priv->moved_list; iter; iter = iter->next) {
		BraseroInotifyMovedData *data;

		data = iter->data;
		g_source_remove (data->id);
		g_free (data->uri);
		g_free (data);
	}
	g_slist_free (disc->priv->moved_list);
	disc->priv->moved_list = NULL;

	/* destroy monitoring hash */
	if (disc->priv->monitored) {
		g_hash_table_destroy (disc->priv->monitored);
		disc->priv->monitored = NULL;
	}

#endif

	disc->priv->sectors = 0;

	disc->priv->activity_counter = 1;
	brasero_audio_disc_decrease_activity_counter (disc);
}

static void
brasero_audio_disc_finalize (GObject *object)
{
	BraseroAudioDisc *cobj;
	cobj = BRASERO_AUDIO_DISC(object);
	
	brasero_audio_disc_reset_real (cobj);

#ifdef BUILD_INOTIFY

	if (cobj->priv->notify_id)
		g_source_remove (cobj->priv->notify_id);

	if (cobj->priv->monitored) {
		g_hash_table_destroy (cobj->priv->monitored);
		cobj->priv->monitored = NULL;
	}

#endif
	
	if (cobj->priv->vfs) {
		brasero_vfs_cancel (cobj->priv->vfs, G_OBJECT (cobj));
		g_object_unref (cobj->priv->vfs);
		cobj->priv->vfs = NULL;
	}

	g_object_unref (cobj->priv->manager);
	cobj->priv->manager = NULL;

	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
brasero_audio_disc_new ()
{
	BraseroAudioDisc *obj;
	
	obj = BRASERO_AUDIO_DISC (g_object_new (BRASERO_TYPE_AUDIO_DISC, NULL));
	
	return GTK_WIDGET (obj);
}

/******************************** activity *************************************/
static BraseroDiscResult
brasero_audio_disc_get_status (BraseroDisc *disc)
{
	GtkTreeModel *model;

	if (BRASERO_AUDIO_DISC (disc)->priv->activity_counter)
		return BRASERO_DISC_NOT_READY;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (BRASERO_AUDIO_DISC (disc)->priv->tree));
	if (!gtk_tree_model_iter_n_children (model, NULL))
		return BRASERO_DISC_ERROR_EMPTY_SELECTION;

	return BRASERO_DISC_OK;
}

static void
brasero_audio_disc_increase_activity_counter (BraseroAudioDisc *disc)
{
	GdkCursor *cursor;

	if (disc->priv->activity_counter == 0 && GTK_WIDGET (disc)->window) {
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (GTK_WIDGET (disc)->window, cursor);
		gdk_cursor_unref (cursor);
	}

	disc->priv->activity_counter++;
}

static void
brasero_audio_disc_decrease_activity_counter (BraseroAudioDisc *disc)
{

	if (disc->priv->activity_counter == 1 && GTK_WIDGET (disc)->window)
		gdk_window_set_cursor (GTK_WIDGET (disc)->window, NULL);

	disc->priv->activity_counter--;
}

/******************************** utility functions ****************************/
static void
brasero_audio_disc_size_changed (BraseroAudioDisc *disc)
{
	brasero_disc_size_changed (BRASERO_DISC (disc),
				   disc->priv->sectors);
}

static gboolean
brasero_audio_disc_has_gap (BraseroAudioDisc *disc,
			    GtkTreeIter *row,
			    GtkTreeIter *gap)
{
	GtkTreeModel *model;
	gboolean is_song;

	*gap = *row;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	if (!gtk_tree_model_iter_next (model, gap))
		return FALSE;

	gtk_tree_model_get (model, gap,
			    SONG_COL, &is_song,
			    -1);

	return (is_song != TRUE);
}

static void
brasero_audio_disc_add_gap (BraseroAudioDisc *disc,
			    GtkTreeIter *iter,
			    gint64 gap)
{
	GtkTreeIter gap_iter;
	GtkTreeModel *model;
	gint64 sectors;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	if (gap) {
		gchar *size;

		if (brasero_audio_disc_has_gap (disc, iter, &gap_iter)) {
			gtk_tree_model_get (model, &gap_iter,
					    SECTORS_COL, &sectors,
					    -1);

			disc->priv->sectors -= sectors;
		}
		else {
			GdkPixbuf *pixbuf;

			gtk_list_store_insert_after (GTK_LIST_STORE (model),
						     &gap_iter,
						     iter);

			pixbuf = gtk_widget_render_icon (GTK_WIDGET (disc),
							 GTK_STOCK_MEDIA_PAUSE,
							 GTK_ICON_SIZE_MENU,
							 NULL);

			gtk_list_store_set (GTK_LIST_STORE (model), &gap_iter,
					    NAME_COL, _("<i><b>Pause</b></i>"),
					    SONG_COL, FALSE,
					    //BACKGROUND_COL, "green yellow",
					    ICON_COL, pixbuf,
					    -1);
			g_object_unref (pixbuf);
		}

		size = brasero_utils_get_time_string (gap, TRUE, FALSE);
		gtk_list_store_set (GTK_LIST_STORE (model), &gap_iter,
				    SIZE_COL, size,
				    SECTORS_COL, BRASERO_TIME_TO_SECTORS (gap),
				    -1);
		g_free (size);

		disc->priv->sectors += BRASERO_TIME_TO_SECTORS (gap);
		brasero_audio_disc_size_changed (disc);
	}
	else if (brasero_audio_disc_has_gap (disc, iter, &gap_iter)) {
		gtk_tree_model_get (model, &gap_iter,
				    SECTORS_COL, &sectors,
				    -1);

		disc->priv->sectors -= sectors;
		brasero_audio_disc_size_changed (disc);
		gtk_list_store_remove (GTK_LIST_STORE (model), &gap_iter);
	}
}

static void
brasero_audio_disc_re_index_track_num (BraseroAudioDisc *disc,
				       GtkTreeModel *model)
{
	GtkTreeIter iter;
	gboolean is_song;
	gchar *text;
	gint num = 0;

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	g_signal_handlers_block_by_func (model,
					 brasero_audio_disc_row_changed_cb,
					 disc);
	do {
		gtk_tree_model_get (model, &iter,
				    SONG_COL, &is_song,
				    -1);

		/* This is gap row */
		if (!is_song)
			continue;

		num ++;
		text = g_strdup_printf ("<b><i>%02i -</i></b>", num);

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    TRACK_NUM_COL, text,
				    -1);

		g_free (text);
	} while (gtk_tree_model_iter_next (model, &iter));

	g_signal_handlers_unblock_by_func (model,
					   brasero_audio_disc_row_changed_cb,
					   disc);
}

static gint
brasero_audio_disc_get_track_num (BraseroAudioDisc *disc,
				  GtkTreeModel *model)
{
	GtkTreeIter iter;
	gboolean is_song;
	gint num = 0;

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return 0;

	g_signal_handlers_block_by_func (model,
					 brasero_audio_disc_row_changed_cb,
					 disc);
	do {
		gtk_tree_model_get (model, &iter,
				    SONG_COL, &is_song,
				    -1);

		if (is_song)
			num ++;

	} while (gtk_tree_model_iter_next (model, &iter));

	g_signal_handlers_unblock_by_func (model,
					   brasero_audio_disc_row_changed_cb,
					   disc);

	return num;		
}

static void
brasero_audio_disc_row_deleted_cb (GtkTreeModel *model,
				   GtkTreePath *path,
				   BraseroAudioDisc *disc)
{
	brasero_disc_contents_changed (BRASERO_DISC (disc),
				       brasero_audio_disc_get_track_num (disc, model));
	brasero_audio_disc_re_index_track_num (disc, model);
}

static void
brasero_audio_disc_row_inserted_cb (GtkTreeModel *model,
				    GtkTreePath *path,
				    GtkTreeIter *iter,
				    BraseroAudioDisc *disc)
{
	brasero_disc_contents_changed (BRASERO_DISC (disc),
				       brasero_audio_disc_get_track_num (disc, model));
}

static void
brasero_audio_disc_row_changed_cb (GtkTreeModel *model,
				   GtkTreePath *path,
				   GtkTreeIter *iter,
				   BraseroAudioDisc *disc)
{
	brasero_audio_disc_re_index_track_num (disc, model);
	brasero_disc_contents_changed (BRASERO_DISC (disc),
				       brasero_audio_disc_get_track_num (disc, model));
}

static void
brasero_audio_disc_set_row_from_metadata (BraseroAudioDisc *disc,
					  GtkTreeModel *model,
					  GtkTreeIter *iter,
					  const BraseroMetadata *metadata)
{
	gchar *size;
	gchar *icon_string;

	icon_string = gnome_icon_lookup (gtk_icon_theme_get_default (), NULL,
					 NULL, NULL, NULL, metadata->type,
					 GNOME_ICON_LOOKUP_FLAGS_NONE, NULL);

	size = brasero_utils_get_time_string (metadata->len, TRUE, FALSE);
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    ICON_COL, icon_string,
			    ARTIST_COL, metadata->artist,
			    SIZE_COL, size,
			    SECTORS_COL, BRASERO_TIME_TO_SECTORS (metadata->len),
			    COMPOSER_COL, metadata->composer,
			    ISRC_COL, metadata->isrc,
			    SONG_COL, TRUE,
			    -1);
	g_free (icon_string);

	if (metadata->title) {
		gchar *name;

		name = g_markup_escape_text (metadata->title, -1);
		gtk_list_store_set (GTK_LIST_STORE (model), iter,
				    NAME_COL, name,
				    -1);
		g_free (name);
	}

	g_free (size);
}

/*************** shared code for dir/playlist addition *************************/
static void
brasero_audio_disc_file_type_error_dialog (BraseroAudioDisc *disc,
					   const gchar *uri)
{
	GtkWidget *dialog, *toplevel;
	gchar *name;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (disc));
	if (toplevel == NULL) {
		g_warning ("Content widget error : can't handle \"%s\".\n", uri);
		return ;
	}

    	BRASERO_GET_BASENAME_FOR_DISPLAY (uri, name);
	dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
					 GTK_DIALOG_DESTROY_WITH_PARENT|
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 _("\"%s\" can't be handled by gstreamer:"),
					 name);
	g_free (name);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Unhandled song"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Make sure the appropriate codec is installed."));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
brasero_audio_disc_add_file (BraseroAudioDisc *disc,
			     const gchar *uri,
			     gint64 duration)
{
#ifdef BUILD_INOTIFY
	brasero_audio_disc_start_monitoring (disc, uri);
#endif

	disc->priv->sectors += BRASERO_TIME_TO_SECTORS (duration);
	brasero_audio_disc_size_changed (disc);
}

static void
brasero_audio_disc_result (BraseroVFS *vfs,
			   GObject *obj,
			   GnomeVFSResult result,
			   const gchar *uri,
			   GnomeVFSFileInfo *info,
			   BraseroMetadata *metadata,
			   gpointer null_data)
{
	gchar *name;
	gchar *markup;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *escaped_name;
	BraseroAudioDisc *disc = BRASERO_AUDIO_DISC (obj);

	if (result != GNOME_VFS_OK)
		return;

	/* we silently ignore the title and any error */
	if (!metadata)
		return;

	if (!metadata->has_audio)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	escaped_name = g_path_get_basename (uri);
	name = gnome_vfs_unescape_string_for_display (escaped_name);
	g_free (escaped_name);

	markup = g_markup_escape_text (name, -1);
    	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    NAME_COL, markup,
			    URI_COL, uri,
			    -1);
	g_free (markup);

	brasero_audio_disc_set_row_from_metadata (disc,
						  model,
						  &iter,
						  metadata);

	brasero_audio_disc_add_file (disc,
				     uri,
				     metadata->len);
}

static void
brasero_audio_disc_vfs_operation_finished (GObject *object,
					   gpointer null_data,
					   gboolean cancelled)
{
	BraseroAudioDisc *disc = BRASERO_AUDIO_DISC (object);

	brasero_audio_disc_decrease_activity_counter (disc);
}

/*********************** directories exploration *******************************/
static BraseroDiscResult
brasero_audio_disc_visit_dir_async (BraseroAudioDisc *disc, const gchar *uri)
{
	GList *uris;
	gboolean result;

	uris = g_list_prepend (NULL, (gchar*) uri);

	if (!disc->priv->vfs)
		disc->priv->vfs = brasero_vfs_get_default ();

	if (!disc->priv->add_dir)
		disc->priv->add_dir = brasero_vfs_register_data_type (disc->priv->vfs,
								      G_OBJECT (disc),
								      G_CALLBACK (brasero_audio_disc_result),
								      brasero_audio_disc_vfs_operation_finished);

	brasero_audio_disc_increase_activity_counter (disc);
	result = brasero_vfs_get_metadata (disc->priv->vfs,
					   uris,
					   GNOME_VFS_FILE_INFO_FOLLOW_LINKS |
					   GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS |
					   GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					   GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE,
					   TRUE,
					   disc->priv->add_dir,
					   NULL);
	g_list_free (uris);

	return result;
}

static gboolean
brasero_audio_disc_add_dir (BraseroAudioDisc *disc, const gchar *uri)
{
	gint answer;
	GtkWidget *dialog;
	GtkWidget *toplevel;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (disc));
	dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
					 GTK_DIALOG_DESTROY_WITH_PARENT |
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Directories can't be added to an audio disc:"));

	gtk_window_set_title (GTK_WINDOW (dialog), _("Directory search"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Do you want to search for audio discs inside?"));

	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				_("Search directory"), GTK_RESPONSE_OK,
				NULL);

	gtk_widget_show_all (dialog);
	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (answer != GTK_RESPONSE_OK)
		return TRUE;

	return brasero_audio_disc_visit_dir_async (disc, uri);
}

/************************** playlist parsing ***********************************/

#if BUILD_PLAYLIST

static BraseroDiscResult
brasero_audio_disc_add_playlist (BraseroAudioDisc *disc,
				 const gchar *uri)
{
	if (!disc->priv->vfs)
		disc->priv->vfs = brasero_vfs_get_default ();

	if (!disc->priv->add_playlist)
		disc->priv->add_playlist = brasero_vfs_register_data_type (disc->priv->vfs,
									   G_OBJECT (disc),
									   G_CALLBACK (brasero_audio_disc_result),
									   brasero_audio_disc_vfs_operation_finished);

	brasero_audio_disc_increase_activity_counter (disc);
	return brasero_vfs_parse_playlist (disc->priv->vfs,
					   uri,
					   GNOME_VFS_FILE_INFO_FOLLOW_LINKS |
					   GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS |
					   GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					   GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE,
					   disc->priv->add_playlist,
					   NULL);
}

#endif

/**************************** New Row ******************************************/
static void
brasero_audio_disc_unreadable_dialog (BraseroAudioDisc *disc,
				      const char *uri,
				      GnomeVFSResult result)
{
	GtkWidget *dialog, *toplevel;
	gchar *name;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (disc));
	if (toplevel == NULL) {
		g_warning ("Can't open file %s : %s\n",
			   uri,
			   gnome_vfs_result_to_string (result));

		return;
	}

	name = g_filename_display_basename (uri);
	dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
					 GTK_DIALOG_DESTROY_WITH_PARENT|
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 _("File \"%s\" can't be opened."),
					 name);
	g_free (name);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Unreadable file"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  gnome_vfs_result_to_string (result));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
brasero_audio_disc_new_row_cb (BraseroVFS *vfs,
			       GObject *obj,
			       GnomeVFSResult result,
			       const gchar *uri,
			       const GnomeVFSFileInfo *info,
			       const BraseroMetadata *metadata,
			       gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeIter gap_iter;
	GtkTreePath *treepath;
	GtkTreeRowReference *ref = user_data;
	BraseroAudioDisc *disc = BRASERO_AUDIO_DISC (obj);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	treepath = gtk_tree_row_reference_get_path (ref);
	gtk_tree_row_reference_free (ref);
	if (!treepath)
		return;

	gtk_tree_model_get_iter (model, &iter, treepath);
	gtk_tree_path_free (treepath);

	if (result != GNOME_VFS_OK) {
		brasero_audio_disc_unreadable_dialog (disc,
						      uri,
						      result);

		if (brasero_audio_disc_has_gap (disc, &iter, &gap_iter))
			brasero_audio_disc_add_gap (disc, &gap_iter, 0);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		return;
	}

	if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		if (brasero_audio_disc_has_gap (disc, &iter, &gap_iter))
			brasero_audio_disc_add_gap (disc, &gap_iter, 0);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		brasero_audio_disc_add_dir (disc, uri);
		return;
	}

#if BUILD_PLAYLIST

	/* see if it a playlist */
	if (info->type == GNOME_VFS_FILE_TYPE_REGULAR
	&&  (!strcmp (info->mime_type, "audio/x-scpls")
	||   !strcmp (info->mime_type, "audio/x-ms-asx")
	||   !strcmp (info->mime_type, "audio/x-mp3-playlist")
	||   !strcmp (info->mime_type, "audio/x-mpegurl"))) {
		/* This is a supported playlist */
		if (brasero_audio_disc_has_gap (disc, &iter, &gap_iter))
			brasero_audio_disc_add_gap (disc, &gap_iter, 0);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		brasero_audio_disc_add_playlist (disc, uri);
		return;
	}

#endif

	if (info->type != GNOME_VFS_FILE_TYPE_REGULAR
	|| !metadata
	|| !metadata->has_audio
	||  metadata->has_video) {
		if (brasero_audio_disc_has_gap (disc, &iter, &gap_iter))
			brasero_audio_disc_add_gap (disc, &gap_iter, 0);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		brasero_audio_disc_file_type_error_dialog (disc, uri);
		return;
	}

	if (GNOME_VFS_FILE_INFO_SYMLINK (info)) {
		uri = g_strconcat ("file://", info->symlink_name, NULL);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    URI_COL, uri, -1);
	}

	brasero_audio_disc_set_row_from_metadata (disc,
						  model,
						  &iter,
						  metadata);
	brasero_audio_disc_add_file (disc,
				     uri,
				     metadata->len);
}

static BraseroDiscResult
brasero_audio_disc_add_uri_real (BraseroAudioDisc *disc,
				 const gchar *uri,
				 gint pos,
				 gint64 gap_sectors,
				 GtkTreePath **path_return)
{
	GtkTreeRowReference *ref;
	GtkTreePath *treepath;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gboolean success;
	gchar *markup;
	gchar *name;
	GList *uris;

	g_return_val_if_fail (uri != NULL, BRASERO_DISC_ERROR_UNKNOWN);

	if (disc->priv->reject_files)
		return BRASERO_DISC_NOT_READY;

	gtk_notebook_set_current_page (GTK_NOTEBOOK (BRASERO_AUDIO_DISC (disc)->priv->notebook), 1);

	store = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	if (pos > -1)
		gtk_list_store_insert (GTK_LIST_STORE (store), &iter, pos);
	else
		gtk_list_store_append (GTK_LIST_STORE (store), &iter);

	BRASERO_GET_BASENAME_FOR_DISPLAY (uri, name);
	markup = g_markup_escape_text (name, -1);
	g_free (name);

    	gtk_list_store_set (GTK_LIST_STORE (store), &iter,
			    NAME_COL, markup,
			    ICON_COL, "image-loading",
			    URI_COL, uri,
			    ARTIST_COL, _("loading"),
			    SIZE_COL, _("loading"),
			    SONG_COL, TRUE,
			    -1);
	g_free (markup);

	if (gap_sectors > 0)
		brasero_audio_disc_add_gap (disc,
					    &iter,
					    BRASERO_SECTORS_TO_TIME (gap_sectors));

	treepath = gtk_tree_model_get_path (store, &iter);
	ref = gtk_tree_row_reference_new (store, treepath);

	if (path_return)
		*path_return = treepath;
	else
		gtk_tree_path_free (treepath);

	/* get info async for the file */
	if (!disc->priv->vfs)
		disc->priv->vfs = brasero_vfs_get_default ();

	if (!disc->priv->add_uri)
		disc->priv->add_uri = brasero_vfs_register_data_type (disc->priv->vfs,
								      G_OBJECT (disc),
								      G_CALLBACK (brasero_audio_disc_new_row_cb),
								      brasero_audio_disc_vfs_operation_finished);
	/* FIXME: if cancelled ref won't be destroyed ? 
	 * it shouldn since the callback is always 
	 * called even if there is an error */
	uris = g_list_prepend (NULL, (gchar *) uri);
	brasero_audio_disc_increase_activity_counter (disc);
	success = brasero_vfs_get_metadata (disc->priv->vfs,
					    uris,
					    GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS |
					    GNOME_VFS_FILE_INFO_FOLLOW_LINKS|
					    GNOME_VFS_FILE_INFO_GET_MIME_TYPE|
					    GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE,
					    FALSE,
					    disc->priv->add_uri,
					    ref);
	g_list_free (uris);

	if (!success)
		return BRASERO_DISC_ERROR_THREAD;

	return BRASERO_DISC_OK;
}

static BraseroDiscResult
brasero_audio_disc_add_uri (BraseroDisc *disc,
			    const gchar *uri)
{
	GtkTreePath *treepath = NULL;
	BraseroAudioDisc *audio_disc;
	BraseroDiscResult result;

	audio_disc = BRASERO_AUDIO_DISC (disc);
	result = brasero_audio_disc_add_uri_real (audio_disc,
						  uri,
						  -1,
						  0,
						  &treepath);

	if (treepath) {
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (audio_disc->priv->tree),
					      treepath,
					      NULL,
					      TRUE,
					      0.5,
					      0.5);
		gtk_tree_path_free (treepath);
	}

	return result;
}

/******************************** Row removing *********************************/
static void
brasero_audio_disc_remove (BraseroAudioDisc *disc,
			   GtkTreePath *treepath)
{
	GtkTreeIter gap_iter;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint64 sectors;
	gchar *uri;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	gtk_tree_model_get_iter (model, &iter, treepath);
	gtk_tree_model_get (model, &iter,
			    URI_COL, &uri,
			    SECTORS_COL, &sectors,
			    -1);

	if (brasero_audio_disc_has_gap (disc, &iter, &gap_iter)) {
		gint64 gap_sectors = 0;

		gtk_tree_model_get (model, &gap_iter,
				    SECTORS_COL, &gap_sectors,
				    -1);

		sectors += gap_sectors;
		gtk_list_store_remove (GTK_LIST_STORE (model), &gap_iter);
	}

	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

	if (sectors > 0) {
		disc->priv->sectors -= sectors;
		brasero_audio_disc_size_changed (disc);
	}

	if (!uri)
		return;

#ifdef BUILD_INOTIFY
	brasero_audio_disc_cancel_monitoring (disc, uri);
#endif
	g_free (uri);
}

static void
brasero_audio_disc_delete_selected (BraseroDisc *disc)
{
	GtkTreeSelection *selection;
	BraseroAudioDisc *audio;
	GtkTreePath *treepath;
	GtkTreeModel *model;
	GList *list;

	audio = BRASERO_AUDIO_DISC (disc);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (audio->priv->tree));

	/* we must start by the end for the treepaths to point to valid rows */
	list = gtk_tree_selection_get_selected_rows (selection, &model);
	list = g_list_reverse (list);
	for (; list; list = g_list_remove (list, treepath)) {
		treepath = list->data;
		brasero_audio_disc_remove (audio, treepath);
		gtk_tree_path_free (treepath);
	}
}

static void
brasero_audio_disc_clear (BraseroDisc *disc)
{
	BraseroAudioDisc *audio;
	GtkTreeModel *model;

	audio = BRASERO_AUDIO_DISC (disc);
	brasero_audio_disc_reset_real (audio);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (audio->priv->tree));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	gtk_notebook_set_current_page (GTK_NOTEBOOK (BRASERO_AUDIO_DISC (disc)->priv->notebook), 0);
	brasero_disc_size_changed (disc, 0);
}

static void
brasero_audio_disc_reset (BraseroDisc *disc)
{
	brasero_audio_disc_clear (disc);
}

/********************************* create track ********************************/
static BraseroDiscResult
brasero_audio_disc_get_track (BraseroDisc *disc,
			      BraseroDiscTrack *track)
{
	gchar *uri;
	GtkTreeIter iter;
	GtkTreeModel *model;
	BraseroDiscSong *song;
	BraseroAudioDisc *audio;

	audio = BRASERO_AUDIO_DISC (disc);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (audio->priv->tree));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return BRASERO_DISC_ERROR_EMPTY_SELECTION;

	track->type = BRASERO_DISC_TRACK_AUDIO;
	song = NULL;

	do {
		gtk_tree_model_get (model, &iter,
				    URI_COL, &uri,
				    -1);

		if (!uri) {
			gint64 sectors;

			gtk_tree_model_get (model, &iter,
					    SECTORS_COL, &sectors,
					    -1);
			if (sectors && song)
				song->gap += sectors;
		}
		else {
			song = g_new0 (BraseroDiscSong, 1);
			song->uri = uri;
			track->contents.tracks = g_slist_append (track->contents.tracks, song);
		}

	} while (gtk_tree_model_iter_next (model, &iter));

	return BRASERO_DISC_OK;
}

static BraseroDiscResult
brasero_audio_disc_set_session_param (BraseroDisc *disc,
				      BraseroBurnSession *session)
{
	BraseroTrackType type;

	type.type = BRASERO_TRACK_TYPE_AUDIO;
	type.subtype.audio_format = BRASERO_AUDIO_FORMAT_UNDEFINED;
	brasero_burn_session_set_input_type (session, &type);
	return BRASERO_BURN_OK;
}

static BraseroDiscResult
brasero_audio_disc_set_session_contents (BraseroDisc *disc,
					 BraseroBurnSession *session)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	BraseroTrack *track;
	BraseroAudioDisc *audio;

	audio = BRASERO_AUDIO_DISC (disc);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (audio->priv->tree));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return BRASERO_DISC_ERROR_EMPTY_SELECTION;

	track = NULL;
	do {
		gchar *uri;
		gchar *title;
		gchar *artist;
		gint64 sectors;
		BraseroSongInfo *info;

		gtk_tree_model_get (model, &iter,
				    URI_COL, &uri,
				    NAME_COL, &title,
				    ARTIST_COL, &artist,
				    SECTORS_COL, &sectors,
				    -1);

		if (!uri) {
			/* This is a gap so sectors refers to its size */
			brasero_track_set_audio_boundaries (track, -1, -1, sectors);
			continue;
		}

		info = g_new0 (BraseroSongInfo, 1);
		info->title = title;
		info->artist = artist;

		track = brasero_track_new (BRASERO_TRACK_TYPE_AUDIO);
		brasero_track_set_audio_source (track, uri, BRASERO_AUDIO_FORMAT_UNDEFINED);
		brasero_track_set_audio_boundaries (track, 0, sectors, -1);
		brasero_track_set_audio_info (track, info);
		brasero_burn_session_add_track (session, track);

	} while (gtk_tree_model_iter_next (model, &iter));

	return BRASERO_DISC_OK;
}

/********************************* load track **********************************/
static BraseroDiscResult
brasero_audio_disc_load_track (BraseroDisc *disc,
			       BraseroDiscTrack *track)
{
	GSList *iter;

	g_return_val_if_fail (track->type == BRASERO_DISC_TRACK_AUDIO, FALSE);

	if (track->contents.tracks == NULL)
		return BRASERO_DISC_ERROR_EMPTY_SELECTION;

	for (iter = track->contents.tracks; iter; iter = iter->next) {
		BraseroDiscSong *song;

		song = iter->data;
		brasero_audio_disc_add_uri_real (BRASERO_AUDIO_DISC (disc),
						 song->uri,
						 -1,
						 song->gap,
						 NULL);
	}

	return BRASERO_DISC_OK;
}

/*********************************  DND ****************************************/
static gint
brasero_audio_disc_get_dest_path (BraseroAudioDisc *disc,
				  gint x,
				  gint y)
{
	gint position;
	GtkTreeModel *model;
	GtkTreeViewDropPosition pos;
	GtkTreePath *treepath = NULL;

	/* while the treeview is still under the information pane, it is not 
	 * realized yet and the following function will fail */
	if (GTK_WIDGET_DRAWABLE (disc->priv->tree))
		gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (disc->priv->tree),
						   x,
						   y,
						   &treepath,
						   &pos);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));

	if (treepath) {
		if (pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE
		||  pos == GTK_TREE_VIEW_DROP_BEFORE) {
			if (!gtk_tree_path_prev (treepath)) {
				gtk_tree_path_free (treepath);
				treepath = NULL;
			}
		}

		if (treepath) {
			gint *indices;
			gboolean is_song;
			GtkTreeIter iter;

			/* we check that the dest is not in between a song
			 * and its pause/gap/silence */
			gtk_tree_model_get_iter (model, &iter, treepath);
			gtk_tree_model_get (model, &iter,
					    SONG_COL, &is_song,
					    -1);

			if (is_song
			&&  gtk_tree_model_iter_next (model, &iter)) {
				gtk_tree_model_get (model, &iter,
						    SONG_COL, &is_song,
						    -1);

				if (!is_song)
					gtk_tree_path_next (treepath);
			}

			indices = gtk_tree_path_get_indices (treepath);
			position = indices [0];

			gtk_tree_path_free (treepath);
		}
		else
			position = -1;
	}
	else
		position = gtk_tree_model_iter_n_children (model, NULL) - 1;

	return position;
}

static void
brasero_audio_disc_merge_gaps (BraseroAudioDisc *disc,
			       GtkTreeModel *model,
			       GtkTreeIter *iter,
			       GtkTreeIter *pos)
{
	GtkTreePath *iter_path, *pos_path;
	gint64 sectors_iter, sectors_pos;
	gchar *size;
	gint equal;

	iter_path = gtk_tree_model_get_path (model, iter);
	pos_path = gtk_tree_model_get_path (model, pos);
	equal = gtk_tree_path_compare (iter_path, pos_path);
	gtk_tree_path_free (iter_path);
	gtk_tree_path_free (pos_path);

	if (!equal)
		return;

	gtk_tree_model_get (model, pos,
			    SECTORS_COL, &sectors_pos,
			    -1);
	gtk_tree_model_get (model, iter,
			    SECTORS_COL, &sectors_iter,
			    -1);

	sectors_pos += sectors_iter;
	size = brasero_utils_get_time_string (BRASERO_SECTORS_TO_TIME (sectors_pos), TRUE, FALSE);
	gtk_list_store_set (GTK_LIST_STORE (model), pos,
			    SIZE_COL, size,
			    SECTORS_COL, sectors_pos,
			    -1);
	g_free (size);

	gtk_list_store_remove (GTK_LIST_STORE (model), iter);
}

static GtkTreePath *
brasero_audio_disc_move_to_dest (BraseroAudioDisc *disc,
				 GtkTreeSelection *selection,
				 GtkTreeModel *model,
				 GtkTreePath *dest,
				 GtkTreeIter *iter,
				 gboolean is_gap)
{
	GtkTreeIter position;
	GtkTreeIter gap_iter;
	gboolean has_gap = FALSE;

	gtk_tree_model_get_iter (model, &position, dest);
	if (is_gap) {
		gboolean is_song;
		GtkTreeIter next_pos;

		/* Check that the row and the row under the destination
		 * row are not gaps as well and if so merge both of them */

		gtk_tree_model_get (model, &position,
				    SONG_COL, &is_song,
				    -1);

		if (!is_song) {
			/* NOTE: if the path we're moving is above the
			 * dest the dest path won't be valid once we
			 * have merged the two gaps */
			gtk_tree_path_free (dest);
			brasero_audio_disc_merge_gaps (disc,
						       model,
						       iter,
						       &position);
			return gtk_tree_model_get_path (model, &position);
		}

		gtk_tree_path_next (dest);
		if (gtk_tree_model_get_iter (model, &next_pos, dest)) {
			gtk_tree_model_get (model, &next_pos,
					    SONG_COL, &is_song,
					    -1);

			if (!is_song) {
				/* NOTE: if the path we're moving is above the
				 * dest the dest path won't be valid once we
				 * have merged the two gaps */
				gtk_tree_path_free (dest);
				brasero_audio_disc_merge_gaps (disc,
							       model,
							       iter,
							       &next_pos);
				return gtk_tree_model_get_path (model, &next_pos);
			}
		}
	}
	else
		has_gap = brasero_audio_disc_has_gap (disc, iter, &gap_iter);

	gtk_tree_path_free (dest);
	gtk_list_store_move_after (GTK_LIST_STORE (model),
				   iter,
				   &position);

	if (has_gap && !gtk_tree_selection_iter_is_selected (selection, &gap_iter)) {
		gtk_list_store_move_after (GTK_LIST_STORE (model),
					   &gap_iter,
					   iter);

		return gtk_tree_model_get_path (model, &gap_iter);
	}

	return gtk_tree_model_get_path (model, iter);
}

static void
brasero_audio_disc_move_to_first_pos (BraseroAudioDisc *disc,
				      GtkTreeSelection *selection,
				      GtkTreeModel *model,
				      GtkTreeIter *iter,
				      gboolean is_gap)
{
	GtkTreeIter position;
	GtkTreeIter gap_iter;
	gboolean has_gap = FALSE;

	gtk_tree_model_get_iter_first (model, &position);
	if (is_gap) {
		gboolean is_song;

		gtk_tree_model_get (model, &position,
				    SONG_COL, &is_song,
				    -1);

		if (!is_song) {
			brasero_audio_disc_merge_gaps (disc,
						       model,
						       iter,
						       &position);
			return;
		}
	}
	else
		has_gap = brasero_audio_disc_has_gap (disc, iter, &gap_iter);

	gtk_list_store_move_before (GTK_LIST_STORE (model),
				    iter,
				    &position);

	if (has_gap && !gtk_tree_selection_iter_is_selected (selection, &gap_iter))
		gtk_list_store_move_after (GTK_LIST_STORE (model),
					   &gap_iter,
					   iter);
}

static void
brasero_audio_disc_drag_data_received_cb (GtkTreeView *tree,
					  GdkDragContext *drag_context,
					  gint x,
					  gint y,
					  GtkSelectionData *selection_data,
					  guint info,
					  guint time,
					  BraseroAudioDisc *disc)
{
	GtkTreeSelection *selection;
	BraseroDiscResult success;
	gboolean result = TRUE;
	GtkTreePath *treepath;
	GtkTreeModel *model;
	gchar **uri, **uris;
	GtkTreeIter iter;
	gboolean is_song;
	gint pos;

	if (info == TREE_MODEL_ROW) {
		GList *references = NULL;
		GtkTreePath *dest;
		GList *list_iter;
		GList *selected;

		/* this signal gets emitted during dragging */
		if (disc->priv->dragging) {
			gdk_drag_status (drag_context,
					 GDK_ACTION_MOVE,
					 time);
			g_signal_stop_emission_by_name (tree, "drag-data-received");
			return;
		}

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc->priv->tree));
		selected = gtk_tree_selection_get_selected_rows (selection, &model);

		for (list_iter = selected; list_iter; list_iter = list_iter->next) {
			GtkTreeRowReference *reference;

			reference = gtk_tree_row_reference_new (model, list_iter->data);
			references = g_list_prepend (references, reference);
			gtk_tree_path_free (list_iter->data);
		}
		g_list_free (selected);

		pos = brasero_audio_disc_get_dest_path (disc, x, y);
		if (pos != -1) {
			references = g_list_reverse (references);
			dest = 	gtk_tree_path_new_from_indices (pos, -1);
		}
		else
			dest = NULL;

		for (list_iter = references; list_iter; list_iter = list_iter->next) {
			treepath = gtk_tree_row_reference_get_path (list_iter->data);
			gtk_tree_row_reference_free (list_iter->data);

			if (!gtk_tree_model_get_iter (model, &iter, treepath)) {
				gtk_tree_path_free (treepath);
				continue;
			}
			gtk_tree_path_free (treepath);

			gtk_tree_model_get (model, &iter,
					    SONG_COL, &is_song,
					    -1);

			if (dest)
				dest = brasero_audio_disc_move_to_dest (disc,
									selection,
									model,
									dest,
									&iter,
									is_song == FALSE);
			else
				brasero_audio_disc_move_to_first_pos (disc,
								      selection,
								      model,
								      &iter,
								      is_song == FALSE);
		}
		g_list_free (references);

		if (dest)
			gtk_tree_path_free (dest);

		gtk_drag_finish (drag_context,
				 TRUE,
				 FALSE,
				 time);
		g_signal_stop_emission_by_name (tree, "drag-data-received");

		brasero_audio_disc_re_index_track_num (disc, model);
		return;
	}

	if (selection_data->length <= 0
	||  selection_data->format != 8) {
		gtk_drag_finish (drag_context, FALSE, FALSE, time);
		return;
	}

	/* get pos and URIS */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc->priv->tree));
	gtk_tree_selection_unselect_all (selection);

	pos = brasero_audio_disc_get_dest_path (disc, x, y);
	pos ++;

	uris = gtk_selection_data_get_uris (selection_data);
	for (uri = uris; *uri != NULL; uri++) {
		treepath = NULL;
		success = brasero_audio_disc_add_uri_real (disc,
							   *uri,
							   pos,
							   0,
							   &treepath);
		if (success == BRASERO_DISC_OK) {
			pos ++;
			if (treepath) {
				gtk_tree_selection_select_path (selection, treepath);
				gtk_tree_path_free (treepath);
			}
		}

		result = result ? (success == BRASERO_DISC_OK) : FALSE;
		g_free (*uri);
	}
	g_free (uris);

	gtk_drag_finish (drag_context,
			 result,
			 (drag_context->action == GDK_ACTION_MOVE),
			 time);
}

static gboolean
brasero_audio_disc_drag_drop_cb (GtkWidget *widget,
				 GdkDragContext*drag_context,
				 gint x,
				 gint y,
				 guint time,
				 BraseroAudioDisc *disc)
{
	disc->priv->dragging = 0;
	return FALSE;
}

static void
brasero_audio_disc_drag_leave_cb (GtkWidget *tree,
				  GdkDragContext *drag_context,
				  guint time,
				  BraseroAudioDisc *disc)
{
	if (disc->priv->drag_context) {
		GdkPixbuf *pixbuf;
		GdkPixmap *pixmap;
		GdkBitmap *mask;

		pixbuf = gtk_widget_render_icon (tree,
						 GTK_STOCK_DELETE,
						 GTK_ICON_SIZE_DND,
						 NULL);
		gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf,
								gtk_widget_get_colormap (tree),
								&pixmap,
								&mask,
								128);
		g_object_unref (pixbuf);

		gtk_drag_set_icon_pixmap (disc->priv->drag_context,
					  gdk_drawable_get_colormap (pixmap),
					  pixmap,
					  mask,
					  -2,
					  -2);
	}
}

static gboolean
brasero_audio_disc_drag_motion_cb (GtkWidget *tree,
				   GdkDragContext *drag_context,
				   gint x,
				   gint y,
				   guint time,
				   BraseroAudioDisc *disc)
{
	GdkAtom target;

	target = gtk_drag_dest_find_target (tree,
					    drag_context,
					    gtk_drag_dest_get_target_list(tree));

	if (target == gdk_atom_intern ("GTK_TREE_MODEL_ROW", FALSE)
	&&  disc->priv->drag_context)
		gtk_drag_set_icon_default (disc->priv->drag_context);

	return FALSE;
}

static void
brasero_audio_disc_drag_begin_cb (GtkWidget *widget,
				  GdkDragContext *drag_context,
				  BraseroAudioDisc *disc)
{
	disc->priv->drag_context = drag_context;
	g_object_ref (drag_context);

	disc->priv->dragging = 1;
}

static void
brasero_audio_disc_drag_end_cb (GtkWidget *tree,
			        GdkDragContext *drag_context,
			        BraseroAudioDisc *disc)
{
	gint x, y;

	if (disc->priv->drag_context) {
		g_object_unref (disc->priv->drag_context);
		disc->priv->drag_context = NULL;
	}

	gtk_widget_get_pointer (tree, &x, &y);
	if (x < 0 || y < 0 || x > tree->allocation.width || y > tree->allocation.height)
		brasero_audio_disc_delete_selected (BRASERO_DISC (disc));
}

/********************************** Cell Editing *******************************/
static void
brasero_audio_disc_display_edited_cb (GtkCellRendererText *renderer,
				      gchar *path_string,
				      gchar *text,
				      BraseroAudioDisc *disc)
{
	GtkTreePath *treepath;
	GtkTreeModel *model;
	GtkTreeIter row;
	gint col_num;

	col_num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), COL_KEY));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	treepath = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &row, treepath);
	gtk_list_store_set (GTK_LIST_STORE (model), &row,
			    col_num, text, -1);
	disc->priv->editing = 0;
}

static void
brasero_audio_disc_display_editing_started_cb (GtkCellRenderer *renderer,
					       GtkCellEditable *editable,
					       gchar *path,
					       BraseroAudioDisc *disc)
{
	disc->priv->editing = 1;
}

static void
brasero_audio_disc_display_editing_canceled_cb (GtkCellRenderer *renderer,
						BraseroAudioDisc *disc)
{
	disc->priv->editing = 0;
}

/********************************** Pause **************************************/
static void
brasero_audio_disc_add_pause (BraseroAudioDisc *disc)
{
	GtkTreeSelection *selection;
	GtkTreeRowReference *ref;
	GtkTreePath *treepath;
	GtkTreeModel *model;
	GList *references = NULL;
	GList *selected;
	GList *iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc->priv->tree));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	selected = gtk_tree_selection_get_selected_rows (selection, &model);

	/* since we are going to modify the model, we need to convert all these
	 * into row references */
	for (iter = selected; iter; iter = iter->next) {
		treepath = iter->data;
		ref = gtk_tree_row_reference_new (model, treepath);
		references = g_list_append (references, ref);
	}

	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);

	for (iter = references; iter; iter = iter->next) {
		GtkTreeIter row;

		ref = iter->data;
		treepath = gtk_tree_row_reference_get_path (ref);
		gtk_tree_row_reference_free (ref);

		if (gtk_tree_model_get_iter (model, &row, treepath)) {
			gboolean is_song;

			gtk_tree_model_get (model, &row,
					    SONG_COL, &is_song,
					    -1);

			if (is_song)
				brasero_audio_disc_add_gap (disc, &row, 2 * GST_SECOND);
		}

		gtk_tree_path_free (treepath);
	}

	g_list_free (references);
}

static void
brasero_audio_disc_pause_clicked_cb (GtkButton *button,
				     BraseroAudioDisc *disc)
{
	brasero_audio_disc_add_pause (disc);
}
static void
brasero_audio_disc_add_pause_cb (GtkAction *action,
				 BraseroAudioDisc *disc)
{
	brasero_audio_disc_add_pause (disc);
}

static void
brasero_audio_disc_selection_changed (GtkTreeSelection *selection,
				      GtkWidget *pause_button)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GList *selected;
	GList *iter;

	treeview = gtk_tree_selection_get_tree_view (selection);
	selected = gtk_tree_selection_get_selected_rows (selection, &model);

	gtk_widget_set_sensitive (pause_button, FALSE);
	for (iter = selected; iter; iter = iter->next) {
		GtkTreeIter row;
		GtkTreePath *treepath;

		treepath = iter->data;
		if (gtk_tree_model_get_iter (model, &row, treepath)) {
			gboolean is_song;

			gtk_tree_model_get (model, &row,
					    SONG_COL, &is_song,
					    -1);
			if (is_song) {
				gtk_widget_set_sensitive (pause_button, TRUE);
				break;
			}
		}
	}

	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);
}

/********************************** Menus **************************************/
static void
brasero_audio_disc_open_file (BraseroAudioDisc *disc)
{
	char *uri;
	gboolean success;
	GtkTreeIter iter;
	GList *item, *list;
	GSList *uris = NULL;
	GtkTreeModel *model;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc->priv->tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	for (item = list; item; item = item->next) {
		treepath = item->data;
		success = gtk_tree_model_get_iter (model, &iter, treepath);
		gtk_tree_path_free (treepath);

		if (!success)
			continue;

		gtk_tree_model_get (model, &iter,
				    URI_COL, &uri, -1);

		uris = g_slist_prepend (uris, uri);
	}
	g_list_free (list);

	brasero_utils_launch_app (GTK_WIDGET (disc), uris);
	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

static void
brasero_audio_disc_edit_song_properties (BraseroAudioDisc *disc,
					 GList *list)
{
	gint isrc;
	gint64 gap;
	GList *item;
	gchar *title;
	gchar *artist;
	gint track_num;
	gchar *composer;
	GtkWidget *props;
	gboolean success;
	GtkTreeIter iter;
	GtkWidget *toplevel;
	GtkTreeModel *model;
	GtkTreePath *treepath;
	GtkResponseType result;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	props = brasero_song_props_new ();

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (disc));
	gtk_window_set_transient_for (GTK_WINDOW (props),
				      GTK_WINDOW (toplevel));
	gtk_window_set_modal (GTK_WINDOW (props), TRUE);
	gtk_window_set_position (GTK_WINDOW (props),
				 GTK_WIN_POS_CENTER_ON_PARENT);

	for (item = list; item; item = item->next) {
		GtkTreeIter gap_iter;
		gchar *track_num_str;
		gboolean is_song;
		gchar *markup;

		treepath = item->data;
		success = gtk_tree_model_get_iter (model, &iter, treepath);

		if (!success)
			continue;

		gtk_tree_model_get (model, &iter,
				    SONG_COL, &is_song,
				    -1);
		if (!is_song)
			continue;

		gtk_tree_model_get (model, &iter,
				    NAME_COL, &title,
				    ARTIST_COL, &artist,
				    COMPOSER_COL, &composer,
				    TRACK_NUM_COL, &track_num_str,
				    ISRC_COL, &isrc,
				    -1);

		if (brasero_audio_disc_has_gap (disc, &iter, &gap_iter)) {
			gtk_tree_model_get (model, &gap_iter,
					    SECTORS_COL, &gap,
					    -1);
			gap = BRASERO_SECTORS_TO_TIME (gap);
		}
		else
			gap = 0;

		gtk_widget_show_all (GTK_WIDGET (props));

		if (track_num_str) {
			track_num = (gint) g_strtod (track_num_str + 6 /* (ignore markup) */, NULL);
			g_free (track_num_str);
		}
		else
			track_num = 0;

		brasero_song_props_set_properties (BRASERO_SONG_PROPS (props),
						   track_num,
						   artist,
						   title,
						   composer,
						   isrc,
						   gap);

		if (artist)
			g_free (artist);
		if (title)
			g_free (title);
		if (composer)
			g_free (composer);

		result = gtk_dialog_run (GTK_DIALOG (props));
		gtk_widget_hide (GTK_WIDGET (props));

		if (result != GTK_RESPONSE_ACCEPT)
			break;

		brasero_song_props_get_properties (BRASERO_SONG_PROPS (props),
						   &artist,
						   &title,
						   &composer,
						   &isrc,
						   &gap);

		markup = g_markup_escape_text (title, -1);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    NAME_COL, markup,
				    ARTIST_COL, artist,
				    COMPOSER_COL, composer,
				    ISRC_COL, isrc,
				    -1);
		g_free (markup);

		if (gap)
			brasero_audio_disc_add_gap (disc, &iter, gap);

		g_free (artist);
		g_free (title);
		g_free (composer);
	}

	gtk_widget_destroy (props);
}

static void
brasero_audio_disc_open_activated_cb (GtkAction *action,
				      BraseroAudioDisc *disc)
{
	brasero_audio_disc_open_file (disc);
}

static void
brasero_audio_disc_edit_information_cb (GtkAction *action,
					BraseroAudioDisc *disc)
{
	GList *list;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc->priv->tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	brasero_audio_disc_edit_song_properties (disc, list);

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static void
brasero_audio_disc_delete_activated_cb (GtkAction *action,
					BraseroDisc *disc)
{
	brasero_audio_disc_delete_selected (disc);
}

static void
brasero_audio_disc_clipboard_text_cb (GtkClipboard *clipboard,
				      const char *text,
				      BraseroAudioDisc *disc)
{
	gchar **array;
	gchar **item;
	gchar *uri;

	array = g_strsplit_set (text, "\n\r", 0);
	item = array;
	while (*item) {
		if (**item != '\0') {
			uri = gnome_vfs_make_uri_from_input (*item);
			brasero_audio_disc_add_uri_real (disc,
							 uri,
							 -1,
							 0,
							 NULL);
			g_free (uri);
		}

		item++;
	}
}

static void
brasero_audio_disc_clipboard_targets_cb (GtkClipboard *clipboard,
					 GdkAtom *atoms,
					 gint n_atoms,
					 BraseroAudioDisc *disc)
{
	GdkAtom *iter;
	gchar *target;

	iter = atoms;
	while (n_atoms) {
		target = gdk_atom_name (*iter);

		if (!strcmp (target, "x-special/gnome-copied-files")
		||  !strcmp (target, "UTF8_STRING")) {
			gtk_clipboard_request_text (clipboard,
						    (GtkClipboardTextReceivedFunc) brasero_audio_disc_clipboard_text_cb,
						    disc);
			g_free (target);
			return;
		}

		g_free (target);
		iter++;
		n_atoms--;
	}
}

static void
brasero_audio_disc_paste_activated_cb (GtkAction *action,
				       BraseroAudioDisc *disc)
{
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_request_targets (clipboard,
				       (GtkClipboardTargetsReceivedFunc) brasero_audio_disc_clipboard_targets_cb,
				       disc);
}

static gboolean
brasero_audio_disc_button_pressed_cb (GtkTreeView *tree,
				      GdkEventButton *event,
				      BraseroAudioDisc *disc)
{
	GtkWidgetClass *widget_class;

	/* we call the default handler for the treeview before everything else
	 * so it can update itself (paticularly its selection) before we have
	 * a look at it */
	widget_class = GTK_WIDGET_GET_CLASS (tree);
	widget_class->button_press_event (GTK_WIDGET (tree), event);

	if (event->button == 3) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (tree);
		brasero_utils_show_menu (gtk_tree_selection_count_selected_rows (selection),
					 disc->priv->manager,
					 event);
		return TRUE;
	}
	else if (event->button == 1) {
		gboolean result;
		GtkTreePath *treepath = NULL;

		result = gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (disc->priv->tree),
							event->x,
							event->y,
							&treepath,
							NULL,
							NULL,
							NULL);
		if (!result || !treepath)
			return FALSE;

		if (disc->priv->selected_path)
			gtk_tree_path_free (disc->priv->selected_path);

		disc->priv->selected_path = treepath;
		brasero_disc_selection_changed (BRASERO_DISC (disc));

		if (event->type == GDK_2BUTTON_PRESS) {
			GList *list;

			list = g_list_prepend (NULL, treepath);
			brasero_audio_disc_edit_song_properties (disc, list);
			g_list_free (list);
		}
	}

	return TRUE;
}

/********************************** key press event ****************************/
static void
brasero_audio_disc_rename_activated (BraseroAudioDisc *disc)
{
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkTreePath *treepath;
	GtkTreeModel *model;
	GList *list;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (disc->priv->tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	for (; list; list = g_list_remove (list, treepath)) {
		treepath = list->data;

		gtk_widget_grab_focus (disc->priv->tree);
		column = gtk_tree_view_get_column (GTK_TREE_VIEW (disc->priv->tree), 0);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (disc->priv->tree),
					      treepath,
					      NULL,
					      TRUE,
					      0.5,
					      0.5);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (disc->priv->tree),
					  treepath,
					  column,
					  TRUE);

		gtk_tree_path_free (treepath);
	}
}

static gboolean
brasero_audio_disc_key_released_cb (GtkTreeView *tree,
				    GdkEventKey *event,
				    BraseroAudioDisc *disc)
{
	if (disc->priv->editing)
		return FALSE;

	if (event->keyval == GDK_KP_Delete || event->keyval == GDK_Delete) {
		brasero_audio_disc_delete_selected (BRASERO_DISC (disc));
	}
	else if (event->keyval == GDK_F2)
		brasero_audio_disc_rename_activated (disc);

	return FALSE;
}

/**********************************               ******************************/
static gchar *
brasero_audio_disc_get_selected_uri (BraseroDisc *disc)
{
	gchar *uri;
	GtkTreeIter iter;
	GtkTreeModel *model;
	BraseroAudioDisc *audio;

	audio = BRASERO_AUDIO_DISC (disc);
	if (!audio->priv->selected_path)
		return NULL;

	/* we are asked for just one uri so return the first one */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (audio->priv->tree));
	if (!gtk_tree_model_get_iter (model, &iter, audio->priv->selected_path)) {
		gtk_tree_path_free (audio->priv->selected_path);
		audio->priv->selected_path = NULL;
		return NULL;
	}

	gtk_tree_model_get (model, &iter, URI_COL, &uri, -1);
	return uri;
}

/********************************** Monitoring *********************************/
#ifdef BUILD_INOTIFY

struct _BraseroMonitoredRemoveData {
	BraseroAudioDisc *disc;
	gchar *uri;
};
typedef struct _BraseroMonitoredRemoveData BraseroMonitoredRemoveData;

struct _BraseroMonitoredDir {
	gint ref;
	gchar *uri;
};
typedef struct _BraseroMonitoredDir BraseroMonitoredDir;

static void
brasero_audio_disc_inotify_free_monitored (gpointer data)
{
	BraseroMonitoredDir *dir = data;

	g_free (dir->uri);
	g_free (dir);
}

static void
brasero_audio_disc_start_monitoring (BraseroAudioDisc *disc,
				     const gchar *uri)
{
	if (!disc->priv->monitored)
		disc->priv->monitored = g_hash_table_new_full (g_direct_hash,
							       g_direct_equal,
							       NULL,
							       brasero_audio_disc_inotify_free_monitored);

	if (disc->priv->notify && !strncmp (uri, "file://", 7)) {
		BraseroMonitoredDir *dir;
		gchar *parent;
		int dev_fd;
		char *path;
		__u32 mask;
		int wd;

		/* we want to be able to catch files being renamed in the same
		 * directory that's why we watch the parent directory instead 
		 * of the file itself. Another advantage is that it saves wds
		 * if several files are in the same directory. */
		parent = g_path_get_dirname (uri);
		dir = g_hash_table_lookup (disc->priv->monitored, parent);
		if (dir) {
			/* no need to add a watch, parent directory
			 * is already being monitored */
			g_free (parent);
			dir->ref ++;
			return;
		}

		dir = g_new0 (BraseroMonitoredDir, 1);
		dir->uri = parent;
		dir->ref = 1;

		dev_fd = g_io_channel_unix_get_fd (disc->priv->notify);
		mask = IN_MODIFY |
		       IN_ATTRIB |
		       IN_MOVED_FROM |
		       IN_MOVED_TO |
		       IN_DELETE |
		       IN_DELETE_SELF |
		       IN_MOVE_SELF |
		       IN_UNMOUNT;

	    	path = gnome_vfs_get_local_path_from_uri (parent);
		wd = inotify_add_watch (dev_fd, path, mask);
		if (wd == -1) {
			g_warning ("ERROR creating watch for local file %s : %s\n",
				   parent,
				   strerror (errno));

			g_free (parent);
			g_free (dir);
		}
		else
			g_hash_table_insert (disc->priv->monitored,
					     GINT_TO_POINTER (wd),
					     dir);
		g_free (path);
	}
}

static gboolean
_foreach_monitored_remove_uri_cb (int wd,
				  BraseroMonitoredDir *dir,
				  BraseroMonitoredRemoveData *data)
{
	gint dev_fd;

	if (strcmp (data->uri, dir->uri))
		return FALSE;

	dir->ref --;
	if (dir->ref)
		return FALSE;

	/* we can now safely remove the watch */
	dev_fd = g_io_channel_unix_get_fd (data->disc->priv->notify);
	inotify_rm_watch (dev_fd, wd);
	return TRUE;
}

static void
brasero_audio_disc_cancel_monitoring (BraseroAudioDisc *disc,
				      const char *uri)
{
	BraseroMonitoredRemoveData callback_data;

	if (!disc->priv->notify)
		return;

	callback_data.uri = g_path_get_dirname (uri);
	callback_data.disc = disc;

	if (disc->priv->monitored)
		g_hash_table_foreach_remove (disc->priv->monitored,
					     (GHRFunc) _foreach_monitored_remove_uri_cb,
					     &callback_data);

	g_free (callback_data.uri);
}

static void
brasero_audio_disc_inotify_removal_warning (BraseroAudioDisc *disc,
					    const gchar *uri)
{
	gchar *name;
	GtkWidget *dialog;
	GtkWidget *toplevel;

	BRASERO_GET_BASENAME_FOR_DISPLAY (uri, name);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (disc));
	dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
					 GTK_DIALOG_DESTROY_WITH_PARENT |
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_CLOSE,
					 _("File \"%s\" was removed from the file system:"),
					 name);
	g_free (name);

	gtk_window_set_title (GTK_WINDOW (dialog), _("File deletion noticed"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("it will be removed from the project."));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
brasero_audio_disc_inotify_remove_all (BraseroAudioDisc *disc,
				       BraseroMonitoredDir *dir,
				       gint32 wd)
{
	gint len;
	gint dev_fd;
	GtkTreeIter iter;
	GtkTreeModel *model;

	brasero_audio_disc_inotify_removal_warning (disc, dir->uri);

	/* it's the same as below except that we remove all children of uri */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	if (gtk_tree_model_get_iter_first (model, &iter))
		return;

	len = strlen (dir->uri);
	do {
		gint64 sectors;
		gchar *row_uri;

		gtk_tree_model_get (model, &iter,
				    URI_COL, &row_uri,
				    SECTORS_COL, &sectors,
				    -1);

		if (row_uri && !strncmp (row_uri, dir->uri, len)) {
			if (sectors > 0)
				disc->priv->sectors -= sectors;

			if (!gtk_list_store_remove (GTK_LIST_STORE (model), &iter)) {
				g_free (row_uri);
				break;
			}
		}

		g_free (row_uri);
	} while (gtk_tree_model_iter_next (model, &iter));

	brasero_audio_disc_size_changed (disc);

	/* remove the monitored directory and stop the watch */
	g_hash_table_remove (disc->priv->monitored, GINT_TO_POINTER (wd));

	dev_fd = g_io_channel_unix_get_fd (disc->priv->notify);
	inotify_rm_watch (dev_fd, wd);
}

static GSList *
brasero_audio_disc_inotify_find_rows (BraseroAudioDisc *disc,
				      const char *uri)
{
	GtkTreePath *treepath;
	GtkTreeModel *model;
	GSList *list = NULL;
	GtkTreeIter iter;
	gchar *row_uri;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return NULL;

	do {
		gtk_tree_model_get (model, &iter,
				    URI_COL, &row_uri,
				    -1);

		if (row_uri && !strcmp (uri, row_uri)) {
			treepath = gtk_tree_model_get_path (model, &iter);

			/* NOTE: prepend is better here since last found will be
			 * the first in the list. That way when deleting rows the
			 * paths we delete last will be valid since they are at 
			 * the top in treeview */
			list = g_slist_prepend (list, treepath);
		}

		g_free (row_uri);
	} while (gtk_tree_model_iter_next (model, &iter));

	return list;
}

static void
brasero_audio_disc_inotify_remove (BraseroAudioDisc *disc,
				   const char *uri)
{
	GtkTreeModel *model;
	GSList *list,*iter_list;

	list = brasero_audio_disc_inotify_find_rows (disc, uri);
	if (!list)
		return;

	brasero_audio_disc_inotify_removal_warning (disc, uri);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	for (iter_list = list; iter_list; iter_list = iter_list->next) {
		GtkTreePath *treepath;
		GtkTreeIter iter;
		gint64 sectors;

		treepath = iter_list->data;

		gtk_tree_model_get_iter (model, &iter, treepath);
		gtk_tree_model_get (model, &iter,
				    SECTORS_COL, &sectors,
				    -1);

		if (sectors > 0)
			disc->priv->sectors -= sectors;

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	}

	g_slist_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_slist_free (list);

	brasero_audio_disc_size_changed (disc);
	brasero_audio_disc_cancel_monitoring (disc, uri);
}

static void
brasero_audio_disc_inotify_modify (BraseroAudioDisc *disc,
				   const char *uri,
				   BraseroMetadata *metadata)
{
	GSList *list, *list_iter;
	GtkTreeModel *model;
	gint64 sectors;

	list = brasero_audio_disc_inotify_find_rows (disc, uri);
	if (!list)
		return;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	for (list_iter = list; list_iter; list_iter = list_iter->next) {
		GtkTreePath *treepath;
		GtkTreeIter iter;

		treepath = list_iter->data;

		gtk_tree_model_get_iter (model, &iter, treepath);
		gtk_tree_model_get (model, &iter,
				    SECTORS_COL, &sectors,
				    -1);

		if (sectors > 0)
			disc->priv->sectors -= sectors;

		disc->priv->sectors += BRASERO_TIME_TO_SECTORS (metadata->len);
		brasero_audio_disc_set_row_from_metadata (disc,
							  model,
							  &iter,
							  metadata);
	}

	brasero_audio_disc_size_changed (disc);

	g_slist_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_slist_free (list);
}

static gboolean
brasero_audio_disc_inotify_move_timeout (BraseroAudioDisc *audio)
{
	BraseroInotifyMovedData *data;

	/* an IN_MOVED_FROM timed out. It is the first in the queue. */
	data = audio->priv->moved_list->data;
	audio->priv->moved_list = g_slist_remove (audio->priv->moved_list, data);
	brasero_audio_disc_inotify_remove (audio, data->uri);

	/* clean up */
	g_free (data->uri);
	g_free (data);
	
	return FALSE;
}

static void
brasero_audio_disc_inotify_move (BraseroAudioDisc *disc,
				 struct inotify_event *event,
				 const gchar *uri)
{
	BraseroInotifyMovedData *data = NULL;

	if (!event->cookie) {
		brasero_audio_disc_inotify_remove (disc, uri);
		return;
	}

	if (event->mask & IN_MOVED_FROM) {
		data = g_new0 (BraseroInotifyMovedData, 1);
		data->cookie = event->cookie;
		data->uri = g_strdup (uri);
			
		/* we remember this move for 5s. If 5s later we haven't received
		 * a corresponding MOVED_TO then we consider the file was
		 * removed. */
		data->id = g_timeout_add (5000,
					  (GSourceFunc) brasero_audio_disc_inotify_move_timeout,
					  disc);

		/* NOTE: the order is important, we _must_ append them */
		disc->priv->moved_list = g_slist_append (disc->priv->moved_list, data);
	}
	else {
		GSList *iter;

		for (iter = disc->priv->moved_list; iter; iter = iter->next) {
			data = iter->data;
			if (data->cookie == event->cookie)
				break;
		}

		if (data) {
			GSList *paths, *list_iter;
			GtkTreeModel *model;
			gchar *old_name;
			gchar *new_name;

			/* we've got one match:
			 * - remove from the list
			 * - remove the timeout
			 * - change all the uris with the new one */
			disc->priv->moved_list = g_slist_remove (disc->priv->moved_list,
								 data);

			paths = brasero_audio_disc_inotify_find_rows (disc, data->uri);
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));

			old_name = g_path_get_basename (data->uri);
			new_name = g_path_get_basename (uri);

			for (list_iter = paths; list_iter; list_iter = list_iter->next) {
				GtkTreePath *treepath;
				GtkTreeIter iter;
				gchar *name;

				treepath = list_iter->data;
				gtk_tree_model_get_iter (model, &iter, treepath);
				gtk_tree_model_get (model, &iter,
						    NAME_COL, &name,
						    -1);

				/* see if the user changed the name or if it is
				 * displaying song name metadata. Basically if 
				 * the name is the same as the old one we change
				 * it to the new one. */
				if (!strcmp (name, old_name)) {
					gchar *markup;

					markup = g_markup_escape_text (new_name, -1);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							    NAME_COL, markup,
							    -1);
					g_free (markup);
				}
				g_free (name);

				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						    URI_COL, uri,
						    -1);
			}

			/* clean up the mess */
			g_slist_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
			g_slist_free (paths);

			g_source_remove (data->id);
			g_free (data->uri);
			g_free (data);

			g_free (old_name);
			g_free (new_name);
		}
	}
}

static void
brasero_audio_disc_inotify_attributes_changed_cb (BraseroVFS *vfs,
						  GObject *obj,
						  GnomeVFSResult result,
						  const gchar *uri,
						  const GnomeVFSFileInfo *info,
						  gpointer null_data)
{
	gboolean readable;
	BraseroAudioDisc *disc = BRASERO_AUDIO_DISC (obj);

	if (result != GNOME_VFS_OK)
		readable = FALSE;
	else if (!GNOME_VFS_FILE_INFO_LOCAL (info))
		readable = TRUE;
	else if (getuid () == info->uid
	     && (info->permissions & GNOME_VFS_PERM_USER_READ))
		readable = TRUE;
	else if (getgid () == info->gid
	     && (info->permissions & GNOME_VFS_PERM_GROUP_READ))
		readable = TRUE;
	else if (info->permissions & GNOME_VFS_PERM_OTHER_READ)
		readable = TRUE;
	else
		readable = FALSE;

	if (!readable)
		brasero_audio_disc_inotify_remove (disc, uri);
}

static gboolean
brasero_audio_disc_inotify_attributes_changed (BraseroAudioDisc *disc,
					       const gchar *uri)
{
	gboolean result;
	GList *uris;

	if (!disc->priv->vfs)
		disc->priv->vfs = brasero_vfs_get_default ();

	if (!disc->priv->attr_changed)
		disc->priv->attr_changed = brasero_vfs_register_data_type (disc->priv->vfs,
									   G_OBJECT (disc),
									   G_CALLBACK (brasero_audio_disc_inotify_attributes_changed_cb),
									   brasero_audio_disc_vfs_operation_finished);

	brasero_audio_disc_increase_activity_counter (disc);

	uris = g_list_prepend (NULL, (gchar *) uri);
	result = brasero_vfs_get_info (disc->priv->vfs,
				       uris,
				       FALSE,
				       GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS |
				       GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
				       disc->priv->attr_changed,
				       NULL);
	g_list_free (uris);

	return result;
}

static gboolean
brasero_audio_disc_inotify_is_in_selection (BraseroAudioDisc *disc,
					    const char *uri)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (disc->priv->tree));
	if (gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;

	do {
		gchar *row_uri;

		gtk_tree_model_get (model, &iter,
				    URI_COL, &row_uri,
				    -1);

		if (!strcmp (uri, row_uri)) {
			g_free (row_uri);
			return TRUE;
		}

		g_free (row_uri);
	} while (gtk_tree_model_iter_next (model, &iter));

	return FALSE;
}

static gboolean
brasero_audio_disc_inotify_monitor_cb (GIOChannel *channel,
				       GIOCondition condition,
				       BraseroAudioDisc *disc)
{
	struct inotify_event event;
	BraseroMonitoredDir *dir;
	GError *err = NULL;
	GIOStatus status;
	gchar *monitored;
	gchar *name;
	guint size;

	while (condition & G_IO_IN) {
		monitored = NULL;

		status = g_io_channel_read_chars (channel,
						  (char *) &event,
						  sizeof (struct inotify_event),
						  &size,
						  &err);

		if (status == G_IO_STATUS_EOF)
			return TRUE;

		if (event.len) {
			name = g_new (char, event.len + 1);
			name[event.len] = '\0';
			status = g_io_channel_read_chars (channel,
							  name,
							  event.len,
							  &size,
							  &err);
			if (status != G_IO_STATUS_NORMAL) {
				g_warning ("Error reading inotify: %s\n",
					   err ? "Unknown error" : err->message);
				g_error_free (err);
				return TRUE;
			}
		}
		else
			name = NULL;

		/* look for ignored signal usually following deletion */
		if (event.mask & IN_IGNORED) {
			g_hash_table_remove (disc->priv->monitored,
					     GINT_TO_POINTER (event.wd));

			if (name)
				g_free (name);

			condition = g_io_channel_get_buffer_condition (channel);
			continue;
		}

		dir = g_hash_table_lookup (disc->priv->monitored,
					   GINT_TO_POINTER (event.wd));
		if (!dir) {
			condition = g_io_channel_get_buffer_condition (channel);
			continue;
		}

		if (dir->uri && name) {
			monitored = g_strconcat (dir->uri, "/", name, NULL);
			g_free (name);
		}
		else
			monitored = NULL;

		/* This is a parent directory of at least
		 * one of the files in the selection */
		if (event.mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) {
			/* The parent directory was moved or deleted so there is
			 * no other choice but to remove all the children :( */
			brasero_audio_disc_inotify_remove_all (disc, dir, event.wd);
		}
		else if (event.mask & IN_DELETE) {
			/* a child was deleted */
			brasero_audio_disc_inotify_remove (disc, monitored);
		}
		else if (event.mask & IN_MOVED_FROM) {
			/* a child was moved from the directory or renamed:
			 * wait 5s for a MOVED_TO signals that would mean
			 * it was simply renamed */
			brasero_audio_disc_inotify_move (disc, &event, monitored);
		}
		else if (event.mask & IN_MOVED_TO) {
			/* a file was either moved to this directory or it's a
			 * renaming (see above) */
			brasero_audio_disc_inotify_move (disc, &event, monitored);
		}
		else if (event.mask & IN_ATTRIB) {
			/* a file attributes were changed */
			brasero_audio_disc_inotify_attributes_changed (disc, monitored);
		}
		else if (event.mask & IN_MODIFY
		      &&  brasero_audio_disc_inotify_is_in_selection (disc, monitored)) {
			/* a file was modified */
			BraseroMetadata *metadata;
			GError *error = NULL;

			metadata = brasero_metadata_new (monitored);

			if (!metadata
			||  !brasero_metadata_get_sync (metadata, FALSE, &error)) {
				if (error) {
					g_warning ("ERROR : %s\n", error->message);
					g_error_free (error);
					g_object_unref (metadata);
				}

				brasero_audio_disc_inotify_remove (disc, monitored);
				continue;
			}

			brasero_audio_disc_inotify_modify (disc,
							   monitored,
							   metadata);
			g_object_unref (metadata);
		}

		if (monitored) {
			g_free (monitored);
			monitored = NULL;
		}

		condition = g_io_channel_get_buffer_condition (channel);
	}

	return TRUE;
}
#endif
