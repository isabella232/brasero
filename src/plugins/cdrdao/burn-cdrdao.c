/***************************************************************************
 *            cdrdao.c
 *
 *  dim jan 22 15:38:18 2006
 *  Copyright  2006  Rouquier Philippe
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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include <nautilus-burn-drive.h>

#include "burn-cdrdao.h"
#include "burn-basics.h"
#include "burn-plugin.h"
#include "burn-job.h"
#include "burn-process.h"
#include "brasero-ncb.h"
#include "burn-medium.h"

BRASERO_PLUGIN_BOILERPLATE (BraseroCdrdao, brasero_cdrdao, BRASERO_TYPE_PROCESS, BraseroProcess);

static GObjectClass *parent_class = NULL;

static gboolean
brasero_cdrdao_read_stderr_image (BraseroCdrdao *cdrdao, const gchar *line)
{
	int min, sec, sub, s1;

	if (sscanf (line, "%d:%d:%d", &min, &sec, &sub) == 3) {
		guint64 secs = min * 60 + sec;

		brasero_job_set_written (BRASERO_JOB (cdrdao), secs * 75 * 2352);
		if (secs > 2)
			brasero_job_start_progress (BRASERO_JOB (cdrdao), FALSE);
	}
	else if (sscanf (line, "Leadout %*s %*d %d:%d:%*d(%i)", &min, &sec, &s1) == 3) {
		BraseroJobAction action;

		brasero_job_get_action (BRASERO_JOB (cdrdao), &action);
		if (action == BRASERO_JOB_ACTION_SIZE) {
			/* get the number of sectors. As we added -raw sector = 2352 bytes */
			brasero_job_set_output_size_for_current_track (BRASERO_JOB (cdrdao), s1, s1 * 2352);
			brasero_job_finished (BRASERO_JOB (cdrdao), NULL);
		}
	}
	else if (strstr (line, "Copying audio tracks")) {
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_DRIVE_COPY,
						_("Copying audio track"),
						FALSE);
	}
	else if (strstr (line, "Copying data track")) {
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_DRIVE_COPY,
						_("Copying data track"),
						FALSE);
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
brasero_cdrdao_read_stderr_record (BraseroCdrdao *cdrdao, const gchar *line)
{
	int fifo, track, min, sec;
	guint written, total;

	if (sscanf (line, "Wrote %u of %u (Buffers %d%%  %*s", &written, &total, &fifo) >= 2) {
		brasero_job_set_dangerous (BRASERO_JOB (cdrdao), TRUE);

		brasero_job_set_written (BRASERO_JOB (cdrdao), written * 1048576);
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_RECORDING,
						NULL,
						FALSE);

		brasero_job_start_progress (BRASERO_JOB (cdrdao), FALSE);
	}
	else if (sscanf (line, "Wrote %*s blocks. Buffer fill min") == 1) {
		/* this is for fixating phase */
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_FIXATING,
						NULL,
						FALSE);
	}
	else if (sscanf (line, "Analyzing track %d %*s start %d:%d:%*d, length %*d:%*d:%*d", &track, &min, &sec) == 3) {
		gchar *string;

		string = g_strdup_printf (_("Analysing track %02i"), track);
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_ANALYSING,
						string,
						TRUE);
		g_free (string);
	}
	else if (sscanf (line, "%d:%d:%*d", &min, &sec) == 2) {
		gint64 written;
		guint64 secs = min * 60 + sec;

		if (secs > 2)
			brasero_job_start_progress (BRASERO_JOB (cdrdao), FALSE);

		written = secs * 75 * 2352;
		brasero_job_set_written (BRASERO_JOB (cdrdao), written);
	}
	else if (strstr (line, "Writing track")) {
		brasero_job_set_dangerous (BRASERO_JOB (cdrdao), TRUE);
	}
	else if (strstr (line, "Writing finished successfully")
	     ||  strstr (line, "On-the-fly CD copying finished successfully")) {
		brasero_job_set_dangerous (BRASERO_JOB (cdrdao), FALSE);
	}
	else if (strstr (line, "Blanking disk...")) {
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_BLANKING,
						NULL,
						FALSE);
		brasero_job_start_progress (BRASERO_JOB (cdrdao), FALSE);
		brasero_job_set_dangerous (BRASERO_JOB (cdrdao), TRUE);
	}
	else {
		gchar *cuepath, *name;
		BraseroTrack *track;

		brasero_job_get_current_track (BRASERO_JOB (cdrdao), &track);
		cuepath = brasero_track_get_toc_source (track, FALSE);
		if (!cuepath)
			return FALSE;

		if (!strstr (line, cuepath)) {
			g_free (cuepath);
			return FALSE;
		}

		name = g_path_get_basename (cuepath);
		g_free (cuepath);

		brasero_job_error (BRASERO_JOB (cdrdao),
				   g_error_new (BRASERO_BURN_ERROR,
						BRASERO_BURN_ERROR_GENERAL,
						_("the cue file (%s) seems to be invalid"),
						name));
		g_free (name);
	}

	return TRUE;
}

static BraseroBurnResult
brasero_cdrdao_read_stderr (BraseroProcess *process, const gchar *line)
{
	BraseroCdrdao *cdrdao;
	gboolean result = FALSE;
	BraseroJobAction action;

	cdrdao = BRASERO_CDRDAO (process);

	brasero_job_get_action (BRASERO_JOB (cdrdao), &action);
	if (action == BRASERO_JOB_ACTION_RECORD
	||  action == BRASERO_JOB_ACTION_ERASE)
		result = brasero_cdrdao_read_stderr_record (cdrdao, line);
	else if (action == BRASERO_JOB_ACTION_IMAGE
	     ||  action == BRASERO_JOB_ACTION_SIZE)
		result = brasero_cdrdao_read_stderr_image (cdrdao, line);

	if (result)
		return BRASERO_BURN_OK;

	if (strstr (line, "Cannot setup device")) {
		brasero_job_error (BRASERO_JOB (cdrdao),
				   g_error_new (BRASERO_BURN_ERROR,
						BRASERO_BURN_ERROR_BUSY_DRIVE,
						_("the drive seems to be busy")));
	}
	else if (strstr (line, "Illegal command")) {
		brasero_job_error (BRASERO_JOB (cdrdao),
				   g_error_new (BRASERO_BURN_ERROR,
						BRASERO_BURN_ERROR_GENERAL,
						_("your version of cdrdao doesn't seem to be supported by libbrasero")));
	}
	else if (strstr (line, "Operation not permitted. Cannot send SCSI")) {
		brasero_job_error (BRASERO_JOB (cdrdao),
				   g_error_new (BRASERO_BURN_ERROR,
						BRASERO_BURN_ERROR_SCSI_IOCTL,
						_("You don't seem to have the required permission to use this drive")));
	}

	return BRASERO_BURN_OK;
}

static void
brasero_cdrdao_set_argv_device (BraseroCdrdao *cdrdao,
				GPtrArray *argv)
{
	gchar *device = NULL;

	g_ptr_array_add (argv, g_strdup ("--device"));
	brasero_job_get_device (BRASERO_JOB (cdrdao), &device);
	g_ptr_array_add (argv, device);
}

static void
brasero_cdrdao_set_argv_common_rec (BraseroCdrdao *cdrdao,
				    GPtrArray *argv)
{
	BraseroBurnFlag flags;
	gchar *speed_str;
	guint speed;

	g_ptr_array_add (argv, g_strdup ("--speed"));

	brasero_job_get_speed (BRASERO_JOB (cdrdao), &speed);
	speed_str = g_strdup_printf ("%d", speed);
	g_ptr_array_add (argv, speed_str);

	brasero_job_get_flags (BRASERO_JOB (cdrdao), &flags);
	if (flags & BRASERO_BURN_FLAG_OVERBURN)
		g_ptr_array_add (argv, g_strdup ("--overburn"));
	if (flags & BRASERO_BURN_FLAG_MULTI)
		g_ptr_array_add (argv, g_strdup ("--multi"));
}

static void
brasero_cdrdao_set_argv_common (BraseroCdrdao *cdrdao,
				GPtrArray *argv)
{
	BraseroBurnFlag flags;

	brasero_job_get_flags (BRASERO_JOB (cdrdao), &flags);
	if (flags & BRASERO_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("--simulate"));

	/* cdrdao manual says it is a similar option to gracetime */
	if (flags & BRASERO_BURN_FLAG_NOGRACE)
		g_ptr_array_add (argv, g_strdup ("-n"));

	g_ptr_array_add (argv, g_strdup ("-v"));
	g_ptr_array_add (argv, g_strdup ("2"));
}

static BraseroBurnResult
brasero_cdrdao_set_argv_record (BraseroCdrdao *cdrdao,
				GPtrArray *argv)
{
	BraseroTrackType type;

	brasero_job_get_input_type (BRASERO_JOB (cdrdao), &type);
        if (type.type == BRASERO_TRACK_TYPE_DISC) {
		NautilusBurnDrive *drive;
		BraseroBurnFlag flags;
		BraseroTrack *track;

		g_ptr_array_add (argv, g_strdup ("copy"));
		brasero_cdrdao_set_argv_device (cdrdao, argv);
		brasero_cdrdao_set_argv_common (cdrdao, argv);
		brasero_cdrdao_set_argv_common_rec (cdrdao, argv);

		brasero_job_get_flags (BRASERO_JOB (cdrdao), &flags);
		if (flags & BRASERO_BURN_FLAG_NO_TMP_FILES)
			g_ptr_array_add (argv, g_strdup ("--on-the-fly"));

		g_ptr_array_add (argv, g_strdup ("--source-device"));

		brasero_job_get_current_track (BRASERO_JOB (cdrdao), &track);
		drive = brasero_track_get_drive_source (track);
		g_ptr_array_add (argv, g_strdup (NCB_DRIVE_GET_DEVICE (drive)));
	}
	else if (type.type == BRASERO_TRACK_TYPE_IMAGE) {
		gchar *cuepath;
		BraseroTrack *track;

		brasero_job_get_current_track (BRASERO_JOB (cdrdao), &track);

		if (type.subtype.img_format == BRASERO_IMAGE_FORMAT_CUE)
			cuepath = brasero_track_get_toc_source (track, TRUE);
		else if (type.subtype.img_format == BRASERO_IMAGE_FORMAT_CDRDAO)
			cuepath = brasero_track_get_toc_source (track, TRUE);
		else
			BRASERO_JOB_NOT_SUPPORTED (cdrdao);

		if (!cuepath)
			BRASERO_JOB_NOT_READY (cdrdao);

		g_ptr_array_add (argv, g_strdup ("write"));

		brasero_cdrdao_set_argv_device (cdrdao, argv);
		brasero_cdrdao_set_argv_common (cdrdao, argv);
		brasero_cdrdao_set_argv_common_rec (cdrdao, argv);

		g_ptr_array_add (argv, cuepath);
	}
	else
		BRASERO_JOB_NOT_SUPPORTED (cdrdao);

	brasero_job_set_use_average_rate (BRASERO_JOB (cdrdao), TRUE);
	brasero_job_set_current_action (BRASERO_JOB (cdrdao),
					BRASERO_BURN_ACTION_PREPARING,
					NULL,
					FALSE);
	return BRASERO_BURN_OK;
}

static BraseroBurnResult
brasero_cdrdao_set_argv_blank (BraseroCdrdao *cdrdao,
			       GPtrArray *argv)
{
	BraseroBurnFlag flags;

	g_ptr_array_add (argv, g_strdup ("blank"));

	brasero_cdrdao_set_argv_device (cdrdao, argv);
	brasero_cdrdao_set_argv_common (cdrdao, argv);

	brasero_job_get_flags (BRASERO_JOB (cdrdao), &flags);
	if (!(flags & BRASERO_BURN_FLAG_FAST_BLANK)) {
		g_ptr_array_add (argv, g_strdup ("--blank-mode"));
		g_ptr_array_add (argv, g_strdup ("full"));
	}

	brasero_job_set_current_action (BRASERO_JOB (cdrdao),
					BRASERO_BURN_ACTION_BLANKING,
					NULL,
					FALSE);

	return BRASERO_BURN_OK;
}

static BraseroBurnResult
brasero_cdrdao_set_argv_image (BraseroCdrdao *cdrdao,
			       GPtrArray *argv,
			       GError **error)
{
	gchar *image = NULL, *toc = NULL;
	BraseroBurnResult result;
	NautilusBurnDrive *drive;
	BraseroJobAction action;
	BraseroTrackType output;
	BraseroTrack *track;

	g_ptr_array_add (argv, g_strdup ("read-cd"));
	g_ptr_array_add (argv, g_strdup ("--device"));

	brasero_job_get_current_track (BRASERO_JOB (cdrdao), &track);
	drive = brasero_track_get_drive_source (track);
	g_ptr_array_add (argv, g_strdup (NCB_DRIVE_GET_DEVICE (drive)));

	g_ptr_array_add (argv, g_strdup ("--read-raw"));

	/* This is done so that if a cue file is required we first generate
	 * a temporary toc file that will be later converted to a cue file.
	 * The datafile is written where it should be from the start. */

	brasero_job_get_output_type (BRASERO_JOB (cdrdao), &output);
	if (output.subtype.img_format == BRASERO_IMAGE_FORMAT_CDRDAO) {
		result = brasero_job_get_image_output (BRASERO_JOB (cdrdao),
						       &image,
						       &toc);
		if (result != BRASERO_BURN_OK)
			return result;
	}
	else {
		result = brasero_job_get_image_output (BRASERO_JOB (cdrdao),
						       &image,
						       NULL);
		if (result != BRASERO_BURN_OK)
			return result;
	
		result = brasero_job_get_tmp_file (BRASERO_JOB (cdrdao),
						   &toc,
						   error);
		if (result != BRASERO_BURN_OK)
			return result;
	}

	brasero_job_get_action (BRASERO_JOB (cdrdao), &action);
	if (action == BRASERO_JOB_ACTION_SIZE) {
		brasero_job_set_current_action (BRASERO_JOB (cdrdao),
						BRASERO_BURN_ACTION_GETTING_SIZE,
						NULL,
						FALSE);
		brasero_job_start_progress (BRASERO_JOB (cdrdao), FALSE);
	}

	g_ptr_array_add (argv, g_strdup ("--datafile"));
	g_ptr_array_add (argv, image);

	g_ptr_array_add (argv, g_strdup ("-v"));
	g_ptr_array_add (argv, g_strdup ("2"));

	g_ptr_array_add (argv, toc);

	brasero_job_set_use_average_rate (BRASERO_JOB (cdrdao), TRUE);

	return BRASERO_BURN_OK;
}

static BraseroBurnResult
brasero_cdrdao_set_argv (BraseroProcess *process,
			 GPtrArray *argv,
			 GError **error)
{
	BraseroCdrdao *cdrdao;
	BraseroJobAction action;

	cdrdao = BRASERO_CDRDAO (process);

	/* sets the first argv */
	g_ptr_array_add (argv, g_strdup ("cdrdao"));

	brasero_job_get_action (BRASERO_JOB (cdrdao), &action);
	if (action == BRASERO_JOB_ACTION_RECORD)
		return brasero_cdrdao_set_argv_record (cdrdao, argv);
	else if (action == BRASERO_JOB_ACTION_ERASE)
		return brasero_cdrdao_set_argv_blank (cdrdao, argv);
	else if (action == BRASERO_JOB_ACTION_IMAGE)
		return brasero_cdrdao_set_argv_image (cdrdao, argv, error);
	else if (action == BRASERO_JOB_ACTION_SIZE) {
		BraseroTrack *track;
		gint64 sectors = 0;

		brasero_job_get_current_track (BRASERO_JOB (cdrdao), &track);
		brasero_track_get_disc_data_size (track, &sectors, NULL);
		brasero_job_set_output_size_for_current_track (BRASERO_JOB (cdrdao),
							       sectors,
							       sectors * 2352);
		return BRASERO_BURN_OK;
	}

	BRASERO_JOB_NOT_SUPPORTED (cdrdao);
}

static void
brasero_cdrdao_class_init (BraseroCdrdaoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	BraseroProcessClass *process_class = BRASERO_PROCESS_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = brasero_cdrdao_finalize;

	process_class->stderr_func = brasero_cdrdao_read_stderr;
	process_class->set_argv = brasero_cdrdao_set_argv;
}

static void
brasero_cdrdao_init (BraseroCdrdao *obj)
{  }

static void
brasero_cdrdao_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

G_MODULE_EXPORT GType
brasero_plugin_register (BraseroPlugin *plugin, gchar **error)
{
	GSList *input;
	GSList *output;
	gchar *prog_name;
	const BraseroMedia media_w = BRASERO_MEDIUM_CD|
				     BRASERO_MEDIUM_WRITABLE|
				     BRASERO_MEDIUM_REWRITABLE|
				     BRASERO_MEDIUM_BLANK;
	const BraseroMedia media_rw = BRASERO_MEDIUM_CD|
				      BRASERO_MEDIUM_REWRITABLE|
				      BRASERO_MEDIUM_APPENDABLE|
				      BRASERO_MEDIUM_CLOSED|
				      BRASERO_MEDIUM_HAS_DATA|
				      BRASERO_MEDIUM_HAS_AUDIO;

	/* First see if this plugin can be used, i.e. if readcd is in
	 * the path */
	prog_name = g_find_program_in_path ("cdrdao");
	if (!prog_name) {
		*error = g_strdup (_("cdrdao could not be found in the path"));
		return G_TYPE_NONE;
	}

	g_free (prog_name);

	brasero_plugin_define (plugin,
			       "cdrdao",
			       _("use cdrdao to image and burn CDs"),
			       "Philippe Rouquier",
			       20);

	/* that's for cdrdao images: CDs only as input */
	input = brasero_caps_disc_new (BRASERO_MEDIUM_CD|
				       BRASERO_MEDIUM_ROM|
				       BRASERO_MEDIUM_WRITABLE|
				       BRASERO_MEDIUM_REWRITABLE|
				       BRASERO_MEDIUM_APPENDABLE|
				       BRASERO_MEDIUM_CLOSED|
				       BRASERO_MEDIUM_HAS_AUDIO|
				       BRASERO_MEDIUM_HAS_DATA);

	/* an image can be created ... */
	output = brasero_caps_image_new (BRASERO_PLUGIN_IO_ACCEPT_FILE,
					 BRASERO_IMAGE_FORMAT_CDRDAO);

	brasero_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* ... or a disc */
	output = brasero_caps_disc_new (media_w);
	brasero_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	/* cdrdao can also record these types of images to a disc */
	input = brasero_caps_image_new (BRASERO_PLUGIN_IO_ACCEPT_FILE,
					BRASERO_IMAGE_FORMAT_CDRDAO|
					BRASERO_IMAGE_FORMAT_CUE);
	
	brasero_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	brasero_plugin_set_flags (plugin,
				  media_w,
				  BRASERO_BURN_FLAG_DAO|
				  BRASERO_BURN_FLAG_BURNPROOF|
				  BRASERO_BURN_FLAG_OVERBURN|
				  BRASERO_BURN_FLAG_DUMMY|
				  BRASERO_BURN_FLAG_NOGRACE,
				  BRASERO_BURN_FLAG_NONE);

	/* cdrdao can also blank */
	output = brasero_caps_disc_new (media_rw);
	brasero_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	brasero_plugin_set_blank_flags (plugin,
					media_rw,
					BRASERO_BURN_FLAG_DUMMY|
					BRASERO_BURN_FLAG_NOGRACE|
					BRASERO_BURN_FLAG_FAST_BLANK,
					BRASERO_BURN_FLAG_NONE);

	return brasero_cdrdao_get_type (plugin);
}
