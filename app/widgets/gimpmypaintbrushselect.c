/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmypaint_brushselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmypaintbrush.h"
#include "core/gimpparamspecs.h"

#include "pdb/gimppdb.h"

#include "gimpmypaintbrushfactoryview.h"
#include "gimpmypaintbrushselect.h"
#include "gimpcontainerbox.h"
#include "gimpspinscale.h"
#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_PAINT_MODE,
  PROP_SPACING
};


static void          gimp_mypaint_brush_select_constructed  (GObject         *object);
static void          gimp_mypaint_brush_select_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);

static GValueArray * gimp_mypaint_brush_select_run_callback (GimpPdbDialog   *dialog,
                                                     GimpObject      *object,
                                                     gboolean         closing,
                                                     GError         **error);

static void       gimp_mypaint_brush_select_opacity_changed (GimpContext     *context,
                                                     gdouble          opacity,
                                                     GimpMypaintBrushSelect *select);
static void       gimp_mypaint_brush_select_mode_changed    (GimpContext     *context,
                                                     GimpLayerModeEffects  paint_mode,
                                                     GimpMypaintBrushSelect *select);

static void       gimp_mypaint_brush_select_opacity_update  (GtkAdjustment   *adj,
                                                     GimpMypaintBrushSelect *select);
static void       gimp_mypaint_brush_select_mode_update     (GtkWidget       *widget,
                                                     GimpMypaintBrushSelect *select);
static void       gimp_mypaint_brush_select_spacing_update  (GtkAdjustment   *adj,
                                                     GimpMypaintBrushSelect *select);


G_DEFINE_TYPE (GimpMypaintBrushSelect, gimp_mypaint_brush_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_mypaint_brush_select_parent_class


static void
gimp_mypaint_brush_select_class_init (GimpMypaintBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_mypaint_brush_select_constructed;
  object_class->set_property = gimp_mypaint_brush_select_set_property;

  pdb_class->run_callback    = gimp_mypaint_brush_select_run_callback;
#if 0
  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        GIMP_OPACITY_TRANSPARENT,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PAINT_MODE,
                                   g_param_spec_enum ("paint-mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE_EFFECTS,
                                                      GIMP_NORMAL_MODE,
                                                      GIMP_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_int ("spacing", NULL, NULL,
                                                     -G_MAXINT, 1000, -1,
                                                     GIMP_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
#endif
}

static void
gimp_mypaint_brush_select_init (GimpMypaintBrushSelect *select)
{
}

static void
gimp_mypaint_brush_select_constructed (GObject *object)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GimpMypaintBrushSelect *select = GIMP_MYPAINT_BRUSH_SELECT (object);
  GtkWidget       *content_area;
  GtkWidget       *vbox;
  GtkWidget       *scale;
  GtkWidget       *hbox;
  GtkWidget       *label;
  GtkAdjustment   *spacing_adj;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);
#if 0
  gimp_context_set_opacity    (dialog->context, select->initial_opacity);
  gimp_context_set_paint_mode (dialog->context, select->initial_mode);

  g_signal_connect (dialog->context, "opacity-changed",
                    G_CALLBACK (gimp_mypaint_brush_select_opacity_changed),
                    dialog);
  g_signal_connect (dialog->context, "paint-mode-changed",
                    G_CALLBACK (gimp_mypaint_brush_select_mode_changed),
                    dialog);
#endif

  dialog->view =
    gimp_mypaint_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                 dialog->context->gimp->mypaint_brush_factory,
                                 dialog->context,
                                 GIMP_VIEW_SIZE_MEDIUM, 1,
                                 dialog->menu_factory);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);

  vbox = GTK_WIDGET (GIMP_CONTAINER_EDITOR (dialog->view)->view);

  /*  Create the opacity scale widget  */
#if 0
  select->opacity_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (gimp_context_get_opacity (dialog->context) * 100.0,
                                        0.0, 100.0,
                                        1.0, 10.0, 0.0));

  scale = gimp_spin_scale_new (select->opacity_data,
                               _("Opacity"), 1);
  gtk_box_pack_end (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (select->opacity_data, "value-changed",
                    G_CALLBACK (gimp_mypaint_brush_select_opacity_update),
                    select);

  /*  Create the paint mode option menu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  select->paint_mode_menu = gimp_paint_mode_menu_new (TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), select->paint_mode_menu, TRUE, TRUE, 0);
  gtk_widget_show (select->paint_mode_menu);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (select->paint_mode_menu),
                              gimp_context_get_paint_mode (dialog->context),
                              G_CALLBACK (gimp_mypaint_brush_select_mode_update),
                              select);

  spacing_adj = GIMP_MYPAINT_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment;

  /*  Use passed spacing instead of mypaint_brushes default  */
  if (select->spacing >= 0)
    gtk_adjustment_set_value (spacing_adj, select->spacing);

  g_signal_connect (spacing_adj, "value-changed",
                    G_CALLBACK (gimp_mypaint_brush_select_spacing_update),
                    select);
#endif
}

static void
gimp_mypaint_brush_select_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GimpMypaintBrushSelect *select = GIMP_MYPAINT_BRUSH_SELECT (object);

  switch (property_id)
    {
#if 0
    case PROP_OPACITY:
      if (dialog->view)
        gimp_context_set_opacity (dialog->context, g_value_get_double (value));
      else
        select->initial_opacity = g_value_get_double (value);
      break;
    case PROP_PAINT_MODE:
      if (dialog->view)
        gimp_context_set_paint_mode (dialog->context, g_value_get_enum (value));
      else
        select->initial_mode = g_value_get_enum (value);
      break;
    case PROP_SPACING:
      if (dialog->view)
        {
          if (g_value_get_int (value) >= 0)
            gtk_adjustment_set_value (GIMP_MYPAINT_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment,
                                      g_value_get_int (value));
        }
      else
        {
          select->spacing = g_value_get_int (value);
        }
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GValueArray *
gimp_mypaint_brush_select_run_callback (GimpPdbDialog  *dialog,
                                GimpObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  GimpMypaintBrush   *mypaint_brush = GIMP_MYPAINT_BRUSH (object);
  GimpArray   *array;
  GValueArray *return_vals;
  g_print("gimp_mypaint_brush_select_run_callback\n");
#if 0
  array = gimp_array_new (temp_buf_get_data (mypaint_brush->mask),
                          temp_buf_get_data_size (mypaint_brush->mask),
                          TRUE);

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        G_TYPE_STRING,        gimp_object_get_name (object),
                                        G_TYPE_DOUBLE,        100,//gimp_context_get_opacity (dialog->context) * 100.0,
                                        GIMP_TYPE_INT32,      1,//GIMP_MYPAINT_BRUSH_SELECT (dialog)->spacing,
                                        GIMP_TYPE_INT32,      gimp_context_get_paint_mode (dialog->context),
                                        GIMP_TYPE_INT32,      1,//mypaint_brush->mask->width,
                                        GIMP_TYPE_INT32,      1,//mypaint_brush->mask->height,
                                        GIMP_TYPE_INT32,      array->length,
                                        GIMP_TYPE_INT8_ARRAY, array,
                                        GIMP_TYPE_INT32,      closing,
                                        G_TYPE_NONE);

  gimp_array_free (array);
#else
  return_vals = NULL;
#endif

  return return_vals;
}
#if 0
static void
gimp_mypaint_brush_select_opacity_changed (GimpContext     *context,
                                   gdouble          opacity,
                                   GimpMypaintBrushSelect *select)
{
  g_signal_handlers_block_by_func (select->opacity_data,
                                   gimp_mypaint_brush_select_opacity_update,
                                   select);

  gtk_adjustment_set_value (select->opacity_data, opacity * 100.0);

  g_signal_handlers_unblock_by_func (select->opacity_data,
                                     gimp_mypaint_brush_select_opacity_update,
                                     select);

  gimp_pdb_dialog_run_callback (GIMP_PDB_DIALOG (select), FALSE);
}

static void
gimp_mypaint_brush_select_mode_changed (GimpContext          *context,
                                GimpLayerModeEffects  paint_mode,
                                GimpMypaintBrushSelect      *select)
{
  g_signal_handlers_block_by_func (select->paint_mode_menu,
                                   gimp_mypaint_brush_select_mode_update,
                                   select);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (select->paint_mode_menu),
                                 paint_mode);

  g_signal_handlers_unblock_by_func (select->paint_mode_menu,
                                     gimp_mypaint_brush_select_mode_update,
                                     select);

  gimp_pdb_dialog_run_callback (GIMP_PDB_DIALOG (select), FALSE);
}

static void
gimp_mypaint_brush_select_opacity_update (GtkAdjustment   *adjustment,
                                  GimpMypaintBrushSelect *select)
{
  gimp_context_set_opacity (GIMP_PDB_DIALOG (select)->context,
                            gtk_adjustment_get_value (adjustment) / 100.0);
}

static void
gimp_mypaint_brush_select_mode_update (GtkWidget       *widget,
                               GimpMypaintBrushSelect *select)
{
  gint paint_mode;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                     &paint_mode))
    {
      gimp_context_set_paint_mode (GIMP_PDB_DIALOG (select)->context,
                                   (GimpLayerModeEffects) paint_mode);
    }
}

static void
gimp_mypaint_brush_select_spacing_update (GtkAdjustment   *adjustment,
                                  GimpMypaintBrushSelect *select)
{
  gdouble value = gtk_adjustment_get_value (adjustment);

  if (select->spacing != value)
    {
      select->spacing = value;

      gimp_pdb_dialog_run_callback (GIMP_PDB_DIALOG (select), FALSE);
    }
}
#endif
