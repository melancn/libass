package com.libass.android;

import java.nio.ByteBuffer;

/**
 * Minimal Android JNI bindings for libass.
 *
 * <p>This class is a sample Java facade that matches the native methods
 * exported by {@code libass_jni.so}. It is provided as a reference; copy it
 * into your application (preserving the {@code com.libass.android.AssNative}
 * package/class, or regenerate the native symbol names to match a renamed
 * class) and load the library with {@code System.loadLibrary("ass_jni")}.
 *
 * <p>All opaque libass handles ({@code ASS_Library}, {@code ASS_Renderer},
 * {@code ASS_Track}) are carried across the JNI boundary as Java {@code long}
 * values; a zero (0L) handle represents {@code NULL}. Callers own the
 * lifecycle: pair every {@code *Init} with the matching {@code *Done} /
 * {@code *Free}.
 *
 * <h2>Basic usage</h2>
 * <pre>{@code
 * long lib   = AssNative.assLibraryInit();
 * long track = AssNative.assReadMemory(lib, assBytes, assBytes.length, null);
 *
 * long renderer = AssNative.assRendererInit(lib);
 * AssNative.assSetFrameSize(renderer, 1920, 1080);
 * AssNative.assSetStorageSize(renderer, 1920, 1080);
 * AssNative.assSetFonts(renderer, null, "sans-serif",
 *                       /* dfp = AUTODETECT *\/ 1, null, true);
 *
 * int[] detect = new int[1];
 * AssImage img = AssNative.assRenderFrame(renderer, track, /* nowMs *\/ 5000, detect);
 * // ... composite img.bitmaps[i] at (img.dstX[i], img.dstY[i]) using img.color[i] ...
 *
 * AssNative.assRendererDone(renderer);
 * AssNative.assFreeTrack(track);
 * AssNative.assLibraryDone(lib);
 * }</pre>
 *
 * @see <a href="https://github.com/libass/libass/blob/master/libass/ass.h">libass/ass.h</a>
 */
public final class AssNative {

    static {
        System.loadLibrary("ass_jni");
    }

    private AssNative() {}

    /* -------------------- constants -------------------- */

    /** Default font provider: use the first available provider. */
    public static final int FONT_PROVIDER_NONE       = 0;
    public static final int FONT_PROVIDER_AUTODETECT  = 1;
    public static final int FONT_PROVIDER_CORETEXT    = 2;
    public static final int FONT_PROVIDER_FONTCONFIG  = 3;
    public static final int FONT_PROVIDER_DIRECTWRITE = 4;

    /** {@link #assSetHinting} */
    public static final int HINTING_NONE   = 0;
    public static final int HINTING_LIGHT  = 1;
    public static final int HINTING_NORMAL = 2;
    public static final int HINTING_NATIVE = 3;

    /** {@link #assSetShaper} */
    public static final int SHAPING_SIMPLE  = 0;
    public static final int SHAPING_COMPLEX = 1;

    /** ASS_Image.type values reported in {@link AssImage#type}. */
    public static final int IMAGE_TYPE_CHARACTER = 0;
    public static final int IMAGE_TYPE_OUTLINE   = 1;
    public static final int IMAGE_TYPE_SHADOW    = 2;

    /* -------------------- library -------------------- */

    /** @return the value of {@code LIBASS_VERSION} the native library was built with. */
    public static native int assLibraryVersion();

    /** Create a {@code ASS_Library}. Free with {@link #assLibraryDone}. */
    public static native long assLibraryInit();

    /** Free a library handle returned by {@link #assLibraryInit}. */
    public static native void assLibraryDone(long lib);

    /**
     * Set a directory scanned for additional fonts (in addition to any system
     * provider). Pass {@code null} to clear. Embedded fonts work without this.
     */
    public static native void assSetFontsDir(long lib, String dir);

    /* -------------------- renderer -------------------- */

    /** Create a {@code ASS_Renderer}. Free with {@link #assRendererDone}. */
    public static native long assRendererInit(long lib);

    /** Free a renderer. Any previously returned {@link AssImage} ByteBuffer becomes invalid. */
    public static native void assRendererDone(long renderer);

    /**
     * Set the output frame size in pixels (including margins). The renderer
     * never returns images outside this area. Must be called before rendering.
     */
    public static native void assSetFrameSize(long renderer, int w, int h);

    /**
     * Set the source video storage size in pixels. Used for source aspect
     * ratio and 3D transform scale. Pass ({@code 0,0}) to reset to default.
     */
    public static native void assSetStorageSize(long renderer, int w, int h);

    /**
     * Configure font lookup. Call before rendering.
     *
     * @param defaultFont   path to a fallback font file, or {@code null}.
     * @param defaultFamily fallback font family name, or {@code null}.
     * @param dfp           one of the {@code FONT_PROVIDER_*} constants.
     * @param config        fontconfig config path or {@code null} (fontconfig only).
     * @param update        whether to (re)build the fontconfig cache (fontconfig only).
     */
    public static native void assSetFonts(long renderer, String defaultFont,
                                          String defaultFamily, int dfp,
                                          String config, boolean update);

    /** Set a fixed font scaling factor (default {@code 1.0}). */
    public static native void assSetFontScale(long renderer, double scale);

    /**
     * Set the pixel aspect ratio (pixel width / pixel height). {@code 1.0} =
     * square pixels, {@code 0} = automatic (default).
     */
    public static native void assSetPixelAspect(long renderer, double par);

    /**
     * Set frame margins (pixels, may be negative for pan-and-scan crops).
     */
    public static native void assSetMargins(long renderer, int top, int bottom,
                                            int left, int right);

    /** Whether margins may be used to place regular events. */
    public static native void assSetUseMargins(long renderer, boolean use);

    /** Set font hinting (one of {@code HINTING_*}). */
    public static native void assSetHinting(long renderer, int ht);

    /** Set the text shaping level (one of {@code SHAPING_*}). */
    public static native void assSetShaper(long renderer, int level);

    /* -------------------- tracks -------------------- */

    /** Allocate a new empty track. Free with {@link #assFreeTrack}. */
    public static native long assNewTrack(long lib);

    /**
     * Read subtitles from a UTF-8 byte buffer (typically an {@code .ass}/{@code .ssa}
     * file). Free with {@link #assFreeTrack}.
     *
     * @param data      subtitle bytes.
     * @param len       number of valid bytes in {@code data}.
     * @param codepage  iconv codepage or {@code null} for UTF-8/autodetect.
     * @return track handle, or {@code 0L} on failure.
     */
    public static native long assReadMemory(long lib, byte[] data, int len, String codepage);

    /**
     * Read subtitles from a file path.
     *
     * @param fname     file path (UTF-8).
     * @param codepage  iconv codepage or {@code null} for UTF-8/autodetect.
     * @return track handle, or {@code 0L} on failure.
     */
    public static native long assReadFile(long lib, String fname, String codepage);

    /** Free a track (and its styles/events). */
    public static native void assFreeTrack(long track);

    /** Parse a full line of subtitle stream data into the track. */
    public static native void assProcessData(long track, byte[] data, int size);

    /** Parse a Matroska codec private section. */
    public static native void assProcessCodecPrivate(long track, byte[] data, int size);

    /** Parse a Matroska subtitle chunk (one event). */
    public static native void assProcessChunk(long track, byte[] data, int size,
                                              long timecodeMs, long durationMs);

    /** Flush buffered events from the track. */
    public static native void assFlushEvents(long track);

    /**
     * Step to a neighbouring subtitle event.
     *
     * @param now       current time in ms.
     * @param movement  number of events to skip ({@code +2}=after next, {@code -1}=previous).
     * @return timeshift to the start of the target event in ms.
     */
    public static native long assStepSub(long track, long now, int movement);

    /* -------------------- rendering -------------------- */

    /**
     * Render one frame.
     *
     * <p>This is a convenience variant that allocates a fresh {@link AssImage}
     * each call. The returned direct {@code ByteBuffer} is backed by native
     * scratch memory that is reused on the next call for the same renderer,
     * so callers that need to retain the pixels must copy them before the
     * next render or {@link #assRendererDone}.
     *
     * <p>Internally this calls the native method with a metadata array sized
     * generously and grows it on overflow (the native side reports a negative
     * required count when the chunk is too small).
     *
     * @param renderer  handle from {@link #assRendererInit}.
     * @param track     handle from {@link #assReadMemory}/{@link #assNewTrack}.
     * @param nowMs     video timestamp in milliseconds.
     * @param detectOut {@code int[1]} that receives the detect_change flag, or {@code null}.
     * @return an {@link AssImage} over the rendered bitmaps, or {@code null} if
     *         nothing is visible for this timestamp.
     */
    public static AssImage assRenderFrame(long renderer, long track, long nowMs, int[] detectOut) {
        int[] meta = new int[2 + 8 * 16]; /* start with room for 16 images */
        for (;;) {
            ByteBuffer buf = assRenderFrameNative(renderer, track, nowMs, detectOut, meta);
            int count = meta[0];
            if (count < 0) {
                /* Not enough space: resize to fit the reported image count. */
                int need = -count;
                meta = new int[2 + 8 * need];
                continue;
            }
            if (count == 0)
                return null; /* nothing to draw */
            return new AssImage(buf, meta, count);
        }
    }

    /**
     * Native render routine. Returns a direct {@link ByteBuffer} over the
     * concatenated, stride-padded alpha bitmaps, and writes per-image metadata
     * into {@code metaOut} (see {@link AssImage} for the layout). On overflow
     * {@code metaOut[0]} is set to the negative required image count and
     * {@code null} is returned.
     */
    private static native ByteBuffer assRenderFrameNative(long renderer, long track,
                                                           long nowMs,
                                                           int[] detectOut, int[] metaOut);

    /**
     * Decoded result of {@link #assRenderFrame} for one frame.
     *
     * <p>{@code bitmaps} is a direct {@link ByteBuffer} that is only valid
     * until the next render call on the same renderer (or {@link #assRendererDone}).
     * It contains each visible image's alpha plane rows concatenated, with
     * every row padded out to {@code stride[i]} bytes.
     */
    public static final class AssImage {
        /** The concatenated, stride-padded alpha bitmaps (valid until next render). */
        public final ByteBuffer bitmaps;
        /** Number of visible images. */
        public final int count;
        /** Bitmap width per image. length == count. */
        public final int[] w;
        /** Bitmap height per image. length == count. */
        public final int[] h;
        /** Bitmap stride per image. length == count. */
        public final int[] stride;
        /** X placement in the video frame per image. length == count. */
        public final int[] dstX;
        /** Y placement in the video frame per image. length == count. */
        public final int[] dstY;
        /** Packed RGBA color + alpha per image. length == count. */
        public final int[] color;
        /** Image type per image (one of {@code IMAGE_TYPE_*}). length == count. */
        public final int[] type;
        /** Byte offset of each image's pixels in {@link #bitmaps}. length == count. */
        public final int[] offset;

        private AssImage(ByteBuffer bitmaps, int[] meta, int count) {
            this.bitmaps = bitmaps;
            this.count = count;
            this.w      = new int[count];
            this.h      = new int[count];
            this.stride = new int[count];
            this.dstX   = new int[count];
            this.dstY   = new int[count];
            this.color  = new int[count];
            this.type   = new int[count];
            this.offset = new int[count];
            for (int i = 0; i < count; i++) {
                int base = 2 + 8 * i;
                w[i]      = meta[base + 0];
                h[i]      = meta[base + 1];
                stride[i] = meta[base + 2];
                dstX[i]   = meta[base + 3];
                dstY[i]   = meta[base + 4];
                color[i]  = meta[base + 5];
                type[i]   = meta[base + 6];
                offset[i] = meta[base + 7];
            }
        }

        /**
         * Read the alpha (0-255) at {@code (x,y)} of image {@code i}.
         */
        public int alpha(int i, int x, int y) {
            return bitmaps.get(offset[i] + y * stride[i] + x) & 0xFF;
        }
    }
}
