/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"

/** @file glamor_polylines.c
 *
 * GC PolyFillRect implementation, taken straight from fb_fill.c
 */

/**
 * glamor_poly_lines() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		  DDXPointPtr points)
{
	xRectangle *rects;
	int x1, x2, y1, y2;
	int i;
	int x_min = MAXSHORT;
	int x_max = MINSHORT;
	int y_min = MAXSHORT;
	int y_max = MINSHORT;
	DrawablePtr temp_dest;
	PixmapPtr temp_pixmap;
	GCPtr temp_gc = NULL;
	/* Don't try to do wide lines or non-solid fill style. */
	if (gc->lineWidth != 0) {
		/* This ends up in miSetSpans, which is accelerated as well as we
		 * can hope X wide lines will be.
		 */
		goto fail;
	}
	if (gc->lineStyle != LineSolid || gc->fillStyle != FillSolid) {
		glamor_fallback
		    ("non-solid fill line style %d, fill style %d\n",
		     gc->lineStyle, gc->fillStyle);
		goto fail;
	}

	rects = malloc(sizeof(xRectangle) * (n - 1));
	x1 = points[0].x;
	y1 = points[0].y;
	/* If we have any non-horizontal/vertical, fall back. */
	for (i = 0; i < n - 1; i++) {
		if (mode == CoordModePrevious) {
			x2 = x1 + points[i + 1].x;
			y2 = y1 + points[i + 1].y;
		} else {
			x2 = points[i + 1].x;
			y2 = points[i + 1].y;
		}
		if (x1 != x2 && y1 != y2) {
			free(rects);
			glamor_fallback("stub diagonal poly_line\n");
			goto fail;
		}
		if (x1 < x2) {
			rects[i].x = x1;
			rects[i].width = x2 - x1 + 1;
		} else {
			rects[i].x = x2;
			rects[i].width = x1 - x2 + 1;
		}
		if (y1 < y2) {
			rects[i].y = y1;
			rects[i].height = y2 - y1 + 1;
		} else {
			rects[i].y = y2;
			rects[i].height = y1 - y2 + 1;
		}

		x1 = x2;
		y1 = y2;
	}
	gc->ops->PolyFillRect(drawable, gc, n - 1, rects);
	free(rects);
	return;

      fail:
	for (i = 0; i < n; i++) {
		if (x_min > points[i].x)
			x_min = points[i].x;
		if (x_max < points[i].x)
			x_max = points[i].x;

		if (y_min > points[i].y)
			y_min = points[i].y;
		if (y_max < points[i].y)
			y_max = points[i].y;
	}

	temp_pixmap = drawable->pScreen->CreatePixmap(drawable->pScreen,
						      x_max - x_min + 1,
						      y_max - y_min + 1,
						      drawable->depth, 0);
	if (temp_pixmap) {
		temp_dest = &temp_pixmap->drawable;
		temp_gc =
		    GetScratchGC(temp_dest->depth, temp_dest->pScreen);
		ValidateGC(temp_dest, temp_gc);
		for (i = 0; i < n; i++) {
			points[i].x -= x_min;
			points[i].y -= y_min;
		}
		(void) glamor_copy_area(drawable,
					temp_dest,
					temp_gc,
					x_min, y_min,
					x_max - x_min + 1,
					y_max - y_min + 1, 0, 0);

	} else
		temp_dest = drawable;

	if (gc->lineWidth == 0) {
		if (glamor_prepare_access(temp_dest, GLAMOR_ACCESS_RW)) {
			if (glamor_prepare_access_gc(gc)) {
				fbPolyLine(temp_dest, gc, mode, n, points);
				glamor_finish_access_gc(gc);
			}
			glamor_finish_access(temp_dest, GLAMOR_ACCESS_RW);
		}
	} else {
		/* fb calls mi functions in the lineWidth != 0 case. */
		fbPolyLine(drawable, gc, mode, n, points);
	}
	if (temp_dest != drawable) {
		(void) glamor_copy_area(temp_dest,
					drawable,
					temp_gc,
					0, 0,
					x_max - x_min + 1,
					y_max - y_min + 1, x_min, y_min);
		drawable->pScreen->DestroyPixmap(temp_pixmap);
		for (i = 0; i < n; i++) {
			points[i].x += x_min;
			points[i].y += y_min;
		}

		FreeScratchGC(temp_gc);
	}
}
