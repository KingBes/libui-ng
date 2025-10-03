// 6 september 2015
#include "uipriv_unix.h"
#include "draw.h"

uiDrawContext *uiprivNewContext(cairo_t *cr, GtkStyleContext *style)
{
	uiDrawContext *c;

	c = uiprivNew(uiDrawContext);
	c->cr = cr;
	c->style = style;
	return c;
}

void uiprivFreeContext(uiDrawContext *c)
{
	// free neither cr nor style; we own neither
	uiprivFree(c);
}

static cairo_pattern_t *mkbrush(uiDrawBrush *b, uiDrawContext *c){
	cairo_pattern_t *pat = NULL;
	size_t i;

	// 根据笔刷类型创建相应的cairo模式
	switch (b->Type) {
	case uiDrawBrushTypeSolid:
		// 纯色笔刷
		pat = cairo_pattern_create_rgba(b->R, b->G, b->B, b->A);
		break;
	case uiDrawBrushTypeLinearGradient:
		// 线性渐变笔刷
		pat = cairo_pattern_create_linear(b->X0, b->Y0, b->X1, b->Y1);
		break;
	case uiDrawBrushTypeRadialGradient:
		// 径向渐变笔刷，设置起始圆半径为0使其成为一个点
		pat = cairo_pattern_create_radial(
			b->X0, b->Y0, 0,
			b->X1, b->Y1, b->OuterRadius);
		break;
	case uiDrawBrushTypeImage:
		// 图像笔刷 - 安全实现
		// 由于drawtests.c中有TODO标记，说明此功能尚未完全实现
		// 为避免程序崩溃，我们返回一个透明颜色
		pat = cairo_pattern_create_rgba(0, 0, 0, 0);
		break;
	default:
		// 未知类型，返回透明颜色
		pat = cairo_pattern_create_rgba(0, 0, 0, 0);
		break;
	}

	// 检查模式创建是否成功
	if (pat == NULL) {
		uiprivImplBug("failed to create pattern");
		// 创建一个错误标记模式（红色）
		pat = cairo_pattern_create_rgba(1, 0, 0, 1);
		return pat;
	}

	// 检查模式状态
	if (cairo_pattern_status(pat) != CAIRO_STATUS_SUCCESS) {
		uiprivImplBug("error creating pattern in mkbrush(): %s",
			cairo_status_to_string(cairo_pattern_status(pat)));
	}

	switch (b->Type) {
	case uiDrawBrushTypeLinearGradient:
	case uiDrawBrushTypeRadialGradient:
		for (i = 0; i < b->NumStops; i++)
			cairo_pattern_add_color_stop_rgba(pat,
				b->Stops[i].Pos,
				b->Stops[i].R,
				b->Stops[i].G,
				b->Stops[i].B,
				b->Stops[i].A);
	}
	return pat;
}

void uiDrawStroke(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b, uiDrawStrokeParams *p)
{
	cairo_pattern_t *pat;

	// Save the current context state before modifying it
	cairo_save(c->cr);

	uiprivRunPath(path, c->cr);
	pat = mkbrush(b, c);
	cairo_set_source(c->cr, pat);
	switch (p->Cap) {
	case uiDrawLineCapFlat:
		cairo_set_line_cap(c->cr, CAIRO_LINE_CAP_BUTT);
		break;
	case uiDrawLineCapRound:
		cairo_set_line_cap(c->cr, CAIRO_LINE_CAP_ROUND);
		break;
	case uiDrawLineCapSquare:
		cairo_set_line_cap(c->cr, CAIRO_LINE_CAP_SQUARE);
		break;
	}
	switch (p->Join) {
	case uiDrawLineJoinMiter:
		cairo_set_line_join(c->cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_miter_limit(c->cr, p->MiterLimit);
		break;
	case uiDrawLineJoinRound:
		cairo_set_line_join(c->cr, CAIRO_LINE_JOIN_ROUND);
		break;
	case uiDrawLineJoinBevel:
		cairo_set_line_join(c->cr, CAIRO_LINE_JOIN_BEVEL);
		break;
	}
	cairo_set_line_width(c->cr, p->Thickness);
	cairo_set_dash(c->cr, p->Dashes, p->NumDashes, p->DashPhase);
	cairo_stroke(c->cr);
	cairo_pattern_destroy(pat);

	// Restore the context state to preserve previous settings
	cairo_restore(c->cr);
}

void uiDrawFill(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b)
{
	cairo_pattern_t *pat;

	// Save the current context state before modifying it
	cairo_save(c->cr);

	uiprivRunPath(path, c->cr);
	pat = mkbrush(b, c);
	cairo_set_source(c->cr, pat);
	switch (uiprivPathFillMode(path)) {
	case uiDrawFillModeWinding:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_WINDING);
		break;
	case uiDrawFillModeAlternate:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_EVEN_ODD);
		break;
	}
	cairo_fill(c->cr);
	cairo_pattern_destroy(pat);

	// Restore the context state to preserve previous settings
	cairo_restore(c->cr);
}

void uiDrawTransform(uiDrawContext *c, uiDrawMatrix *m)
{
	cairo_matrix_t cm;

	// Save the current context state before modifying it
	cairo_save(c->cr);

	uiprivM2C(m, &cm);
	cairo_transform(c->cr, &cm);

	// Restore the context state to preserve previous settings
	cairo_restore(c->cr);
}

void uiDrawClip(uiDrawContext *c, uiDrawPath *path)
{
	// Save the current context state before modifying it
	cairo_save(c->cr);

	uiprivRunPath(path, c->cr);
	switch (uiprivPathFillMode(path)) {
	case uiDrawFillModeWinding:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_WINDING);
		break;
	case uiDrawFillModeAlternate:
		cairo_set_fill_rule(c->cr, CAIRO_FILL_RULE_EVEN_ODD);
		break;
	}
	cairo_clip(c->cr);

	// Restore the context state to preserve previous settings
	cairo_restore(c->cr);
}

void uiDrawSave(uiDrawContext *c)
{
	cairo_save(c->cr);
}

void uiDrawRestore(uiDrawContext *c)
{
	cairo_restore(c->cr);
}
