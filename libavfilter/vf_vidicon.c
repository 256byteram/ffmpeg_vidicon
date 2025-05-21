/*
 * FFmpeg video filter: vidicon
 * Simulates vidicon light trails by accumulating and fading video frames
 */

#include "libavfilter/formats.h"
#include "libavfilter/filters.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/internal.h"
#include "libavfilter/video.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/mem.h"  // for av_malloc(), av_free()
#include <emmintrin.h>
#include <math.h>

#define FADE_FACTOR 0.5f
#define GAIN 1.0f

typedef struct {
    const AVClass *class;

    float fade_r, fade_g, fade_b;	// Fade options per channel
    float gain_r, gain_g, gain_b;	// Gain options per channel
    
    float fade;		// Global fade option
    float gain;		// Global gain option

    int width;
    int height;

    float **accum_r;
    float **accum_g;
    float **accum_b;

    float blend_factor;
    float fade_factor;

    uint8_t *src_r;
    uint8_t *src_g;
    uint8_t *src_b;
    
    uint8_t *out_r;
    uint8_t *out_g;
    uint8_t *out_b;

} VidiconTrailContext;

static int config_input(AVFilterLink *inlink) {

	VidiconTrailContext *ctx = inlink->dst->priv;

	ctx->width = inlink->w;
	ctx->height = inlink->h;

	// Allocate persistent accumulation buffers (float **)
	ctx->accum_r = av_mallocz(ctx->height * sizeof(float *));
	ctx->accum_g = av_mallocz(ctx->height * sizeof(float *));
	ctx->accum_b = av_mallocz(ctx->height * sizeof(float *));

	for (int y = 0; y < ctx->height; y++) {
		ctx->accum_r[y] = av_malloc(ctx->width * sizeof(float));
		ctx->accum_g[y] = av_malloc(ctx->width * sizeof(float));
		ctx->accum_b[y] = av_malloc(ctx->width * sizeof(float));
	}

    	ctx->src_r = av_mallocz(ctx->height * sizeof(float *));
	ctx->src_g = av_mallocz(ctx->height * sizeof(float *));
	ctx->src_b = av_mallocz(ctx->height * sizeof(float *));

    	ctx->out_r = av_mallocz(ctx->height * sizeof(float *));
	ctx->out_g = av_mallocz(ctx->height * sizeof(float *));
	ctx->out_b = av_mallocz(ctx->height * sizeof(float *));

	// Use shared value if per-channel is not explicitly set
	if (ctx->fade_r < 0) ctx->fade_r = ctx->fade >= 0 ? ctx->fade : 0.5f;
	if (ctx->fade_g < 0) ctx->fade_g = ctx->fade >= 0 ? ctx->fade : 0.5f;
	if (ctx->fade_b < 0) ctx->fade_b = ctx->fade >= 0 ? ctx->fade : 0.5f;

	if (ctx->gain_r < 0) ctx->gain_r = ctx->gain >= 0 ? ctx->gain : 1.0f;
	if (ctx->gain_g < 0) ctx->gain_g = ctx->gain >= 0 ? ctx->gain : 1.0f;
	if (ctx->gain_b < 0) ctx->gain_b = ctx->gain >= 0 ? ctx->gain : 1.0f;

	av_log(ctx, AV_LOG_INFO, "Fade R/G/B = %f / %f / %f\n", ctx->fade_r, ctx->fade_g, ctx->fade_b);
	av_log(ctx, AV_LOG_INFO, "Gain R/G/B = %f / %f / %f\n", ctx->gain_r, ctx->gain_g, ctx->gain_b);

	return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_RGB24,
        AV_PIX_FMT_NONE
    };

    return ff_set_common_formats(ctx, ff_make_format_list(pix_fmts));
}

// Split interleaved RGB24 input into separate float R, G, B planes
static void split_rgb24_to_planes(const uint8_t *rgb, uint8_t *r, uint8_t *g, uint8_t *b, int width)
{
    for (int x = 0; x < width; x++) {
        r[x] = rgb[3 * x + 0];
        g[x] = rgb[3 * x + 1];
        b[x] = rgb[3 * x + 2];
    }
}

// Merge float R, G, B planes back into interleaved RGB24 output
static void merge_planes_to_rgb24(uint8_t *rgb, const uint8_t *r, const uint8_t *g, const uint8_t *b, int width)
{
    for (int x = 0; x < width; x++) {
        rgb[3 * x + 0] = r[x];
        rgb[3 * x + 1] = g[x];
        rgb[3 * x + 2] = b[x];
    }
}


// Process a single color plane (8 pixels per iteration)
static void process_color_plane_sse2(uint8_t *dst, const uint8_t *src, float *accum, int width,
                                     float fade_strength, float blend_factor)
{
    __m128 vfade = _mm_set1_ps(fade_strength);
    __m128 vblend = _mm_set1_ps(blend_factor);

    for (int x = 0; x < width; x += 8) {
        // Load 8 uint8_t values
        __m128i vsrc8 = _mm_loadl_epi64((__m128i*)&src[x]);
        __m128i vsrc16 = _mm_unpacklo_epi8(vsrc8, _mm_setzero_si128());
        __m128i vsrc32_lo = _mm_unpacklo_epi16(vsrc16, _mm_setzero_si128());
        __m128i vsrc32_hi = _mm_unpackhi_epi16(vsrc16, _mm_setzero_si128());

        // Convert to float
        __m128 vsrc_f_lo = _mm_cvtepi32_ps(vsrc32_lo);
        __m128 vsrc_f_hi = _mm_cvtepi32_ps(vsrc32_hi);

        // Load accumulation buffer
        __m128 vacc_lo = _mm_loadu_ps(&accum[x + 0]);
        __m128 vacc_hi = _mm_loadu_ps(&accum[x + 4]);

        // Accumulation: decay + blend in new input
        vacc_lo = _mm_add_ps(_mm_mul_ps(vacc_lo, vfade), _mm_mul_ps(vsrc_f_lo, vblend));
        vacc_hi = _mm_add_ps(_mm_mul_ps(vacc_hi, vfade), _mm_mul_ps(vsrc_f_hi, vblend));

        // Store updated accumulation buffer
        _mm_storeu_ps(&accum[x + 0], vacc_lo);
        _mm_storeu_ps(&accum[x + 4], vacc_hi);

        // Convert to int and clamp
        __m128i vi32_lo = _mm_cvtps_epi32(vacc_lo);
        __m128i vi32_hi = _mm_cvtps_epi32(vacc_hi);

        __m128i vi16 = _mm_packs_epi32(vi32_lo, vi32_hi);
        __m128i vi8 = _mm_packus_epi16(vi16, _mm_setzero_si128());

        // Store 8 result bytes
        _mm_storel_epi64((__m128i*)&dst[x], vi8);
    }
}

static int filter_frame(AVFilterLink *inlink, AVFrame *frame) {
    AVFilterContext *ctx = inlink->dst;
    VidiconTrailContext *s = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];

    int width = frame->width;
    int height = frame->height;
    int stride = frame->linesize[0];

    av_frame_make_writable(frame);


    for (int y = 0; y < height; y++)
    {
	uint8_t* line = frame->data[0] + y * stride;
	   
	split_rgb24_to_planes (line, s->src_r, s->src_g, s->src_b, width);
	  
	float* accum_r = s->accum_r[y];
	float* accum_g = s->accum_g[y];
	float* accum_b = s->accum_b[y];
	   
	process_color_plane_sse2(s->out_r, s->src_r, accum_r, width, s->fade_r, s->gain_r);
	process_color_plane_sse2(s->out_g, s->src_g, accum_g, width, s->fade_g, s->gain_g);
	process_color_plane_sse2(s->out_b, s->src_b, accum_b, width, s->fade_b, s->gain_b);
	merge_planes_to_rgb24(line, s->out_r, s->out_g, s->out_b, width);

    }

    return ff_filter_frame(outlink, frame);
}

static av_cold void uninit(AVFilterContext *ctx) {
	VidiconTrailContext *s = ctx->priv;

	for (int y = 0; y < s->height; y++) {
		av_free(s->accum_r[y]);
		av_free(s->accum_g[y]);
		av_free(s->accum_b[y]);
	}
	av_free(s->accum_r);
	av_free(s->accum_g);
	av_free(s->accum_b);

	av_free(s->src_r);
	av_free(s->src_g);
	av_free(s->src_b);

	av_free(s->out_r);
	av_free(s->out_g);
	av_free(s->out_b);

}

static const AVFilterPad vidicon_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
        .config_props = config_input,
    }
};

static const AVFilterPad vidicon_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    }
};

#define OFFSET(x) offsetof(VidiconTrailContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption vidicon_options[] = {
	// Shared parameters
	{ "fade", "Fade factor for all channels", OFFSET(fade), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },
	{ "gain", "Gain factor for all channels", OFFSET(gain), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },

	// Per-channel overrides
	{ "fade_r", "Fade factor for red channel", OFFSET(fade_r), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },
	{ "fade_g", "Fade factor for green channel", OFFSET(fade_g), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },
	{ "fade_b", "Fade factor for blue channel", OFFSET(fade_b), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },

	{ "gain_r", "Gain factor for red channel", OFFSET(gain_r), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },
	{ "gain_g", "Gain factor for green channel", OFFSET(gain_g), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },
	{ "gain_b", "Gain factor for blue channel", OFFSET(gain_b), AV_OPT_TYPE_FLOAT, {.dbl = -1.0}, -1.0, 1.0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },


	{ NULL }
};

static const AVClass vidicon_class = {
	.class_name = "vidicon",
	.item_name = av_default_item_name,
	.option = vidicon_options,
	.version = LIBAVUTIL_VERSION_INT
};

const AVFilter ff_vf_vidicon = {
    .name          = "vidicon",
    .description   = NULL_IF_CONFIG_SMALL("Simulate vidicon light trail persistence."),
    .priv_size     = sizeof(VidiconTrailContext),
    .uninit        = uninit,
    FILTER_INPUTS(vidicon_inputs),
    FILTER_OUTPUTS(vidicon_outputs),
    .formats = {.query_func = query_formats},
    .formats_state = FF_FILTER_FORMATS_QUERY_FUNC,
    .priv_class    = &vidicon_class,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC
};

