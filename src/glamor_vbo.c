/*
 * Copyright © 2014 Intel Corporation
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file glamor_vbo.c
 *
 * Helpers for managing streamed vertex bufffers used in glamor.
 */

#include "glamor_priv.h"

/* Allow compilation with outdated GL headers */
#ifndef GL_MAP_PERSISTENT_BIT
#	define GL_MAP_PERSISTENT_BIT 0x0040
#endif
#ifndef GL_MAP_COHERENT_BIT
#	define GL_MAP_COHERENT_BIT 0x0080
#endif

void *
glamor_get_vbo_space(ScreenPtr screen, int size, char **vbo_offset)
{
	glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;
	int err = 0;

	dispatch = glamor_get_dispatch(glamor_priv);

	dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);

	if (glamor_priv->has_buffer_storage) {
	    if (glamor_priv->vbo_size < glamor_priv->vbo_offset + size) {
		if (glamor_priv->vbo_mapped)
		    dispatch->glUnmapBuffer(GL_ARRAY_BUFFER);

		if (size > glamor_priv->vbo_size) {
		    glamor_priv->vbo_size = MAX(256 * 1024, size);

		    /* We aren't allowed to resize glBufferStorage()
		     * buffers, so we need to gen a new one.
		     */
		    dispatch->glDeleteBuffers(1, &glamor_priv->vbo);
		    dispatch->glGenBuffers(1, &glamor_priv->vbo);
		    dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);

		    assert(dispatch->glGetError() == GL_NO_ERROR);
		    dispatch->glBufferStorage(GL_ARRAY_BUFFER, glamor_priv->vbo_size, NULL,
					      GL_MAP_WRITE_BIT |
					      GL_MAP_PERSISTENT_BIT |
					      GL_MAP_COHERENT_BIT);

		    err = dispatch->glGetError();
		    if (err != GL_NO_ERROR) {
			    /* If the driver failed our coherent mapping, fall
			     * back to the ARB_mbr path.
			     */
			    ErrorF("ARB_buffer_storage: falling back to MapBufferRange path!\n");
			    glamor_priv->has_buffer_storage = FALSE;
			    glamor_priv->vbo_size = 0;
			    glamor_put_dispatch(glamor_priv);
			    return glamor_get_vbo_space(screen, size, vbo_offset);
		    }
		}

		glamor_priv->vbo_offset = 0;
		glamor_priv->vb_base = dispatch->glMapBufferRange(GL_ARRAY_BUFFER,
								  0, glamor_priv->vbo_size,
								  GL_MAP_WRITE_BIT |
								  GL_MAP_INVALIDATE_BUFFER_BIT |
								  GL_MAP_PERSISTENT_BIT |
								  GL_MAP_COHERENT_BIT);
	    }
	    glamor_priv->vb = glamor_priv->vb_base + glamor_priv->vbo_offset;
	    *vbo_offset = (void *)(uintptr_t)glamor_priv->vbo_offset;
	    glamor_priv->vbo_offset += size;
	    glamor_priv->vbo_mapped = TRUE;
	} else if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
	    if (glamor_priv->vbo_size < glamor_priv->vbo_offset + size) {
		glamor_priv->vbo_size = MAX(65536, size);
		glamor_priv->vbo_offset = 0;
		dispatch->glBufferData(GL_ARRAY_BUFFER,
				       glamor_priv->vbo_size, NULL, GL_STREAM_DRAW);
	    }

	    glamor_priv->vb = dispatch->glMapBufferRange(GL_ARRAY_BUFFER,
							 glamor_priv->vbo_offset,
							 size,
							 GL_MAP_WRITE_BIT |
							 GL_MAP_UNSYNCHRONIZED_BIT |
							 GL_MAP_INVALIDATE_RANGE_BIT);
	    assert(glamor_priv->vb != NULL);
	    *vbo_offset = (void *)(uintptr_t)glamor_priv->vbo_offset;
	    glamor_priv->vbo_offset += size;
	    glamor_priv->vbo_mapped = TRUE;
	} else {
	    /* Return a pointer to the statically allocated non-VBO
	     * memory. We'll upload it through glBufferData() later.
	     */
	    if (glamor_priv->vbo_size < size) {
		glamor_priv->vbo_size = size;
		free(glamor_priv->vb);
		glamor_priv->vb = XNFalloc(size);
	    }
	    *vbo_offset = NULL;
	    glamor_priv->vbo_offset = 0;
	}

	glamor_put_dispatch(glamor_priv);

	return glamor_priv->vb;
}

void
glamor_put_vbo_space(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;

	/* If we're in the ARB_buffer_storage path, we have a persistent
	 * mapping, so we can leave it around until we reach the end.
	 */
	if (glamor_priv->has_buffer_storage)
		return;

	dispatch = glamor_get_dispatch(glamor_priv);

	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
	    if (glamor_priv->vbo_mapped) {
		dispatch->glUnmapBuffer(GL_ARRAY_BUFFER);
		glamor_priv->vbo_mapped = FALSE;
	    }
	} else {
	    dispatch->glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);
	    dispatch->glBufferData(GL_ARRAY_BUFFER, glamor_priv->vbo_offset,
				   glamor_priv->vb, GL_DYNAMIC_DRAW);
	}

	glamor_put_dispatch(glamor_priv);
}
