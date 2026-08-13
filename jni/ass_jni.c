/*
 * Copyright (C) 2026 libass contributors
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORT ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Android JNI bindings for libass. Targets the class
 * `com.libass.android.AssNative`. All opaque libass handles (ASS_Library,
 * ASS_Renderer, ASS_Track) are carried across the JNI boundary as Java
 * `long` values; a zero (0L) handle represents NULL.
 */

#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include "ass.h"

#define ASSJNI_PKG(func) Java_com_libass_android_AssNative_##func

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static void *ptr_from_handle(jlong handle)
{
    return (void *)(uintptr_t)handle;
}

static jlong handle_from_ptr(void *ptr)
{
    return (jlong)(uintptr_t)ptr;
}

static char *cstr_from_jstring(JNIEnv *env, jstring jstr)
{
    if (!jstr)
        return NULL;
    return (*env)->GetStringUTFChars(env, jstr, NULL);
}

static void release_cstr(JNIEnv *env, jstring jstr, const char *cstr)
{
    if (jstr && cstr)
        (*env)->ReleaseStringUTFChars(env, jstr, cstr);
}

/* ------------------------------------------------------------------ */
/* Bitmap scratch buffer registry (single-threaded render assumed)   */
/* ------------------------------------------------------------------ */

#define ASSJNI_SCRATCH_MAX 64

struct assjni_scratch {
    ASS_Renderer *renderer;
    void *buf;
    size_t len;
};

static struct assjni_scratch assjni_scratch[ASSJNI_SCRATCH_MAX];

static struct assjni_scratch *assjni_scratch_find(ASS_Renderer *renderer)
{
    for (int i = 0; i < ASSJNI_SCRATCH_MAX; i++)
        if (assjni_scratch[i].renderer == renderer)
            return &assjni_scratch[i];
    return NULL;
}

static struct assjni_scratch *assjni_scratch_add(ASS_Renderer *renderer)
{
    for (int i = 0; i < ASSJNI_SCRATCH_MAX; i++) {
        if (!assjni_scratch[i].renderer && !assjni_scratch[i].buf) {
            assjni_scratch[i].renderer = renderer;
            assjni_scratch[i].buf = NULL;
            assjni_scratch[i].len = 0;
            return &assjni_scratch[i];
        }
    }
    return NULL;
}

static void assjni_scratch_free(ASS_Renderer *renderer)
{
    struct assjni_scratch *s = assjni_scratch_find(renderer);
    if (!s)
        return;
    free(s->buf);
    s->buf = NULL;
    s->len = 0;
    s->renderer = NULL;
}

/* ------------------------------------------------------------------ */
/* Library lifecycle                                                  */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL
ASSJNI_PKG(assLibraryVersion)(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return (jint)ass_library_version();
}

JNIEXPORT jlong JNICALL
ASSJNI_PKG(assLibraryInit)(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return handle_from_ptr(ass_library_init());
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assLibraryDone)(JNIEnv *env, jobject thiz, jlong handle)
{
    (void)env;
    (void)thiz;
    ASS_Library *lib = ptr_from_handle(handle);
    if (lib)
        ass_library_done(lib);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetFontsDir)(JNIEnv *env, jobject thiz, jlong handle, jstring dir)
{
    (void)thiz;
    ASS_Library *lib = ptr_from_handle(handle);
    if (!lib)
        return;
    const char *cdir = cstr_from_jstring(env, dir);
    ass_set_fonts_dir(lib, cdir);
    release_cstr(env, dir, cdir);
}

/* ------------------------------------------------------------------ */
/* Renderer lifecycle + configuration                                 */
/* ------------------------------------------------------------------ */

JNIEXPORT jlong JNICALL
ASSJNI_PKG(assRendererInit)(JNIEnv *env, jobject thiz, jlong libHandle)
{
    (void)env;
    (void)thiz;
    ASS_Library *lib = ptr_from_handle(libHandle);
    if (!lib)
        return 0;
    return handle_from_ptr(ass_renderer_init(lib));
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assRendererDone)(JNIEnv *env, jobject thiz, jlong handle)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (!renderer)
        return;
    assjni_scratch_free(renderer);
    ass_renderer_done(renderer);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetFrameSize)(JNIEnv *env, jobject thiz, jlong handle, jint w, jint h)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_frame_size(renderer, w, h);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetStorageSize)(JNIEnv *env, jobject thiz, jlong handle, jint w, jint h)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_storage_size(renderer, w, h);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetFonts)(JNIEnv *env, jobject thiz, jlong handle,
                        jstring defaultFont, jstring defaultFamily,
                        jint dfp, jstring config, jboolean update)
{
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (!renderer)
        return;
    const char *cfont = cstr_from_jstring(env, defaultFont);
    const char *cfamily = cstr_from_jstring(env, defaultFamily);
    const char *cconfig = cstr_from_jstring(env, config);
    ass_set_fonts(renderer, cfont, cfamily, (int)dfp, cconfig, update ? 1 : 0);
    release_cstr(env, defaultFont, cfont);
    release_cstr(env, defaultFamily, cfamily);
    release_cstr(env, config, cconfig);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetFontScale)(JNIEnv *env, jobject thiz, jlong handle, jdouble scale)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_font_scale(renderer, scale);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetPixelAspect)(JNIEnv *env, jobject thiz, jlong handle, jdouble par)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_pixel_aspect(renderer, par);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetMargins)(JNIEnv *env, jobject thiz, jlong handle,
                          jint t, jint b, jint l, jint r)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_margins(renderer, t, b, l, r);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetUseMargins)(JNIEnv *env, jobject thiz, jlong handle, jboolean use)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_use_margins(renderer, use ? 1 : 0);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetHinting)(JNIEnv *env, jobject thiz, jlong handle, jint ht)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_hinting(renderer, (ASS_Hinting)ht);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assSetShaper)(JNIEnv *env, jobject thiz, jlong handle, jint level)
{
    (void)env;
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(handle);
    if (renderer)
        ass_set_shaper(renderer, (ASS_ShapingLevel)level);
}

/* ------------------------------------------------------------------ */
/* Track creation / loading                                          */
/* ------------------------------------------------------------------ */

JNIEXPORT jlong JNICALL
ASSJNI_PKG(assNewTrack)(JNIEnv *env, jobject thiz, jlong libHandle)
{
    (void)env;
    (void)thiz;
    ASS_Library *lib = ptr_from_handle(libHandle);
    if (!lib)
        return 0;
    return handle_from_ptr(ass_new_track(lib));
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assFreeTrack)(JNIEnv *env, jobject thiz, jlong handle)
{
    (void)env;
    (void)thiz;
    ASS_Track *track = ptr_from_handle(handle);
    if (track)
        ass_free_track(track);
}

JNIEXPORT jlong JNICALL
ASSJNI_PKG(assReadMemory)(JNIEnv *env, jobject thiz, jlong libHandle,
                          jbyteArray data, jint len, jstring codepage)
{
    (void)thiz;
    ASS_Library *lib = ptr_from_handle(libHandle);
    if (!lib || !data || len <= 0)
        return 0;

    /* libass parses in place; pass a private NUL-terminated copy to be safe. */
    char *buf = malloc((size_t)len + 1);
    if (!buf)
        return 0;
    (*env)->GetByteArrayRegion(env, data, 0, len, (jbyte *)buf);
    buf[len] = '\0';

    const char *ccp = cstr_from_jstring(env, codepage);
    ASS_Track *track = ass_read_memory(lib, buf, (size_t)len, ccp);
    release_cstr(env, codepage, ccp);

    free(buf);
    return handle_from_ptr(track);
}

JNIEXPORT jlong JNICALL
ASSJNI_PKG(assReadFile)(JNIEnv *env, jobject thiz, jlong libHandle,
                        jstring fname, jstring codepage)
{
    (void)thiz;
    ASS_Library *lib = ptr_from_handle(libHandle);
    if (!lib || !fname)
        return 0;
    const char *cfn = cstr_from_jstring(env, fname);
    const char *ccp = cstr_from_jstring(env, codepage);
    ASS_Track *track = ass_read_file(lib, cfn, ccp);
    release_cstr(env, fname, cfn);
    release_cstr(env, codepage, ccp);
    return handle_from_ptr(track);
}

/* ------------------------------------------------------------------ */
/* Event processing                                                   */
/* ------------------------------------------------------------------ */

JNIEXPORT void JNICALL
ASSJNI_PKG(assProcessData)(JNIEnv *env, jobject thiz, jlong handle,
                           jbyteArray data, jint size)
{
    (void)thiz;
    ASS_Track *track = ptr_from_handle(handle);
    if (!track || !data || size <= 0)
        return;
    char *buf = malloc((size_t)size + 1);
    if (!buf)
        return;
    (*env)->GetByteArrayRegion(env, data, 0, size, (jbyte *)buf);
    buf[size] = '\0';
    ass_process_data(track, buf, size);
    free(buf);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assProcessCodecPrivate)(JNIEnv *env, jobject thiz, jlong handle,
                                   jbyteArray data, jint size)
{
    (void)thiz;
    ASS_Track *track = ptr_from_handle(handle);
    if (!track || !data || size <= 0)
        return;
    char *buf = malloc((size_t)size + 1);
    if (!buf)
        return;
    (*env)->GetByteArrayRegion(env, data, 0, size, (jbyte *)buf);
    buf[size] = '\0';
    ass_process_codec_private(track, buf, size);
    free(buf);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assProcessChunk)(JNIEnv *env, jobject thiz, jlong handle,
                            jbyteArray data, jint size,
                            jlong timecode, jlong duration)
{
    (void)thiz;
    ASS_Track *track = ptr_from_handle(handle);
    if (!track || !data || size <= 0)
        return;
    char *buf = malloc((size_t)size + 1);
    if (!buf)
        return;
    (*env)->GetByteArrayRegion(env, data, 0, size, (jbyte *)buf);
    buf[size] = '\0';
    ass_process_chunk(track, buf, size, timecode, duration);
    free(buf);
}

JNIEXPORT void JNICALL
ASSJNI_PKG(assFlushEvents)(JNIEnv *env, jobject thiz, jlong handle)
{
    (void)env;
    (void)thiz;
    ASS_Track *track = ptr_from_handle(handle);
    if (track)
        ass_flush_events(track);
}

JNIEXPORT jlong JNICALL
ASSJNI_PKG(assStepSub)(JNIEnv *env, jobject thiz, jlong handle,
                       jlong now, jint movement)
{
    (void)env;
    (void)thiz;
    ASS_Track *track = ptr_from_handle(handle);
    if (!track)
        return 0;
    return (jlong)ass_step_sub(track, now, movement);
}

/* ------------------------------------------------------------------ */
/* Rendering                                                          */
/* ------------------------------------------------------------------ */
/*
 * assRenderFrame renders one frame and returns a direct java.nio.ByteBuffer
 * that exposes the concatenated alpha bitmaps of all ASS_Image nodes, with
 * each row padded out to `stride` (so the last row is fully readable).
 *
 * Per-image metadata is written into `metaOut` (an int[]), laid out as:
 *
 *   metaOut[0]              = image count (>= 0), or -(required capacity) on overflow
 *   metaOut[1]              = detect_change value from libass
 *   metaOut[2 + 8*k + 0]    = w
 *   metaOut[2 + 8*k + 1]    = h
 *   metaOut[2 + 8*k + 2]    = stride
 *   metaOut[2 + 8*k + 3]    = dst_x
 *   metaOut[2 + 8*k + 4]    = dst_y
 *   metaOut[2 + 8*k + 5]    = color  (RGBA, packed as uint32)
 *   metaOut[2 + 8*k + 6]    = type   (0=character,1=outline,2=shadow)
 *   metaOut[2 + 8*k + 7]    = byte offset of this image's pixels in the ByteBuffer
 *
 * The detect_change flag is also written to detectChangeOut[0].
 *
 * The returned ByteBuffer is backed by scratch memory owned by this renderer.
 * It is only valid until the next call to assRenderFrame for the same renderer
 * or until assRendererDone is called on it. Callers that need to retain the
 * pixels across frames must copy them out before the next render.
 */
JNIEXPORT jobject JNICALL
ASSJNI_PKG(assRenderFrameNative)(JNIEnv *env, jobject thiz, jlong rendererHandle,
                            jlong trackHandle, jlong now,
                            jintArray detectChangeOut, jintArray metaOut)
{
    (void)thiz;
    ASS_Renderer *renderer = ptr_from_handle(rendererHandle);
    ASS_Track *track = ptr_from_handle(trackHandle);
    if (!renderer || !track)
        return NULL;

    int detect_change = 0;
    ASS_Image *img = ass_render_frame(renderer, track, now, &detect_change);

    if (detectChangeOut) {
        jint dc = (jint)detect_change;
        (*env)->SetIntArrayRegion(env, detectChangeOut, 0, 1, &dc);
    }

    /* First pass: count images and total bitmap bytes (padded rows). */
    int count = 0;
    size_t total = 0;
    for (ASS_Image *p = img; p; p = p->next) {
        if (p->w <= 0 || p->h <= 0)
            continue;
        count++;
        total += (size_t)p->stride * (size_t)p->h;
    }

    int meta_cap = metaOut ? (*env)->GetArrayLength(env, metaOut) : 0;
    int required = 2 + 8 * (count > 0 ? count : 0);

    if (metaOut && (size_t)meta_cap < required) {
        /* Signal the caller how many images need to be reported to retry. */
        jint neg = -(jint)count;
        (*env)->SetIntArrayRegion(env, metaOut, 0, 1, &neg);
        return NULL;
    }

    struct assjni_scratch *s = assjni_scratch_find(renderer);
    if (!s) {
        s = assjni_scratch_add(renderer);
        if (!s)
            return NULL;
    }
    if (s->len < total) {
        void *nb = realloc(s->buf, total ? total : 1);
        if (!nb)
            return NULL;
        s->buf = nb;
        s->len = total;
    }

    /* Build the metadata array in a heap buffer (count is unbounded in theory). */
    jint *meta_items = NULL;
    if (metaOut && required > 0) {
        meta_items = (jint *)malloc((size_t)required * sizeof(jint));
        if (!meta_items)
            return NULL;
    }
    int idx = 0;
    if (metaOut) {
        meta_items[idx++] = (jint)count;
        meta_items[idx++] = (jint)detect_change;
    }

    /* Second pass: copy bitmaps into the scratch buffer (pad last row). */
    size_t offset = 0;
    for (ASS_Image *p = img; p; p = p->next) {
        if (p->w <= 0 || p->h <= 0)
            continue;
        const unsigned char *src = p->bitmap;
        unsigned char *dst = (unsigned char *)s->buf + offset;
        int h = p->h, w = p->w, stride = p->stride;
        for (int y = 0; y < h - 1; y++) {
            memcpy(dst, src, stride);
            src += stride;
            dst += stride;
        }
        /* Last row: copy w valid bytes then zero-pad up to stride. */
        memcpy(dst, src, w);
        if (stride > w)
            memset(dst + w, 0, (size_t)(stride - w));

        if (metaOut) {
            meta_items[idx++] = (jint)w;
            meta_items[idx++] = (jint)h;
            meta_items[idx++] = (jint)stride;
            meta_items[idx++] = (jint)p->dst_x;
            meta_items[idx++] = (jint)p->dst_y;
            meta_items[idx++] = (jint)p->color;
            meta_items[idx++] = (jint)p->type;
            meta_items[idx++] = (jint)offset;
        }
        offset += (size_t)stride * (size_t)h;
    }

    if (metaOut && idx > 0)
        (*env)->SetIntArrayRegion(env, metaOut, 0, idx, meta_items);
    free(meta_items);

    if (total == 0)
        return NULL;

    return (*env)->NewDirectByteBuffer(env, s->buf, (jlong)s->len);
}

/* Optional version pin for loaders that expect JNI_OnLoad. */
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    (void)reserved;
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;
    return JNI_VERSION_1_6;
}
