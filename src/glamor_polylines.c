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

#include "glamor_priv.h"

/** @file glamor_polylines.c
 *
 * GC PolyFillRect implementation, taken straight from fb_fill.c
 */

static void _draw_line(xRectangle *rects, int rect, int x1, int y1, int x2, int y2){
		rects[rect].x = x1 < x2 ? x1 : x2;
		rects[rect].y = y1 < y2 ? y1 : y2;
		rects[rect].width = abs(x1 - x2) + 1;
		rects[rect].height = abs(y1 - y2) + 1;
}

static xRectangle *_emit_line(xRectangle *rects, int *rects_cnt, int *rect, int x1, int y1, int x2, int y2){
	*rects_cnt += 1;
	rects = realloc(rects, sizeof(xRectangle) * *rects_cnt);
	_draw_line(rects, *rect, x1, y1, x2, y2);
	*rect += 1;
	return rects;
}

static void _init_points(int *last_x, int *last_y, int cur_x, int cur_y, int *last_start_x, int *last_start_y){
		*last_x = *last_start_x = cur_x;
		*last_y = *last_start_y = cur_y;
}

static xRectangle *_next_point(xRectangle *rects, int *rects_cnt, int *rect, int cur_x, int cur_y, int *last_x, int *last_y, int *last_start_x, int *last_start_y, int steep){
	if ((steep && *last_x != cur_x) || (!steep && *last_y != cur_y)){
		//emit a line from last_start_x,last_start_y to last_x,last_y
		rects = _emit_line(rects, rects_cnt, rect, *last_start_x, *last_start_y, *last_x, *last_y);
		*last_start_x = cur_x;
		*last_start_y = cur_y;
	}
	*last_x = cur_x;
	*last_y = cur_y;
	return rects;
}

/**
 * We need to split the line into as small a number of segments as
 * possible.
 *
 * E.g. line from (x,y) of (1,1)->(5,2) with a slope of .25

 * would be split into two lines:
 * (1,1)->(2,1), (3,2)->(5,2)
 *
 * This is basically an implementation of Bresenham's line algorithm but with
 * FP for now, since I'm lazy.
 *
 * If the line's horizontal-ish, then iterate over the x values, and
 * every time the rounded y value changes, start a new rectangle.
 * If abs(slope) is > 1, then iterate over the y values instead.
 * If the slope is == 1, then we're basically stuck drawing a bunch of points.
 *
 * @param rects Allocated list of xRectangle storage. grows when line is split
 * @param rect_cnt Number of elements allocated in list
 * @param rect Current index in list. modified as needed when the line is split
 * @param x1 - X Coordinate of first point in line
 * @param y1 - Y Coordinate of first point in line
 * @param x2 - X Coordinate of second point in line
 * @param y2 - Y Coordinate of second point in line
 * @return rects - reallocated as needed... grows 1 rectangle every time the line splits
 */
static xRectangle *_glamor_diagonal_line(xRectangle *rects, int *rect_cnt, int *rect, int x1, int y1, int x2, int y2){
	float slope = (float)(y2-y1) / (float)(x2-x1);
	int vert = fabs(slope) > 1;
	int i;

	int cur_x, cur_y; //Current point being processed
	int last_x, last_y; //Last point that was processed
	int last_start_x, last_start_y; //Last x,y of a started line

	if (vert){
		//If we're dealing with slope > 1, then swap the x/y coords to make the
		//line more horizontal than vertical. Reduces looping code
		int temp = x1;
		x1 = y1;
		y1 = temp;

		temp = x2;
		x2 = y2;
		y2 = temp;

		//and recalculate the slope.
		slope = (float)(y2-y1) / (float)(x2-x1);
	}
	if (x1 > x2){
		//And now, if the points go right to left, swap them.
		int temp = x1;
		x1 = x2;
		x2 = temp;

		temp = y1;
		y1 = y2;
		y2 = temp;
	}
	cur_x = x1;
	cur_y = y1;

	//Now just iterate over the range from x1 to x2 and calculate the y values
	//When plotting the points, if (vert==true), then just swap the cur_x/cur_y values
	if (vert)
		_init_points(&last_x, &last_y, y1, x1, &last_start_x, &last_start_y);
	else
		_init_points(&last_x, &last_y, x1, y1, &last_start_x, &last_start_y);

	for(i = 0; i <= x2-x1; i++){
		cur_x = x1+i;
		cur_y = y1 + round(((float)i)*slope);
		if (vert)
			rects = _next_point(rects, rect_cnt, rect, cur_y, cur_x, &last_x, &last_y, &last_start_x, &last_start_y, vert);
		else
			rects = _next_point(rects, rect_cnt, rect, cur_x, cur_y, &last_x, &last_y, &last_start_x, &last_start_y, vert);
	}

	//And now finalize the last line segment using the space that was originally
	//allocated for a horizontal/vertical line slot.
	if (vert)
		_draw_line(rects, *rect, last_start_x, last_start_y, cur_y, cur_x);
	else
		_draw_line(rects, *rect, last_start_x, last_start_y, cur_x, cur_y);

	return rects;
}

/**
 * glamor_poly_lines() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
static Bool
_glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		   DDXPointPtr points, Bool fallback)
{
	xRectangle *rects;
	int x1, x2, y1, y2;
	int i, rect_cnt, rect;

	/* Don't try to do wide lines or non-solid fill style. */
	if (gc->lineWidth != 0) {
		/* This ends up in miSetSpans, which is accelerated as well as we
		 * can hope X wide lines will be.
		 */
		goto wide_line;
	}
	if (gc->lineStyle != LineSolid) {
		glamor_fallback
		    ("non-solid fill line style %d\n",
		     gc->lineStyle);
		goto fail;
	}
	rect_cnt = n-1;
	rects = malloc(sizeof(xRectangle) * rect_cnt);
	x1 = points[0].x;
	y1 = points[0].y;
	/* If we have any non-horizontal/vertical, fall back. */
	for (rect = i = 0; i < n - 1; i++, rect++) {
		if (mode == CoordModePrevious) {
			x2 = x1 + points[i + 1].x;
			y2 = y1 + points[i + 1].y;
		} else {
			x2 = points[i + 1].x;
			y2 = points[i + 1].y;
		}
		if (x1 != x2 && y1 != y2) {
			//For a diagonal line, for every line segment after the first one
			//in the line, we will bump rect_cnt and realloc rects
			rects = _glamor_diagonal_line(rects, &rect_cnt, &rect, x1, y1, x2, y2);

			//free(rects);
			//glamor_fallback("stub diagonal poly_line\n");
			//goto fail;
		} else {
			if (x1 < x2) {
				rects[rect].x = x1;
				rects[rect].width = x2 - x1 + 1;
			} else {
				rects[rect].x = x2;
				rects[rect].width = x1 - x2 + 1;
			}
			if (y1 < y2) {
				rects[rect].y = y1;
				rects[rect].height = y2 - y1 + 1;
			} else {
				rects[rect].y = y2;
				rects[rect].height = y1 - y2 + 1;
			}
		}

		x1 = x2;
		y1 = y2;
	}
	gc->ops->PolyFillRect(drawable, gc, rect_cnt, rects);
	free(rects);
	return TRUE;

      fail:
	if (!fallback
	    && glamor_ddx_fallback_check_pixmap(drawable)
	    && glamor_ddx_fallback_check_gc(gc))
		return FALSE;

	if (gc->lineWidth == 0) {
		if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW) &&
		    glamor_prepare_access_gc(gc)) {
		    fbPolyLine(drawable, gc, mode, n, points);
		}
		glamor_finish_access_gc(gc);
		glamor_finish_access(drawable);
	} else {
wide_line:
		/* fb calls mi functions in the lineWidth != 0 case. */
		fbPolyLine(drawable, gc, mode, n, points);
	}
	return TRUE;
}

void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		  DDXPointPtr points)
{
	_glamor_poly_lines(drawable, gc, mode, n, points, TRUE);
}

Bool
glamor_poly_lines_nf(DrawablePtr drawable, GCPtr gc, int mode, int n,
		     DDXPointPtr points)
{
	return _glamor_poly_lines(drawable, gc, mode, n, points, FALSE);
}
