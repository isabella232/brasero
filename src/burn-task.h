/***************************************************************************
 *            burn-task.h
 *
 *  mer sep 13 09:16:29 2006
 *  Copyright  2006  Rouquier Philippe
 *  bonfire-app@wanadoo.fr
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

#ifndef BURN_TASK_H
#define BURN_TASK_H

#include <glib.h>
#include <glib-object.h>

#include "burn-basics.h"
#include "burn-job.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"

G_BEGIN_DECLS

#define BRASERO_TYPE_TASK         (brasero_task_get_type ())
#define BRASERO_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BRASERO_TYPE_TASK, BraseroTask))
#define BRASERO_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BRASERO_TYPE_TASK, BraseroTaskClass))
#define BRASERO_IS_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BRASERO_TYPE_TASK))
#define BRASERO_IS_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BRASERO_TYPE_TASK))
#define BRASERO_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BRASERO_TYPE_TASK, BraseroTaskClass))

typedef struct _BraseroTask BraseroTask;
typedef struct _BraseroTaskClass BraseroTaskClass;

struct _BraseroTask {
	BraseroTaskCtx parent;
};

struct _BraseroTaskClass {
	BraseroTaskCtxClass parent_class;
};

GType brasero_task_get_type ();

BraseroTask *brasero_task_new ();

void
brasero_task_add_item (BraseroTask *task, BraseroTaskItem *item);

void
brasero_task_reset (BraseroTask *task);

BraseroBurnResult
brasero_task_run (BraseroTask *task,
		  GError **error);

BraseroBurnResult
brasero_task_check (BraseroTask *task,
		    GError **error);

BraseroBurnResult
brasero_task_cancel (BraseroTask *task,
		     gboolean protect);

gboolean
brasero_task_is_running (BraseroTask *task);

BraseroBurnResult
brasero_task_get_output_type (BraseroTask *task, BraseroTrackType *output);

G_END_DECLS

#endif /* BURN_TASK_H */
