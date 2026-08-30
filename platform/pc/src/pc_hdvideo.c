/* pc_hdvideo.c -- HD FMV replacement decoder. Decodes an H.264/HEVC .mp4 (upscaled STR frames, 1:1
 * with the game's frame counter) via libav and hands back RGB24 for game frame N; the STR still
 * supplies audio + timing. No-op stubs without VH_HD_VIDEO. See docs/pc-port/subsystems/mdec.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pc_platform.h"   /* PC_Verbose: per-movie open/close lines are chatter */

#ifdef VH_HD_VIDEO
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

static struct {
    AVFormatContext *fmt;
    AVCodecContext  *dec;
    struct SwsContext *sws;
    AVFrame  *frame;         /* decoded (native pixfmt) */
    AVPacket *pkt;
    unsigned char *rgb;      /* RGB24 output, w*h*3 */
    int   rgbStride;
    int   vidStream;
    int   w, h;
    enum AVPixelFormat pixFmt;   /* geometry the sws/rgb buffer above are currently sized/built for */
    int   cur;               /* index of the frame currently in `rgb` (-1 = none) */
    int   eof;               /* decoder fully drained */
    int   open;
} V;

/* Free the DECODER only. V.rgb (the last presented frame) is kept: the movie overlay still points at it
 * after a movie ends (the game leaves the last frame up until the next scene / ClearScreen, matching the
 * native MDEC path). It's released at the next Open -- by then the overlay has moved to the new movie. */
static void hdv_free_decoder(void) {
    if (V.sws)   { sws_freeContext(V.sws); V.sws = NULL; }
    if (V.frame) av_frame_free(&V.frame);
    if (V.pkt)   av_packet_free(&V.pkt);
    if (V.dec)   avcodec_free_context(&V.dec);
    if (V.fmt)   avformat_close_input(&V.fmt);
    V.vidStream = -1; V.cur = -1; V.eof = 0; V.open = 0;
}
static void hdv_free_all(void) { hdv_free_decoder(); free(V.rgb); V.rgb = NULL; V.w = V.h = 0; }

int PC_HdVideoOpen(const char *path) {
    const AVCodec *codec;
    /* Errors only: our minimal static libav has no nasm-built SIMD, so libswscale otherwise prints
     * a harmless "No accelerated colorspace conversion" info line on every first open -- console
     * noise in the logs players paste. Real decode errors still come through. */
    { static int once; if (!once) { once = 1; av_log_set_level(AV_LOG_ERROR); } }
    hdv_free_all();    /* release the PREVIOUS movie's decoder + kept frame (overlay has moved on) */
    if (avformat_open_input(&V.fmt, path, NULL, NULL) != 0) return 0;
    if (avformat_find_stream_info(V.fmt, NULL) < 0)         { hdv_free_all(); return 0; }
    V.vidStream = av_find_best_stream(V.fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (V.vidStream < 0)                                    { hdv_free_all(); return 0; }
    V.dec = avcodec_alloc_context3(codec);
    if (!V.dec)                                             { hdv_free_all(); return 0; }
    if (avcodec_parameters_to_context(V.dec, V.fmt->streams[V.vidStream]->codecpar) < 0 ||
        avcodec_open2(V.dec, codec, NULL) < 0)              { hdv_free_all(); return 0; }
    V.w = V.dec->width; V.h = V.dec->height;
    if (V.w <= 0 || V.h <= 0 || V.w > 8192 || V.h > 8192)   { hdv_free_all(); return 0; }
    V.frame = av_frame_alloc();
    V.pkt   = av_packet_alloc();
    V.rgbStride = V.w * 3;
    V.rgb = (unsigned char *)malloc((size_t)V.rgbStride * V.h);
    V.sws = sws_getContext(V.w, V.h, V.dec->pix_fmt, V.w, V.h, AV_PIX_FMT_RGB24,
                           SWS_BILINEAR, NULL, NULL, NULL);
    V.pixFmt = V.dec->pix_fmt;
    if (!V.frame || !V.pkt || !V.rgb || !V.sws)             { hdv_free_all(); return 0; }
    V.cur = -1; V.eof = 0; V.open = 1;
    if (PC_Verbose()) fprintf(stderr, "[HDvideo] open %s (%dx%d)\n", path, V.w, V.h);
    return 1;
}

/* Decode exactly one more frame into V.frame, convert to V.rgb, ++V.cur. 1 on success, 0 at EOF. */
static int hdv_decode_next(void) {
    for (;;) {
        int r = avcodec_receive_frame(V.dec, V.frame);
        if (r == 0) {
            /* A later SPS/PPS can change width/height/pixel format mid-stream (codex 1.2) --
             * recreate the scaler + output buffer for the new geometry rather than scaling
             * into a context/buffer still sized for the old one. Keep the same 8192 cap as
             * open time; a geometry too large to honor ends the video, keeping the last frame. */
            if (V.frame->width != V.w || V.frame->height != V.h || V.frame->format != V.pixFmt) {
                int nw = V.frame->width, nh = V.frame->height;
                unsigned char *nrgb;
                struct SwsContext *nsws;
                if (nw <= 0 || nh <= 0 || nw > 8192 || nh > 8192) { V.eof = 1; return 0; }
                /* Build the new converter before touching V.rgb: a failure here must leave the
                 * kept last frame (V.rgb sized by the old V.w/V.h) fully intact. */
                nsws = sws_getContext(nw, nh, (enum AVPixelFormat)V.frame->format, nw, nh,
                                      AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
                if (!nsws) { V.eof = 1; return 0; }
                nrgb = (unsigned char *)realloc(V.rgb, (size_t)nw * 3 * (size_t)nh);
                if (!nrgb) { sws_freeContext(nsws); V.eof = 1; return 0; }
                V.rgb = nrgb;
                if (V.sws) sws_freeContext(V.sws);
                V.sws = nsws;
                V.w = nw; V.h = nh; V.rgbStride = nw * 3;
                V.pixFmt = (enum AVPixelFormat)V.frame->format;
            }
            {
                unsigned char *dst[1] = { V.rgb };
                int stride[1] = { V.rgbStride };
                sws_scale(V.sws, (const unsigned char *const *)V.frame->data, V.frame->linesize,
                          0, V.h, dst, stride);
            }
            V.cur++;
            return 1;
        }
        if (r == AVERROR_EOF) { V.eof = 1; return 0; }
        if (r != AVERROR(EAGAIN)) { V.eof = 1; return 0; }   /* decode error -> stop */
        /* need more input */
        if (av_read_frame(V.fmt, V.pkt) < 0) {               /* end of file -> flush decoder */
            avcodec_send_packet(V.dec, NULL);
            continue;
        }
        if (V.pkt->stream_index == V.vidStream) avcodec_send_packet(V.dec, V.pkt);
        av_packet_unref(V.pkt);
    }
}

/* RGB24 picture for game frame `want` (>= 0). Decodes forward to it; clamps to the last frame at EOF.
 * Returns NULL only if nothing has ever decoded. w/h set on success. */
const unsigned char *PC_HdVideoFrame(int want, int *w, int *h) {
    if (!V.open) return NULL;
    while (V.cur < want && !V.eof) hdv_decode_next();
    if (V.cur < 0) return NULL;
    if (w) *w = V.w;
    if (h) *h = V.h;
    return V.rgb;
}

void PC_HdVideoClose(void) { if (V.open) { if (PC_Verbose()) fprintf(stderr, "[HDvideo] close\n"); hdv_free_decoder(); } }
int  PC_HdVideoActive(void) { return V.open; }

#else  /* built without libav: stubs so callers link + degrade to native MDEC */
int  PC_HdVideoOpen(const char *path)                        { (void)path; return 0; }
const unsigned char *PC_HdVideoFrame(int w, int *ow, int *oh){ (void)w;(void)ow;(void)oh; return NULL; }
void PC_HdVideoClose(void)                                   { }
int  PC_HdVideoActive(void)                                  { return 0; }
#endif

#ifdef HDVIDEO_TEST   /* standalone: cc -DVH_HD_VIDEO -DHDVIDEO_TEST pc_hdvideo.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil) */
int main(int argc, char **argv) {
    int w, h, i, dump = argc > 2 ? atoi(argv[2]) : 200;
    if (argc < 2) { fprintf(stderr, "usage: %s file.mp4 [frame]\n", argv[0]); return 2; }
    if (!PC_HdVideoOpen(argv[1])) { fprintf(stderr, "open failed\n"); return 1; }
    for (i = 0; i <= dump; i++) {
        const unsigned char *rgb = PC_HdVideoFrame(i, &w, &h);
        if (!rgb) { fprintf(stderr, "no frame at %d\n", i); break; }
        if (i == dump) {
            FILE *f = fopen("/tmp/hdvid/decoded.ppm", "wb");
            if (f) { fprintf(f, "P6\n%d %d\n255\n", w, h); fwrite(rgb, 1, (size_t)w * h * 3, f); fclose(f);
                     fprintf(stderr, "frame %d -> /tmp/hdvid/decoded.ppm (%dx%d)\n", dump, w, h); }
        }
    }
    PC_HdVideoClose();
    return 0;
}
#endif
