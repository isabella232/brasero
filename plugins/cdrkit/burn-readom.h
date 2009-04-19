/***************************************************************************
 *            readom.h
 *
 *  dim jan 22 18:06:10 2006
 *  Copyright  2006  Rouquier Philippe
 *  brasero-app@wanadoo.fr
 ***************************************************************************/

/*
 *  Brasero is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Brasero is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef READOM_H
#define READOM_H

#include <glib.h>
#include <glib-object.h>

#include "burn-process.h"

G_BEGIN_DECLS

#define BRASERO_TYPE_READOM         (brasero_readom_get_type ())
#define BRASERO_READOM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BRASERO_TYPE_READOM, BraseroReadom))
#define BRASERO_READOM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BRASERO_TYPE_READOM, BraseroReadomClass))
#define BRASERO_IS_READOM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BRASERO_TYPE_READOM))
#define BRASERO_IS_READOM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BRASERO_TYPE_READOM))
#define BRASERO_READOM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BRASERO_TYPE_READOM, BraseroReadomClass))

G_END_DECLS

#endif /* READOM_H */
