/* nuklear - 1.32.0 - public domain */

#define NK_INCLUDE_DEFAULT_ALLOCATOR
#include "nuklear.h"

#include <memory>
#include <blend2d.h>

float nk_text_width(BLFont *font, float h, const char *text, int len)
{
    if (font) {
        BLGlyphBuffer buf;
        buf.setUtf8Text(text, len);
        BLTextMetrics metrics;
        if (h == font->size()) {
            font->shape(buf);
            font->getTextMetrics(buf, metrics);
        } else {
            BLFont f;
            f.createFromFace(font->face(), h);
            f.shape(buf);
            f.getTextMetrics(buf, metrics);
        }
        return metrics.advance.x;
    }
    return 0;
}

static inline BLRgba32 convColor(const nk_color &color) {
    return BLRgba32(color.r, color.g, color.b, color.a);
}

void
nk_render(BLContext *ctx, const struct nk_command *cmd)
{
	ctx->setCompOp(BL_COMP_OP_SRC_COPY);

    switch (cmd->type) {
    case NK_COMMAND_NOP: break;
    case NK_COMMAND_SCISSOR: {
        const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
        ctx->clipToRect(s->x, s->y, s->w, s->h);
    } break;
    case NK_COMMAND_LINE: {
        const struct nk_command_line *l = (const struct nk_command_line *)cmd;
        ctx->setStrokeStyle(convColor(l->color));
        ctx->setStrokeWidth(l->line_thickness);
        ctx->strokeLine(l->begin.x, l->begin.y, l->end.x, l->end.y);
    } break;
    case NK_COMMAND_RECT: {
        const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
        ctx->setStrokeStyle(convColor(r->color));
        ctx->setStrokeWidth(r->line_thickness);
        if (r->rounding) {
            ctx->strokeRoundRect(r->x, r->y, r->w, r->h, r->rounding);
        } else {
            ctx->strokeRect(r->x, r->y, r->w, r->h);
        }
    } break;
    case NK_COMMAND_RECT_FILLED: {
        const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
        ctx->setFillStyle(convColor(r->color));
        if (r->rounding) {
            ctx->fillRoundRect(r->x, r->y, r->w, r->h, r->rounding);
        } else {
            ctx->fillRect(r->x, r->y, r->w, r->h);
        }
    } break;
    case NK_COMMAND_CIRCLE: {
        const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
        ctx->setStrokeStyle(convColor(c->color));
        ctx->setStrokeWidth(c->line_thickness);
        double rx = c->w/2.0;
        double ry = c->h/2.0;
        double cx = c->x + rx;
        double cy = c->y + ry;
        ctx->strokeEllipse(cx, cy, rx, ry);
    } break;
    case NK_COMMAND_CIRCLE_FILLED: {
        const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
        ctx->setFillStyle(convColor(c->color));
        double rx = c->w/2.0;
        double ry = c->h/2.0;
        double cx = c->x + rx;
        double cy = c->y + ry;
        ctx->fillEllipse(cx, cy, rx, ry);
    } break;
    case NK_COMMAND_ARC: {
        const struct nk_command_arc *q = (const struct nk_command_arc *)cmd;
        ctx->setStrokeStyle(convColor(q->color));
        ctx->setStrokeWidth(q->line_thickness);
        ctx->strokeArc(q->cx, q->cy, q->r, q->a[0], q->a[1]);
    } break;
    case NK_COMMAND_ARC_FILLED: {
        const struct nk_command_arc_filled *q = (const struct nk_command_arc_filled *)cmd;
        ctx->setFillStyle(convColor(q->color));
        ctx->fillPie(q->cx, q->cy, q->r, q->a[0], q->a[1]);
    } break;
    case NK_COMMAND_TRIANGLE: {
        const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
        ctx->setStrokeStyle(convColor(t->color));
        ctx->setStrokeWidth(t->line_thickness);
        ctx->strokeTriangle(t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y);
    } break;
    case NK_COMMAND_TRIANGLE_FILLED: {
        const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
        ctx->setFillStyle(convColor(t->color));
        ctx->fillTriangle(t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y);
    } break;
    case NK_COMMAND_POLYGON: {
        const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
        auto pts = std::make_unique<BLPoint[]>(p->point_count);
        for (int i=0;i<p->point_count;i++) {
            pts[i] = BLPoint(p->points[i].x, p->points[i].y);
        }
        ctx->setStrokeStyle(convColor(p->color));
        ctx->setStrokeWidth(p->line_thickness);
        ctx->strokePolygon(pts.get(), p->point_count);
    } break;
    case NK_COMMAND_POLYGON_FILLED: {
        const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
        auto pts = std::make_unique<BLPoint[]>(p->point_count);
        for (int i=0;i<p->point_count;i++) {
            pts[i] = BLPoint(p->points[i].x, p->points[i].y);
        }
        ctx->setFillStyle(convColor(p->color));
        ctx->fillPolygon(pts.get(), p->point_count);
    } break;
    case NK_COMMAND_POLYLINE: {
        const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
        auto pts = std::make_unique<BLPoint[]>(p->point_count);
        for (int i=0;i<p->point_count;i++) {
            pts[i] = BLPoint(p->points[i].x, p->points[i].y);
        }
        ctx->setStrokeStyle(convColor(p->color));
        ctx->setStrokeWidth(p->line_thickness);
        ctx->strokePolyline(pts.get(), p->point_count);
        } break;
    case NK_COMMAND_TEXT: {
        const struct nk_command_text *t = (const struct nk_command_text*)cmd;
        auto font = (BLFont*)t->font->userdata.ptr;
        if (font) {
            auto bg = convColor(t->background);
            auto fg = convColor(t->foreground);
            if (bg) {
                ctx->setFillStyle(bg);
                ctx->fillRect(t->x, t->y, t->w, t->h);
            }
            ctx->setFillStyle(fg);
            ctx->fillUtf8Text(BLPoint(t->x, t->y + font->metrics().ascent), *font, (const char*)t->string, t->length);
        }
    } break;
    case NK_COMMAND_CURVE: {
        const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
        ctx->setStrokeStyle(convColor(q->color));
        ctx->setStrokeWidth(q->line_thickness);
        BLPath path;
        path.moveTo(q->begin.x, q->begin.y);
        path.cubicTo(q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x, q->ctrl[2].y, q->end.x, q->end.y);
        ctx->strokePath(path);
    } break;
    case NK_COMMAND_RECT_MULTI_COLOR: {
        const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
        BLRgba32 colors[4];
        colors[0] = convColor(r->left);
        colors[1] = convColor(r->top);
        colors[2] = convColor(r->right);
        colors[3] = convColor(r->bottom);
        //一旦仮
        ctx->setFillStyle(colors[0]);
        ctx->fillRect(r->x, r->y, r->w, r->h);
    } break;
    case NK_COMMAND_IMAGE: {
        const struct nk_command_image *i = (const struct nk_command_image *)cmd;
/*
        auto image = (NkImage*)i->img.handle.ptr;
        if (image->mImage) {
            ctx->drawImageRect(image->mImage, SkRect::MakeXYWH(i->x, i->y, i->w, i->h), SkSamplingOptions(SkFilterMode::kNearest, SkMipmapMode::kNearest));
        };
*/
    } break;
    case NK_COMMAND_CUSTOM:
    default: break;
    }
}
