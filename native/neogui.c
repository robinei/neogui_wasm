#include "neogui.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

UIContext ui_context;
#define ctx ui_context

#define CLEAR_BUF(type, name) memset(ctx.name, 0, sizeof(type) * ctx.elem_count);

#define REALLOC_BUF(type, name) \
    ctx.name = (type *)realloc(ctx.name, sizeof(type) * ctx.elem_capacity); \
    memset(ctx.name + ctx.elem_count, 0, sizeof(type) * (ctx.elem_capacity - ctx.elem_count));



static Elem get_single_child(Elem e) {
    assert(e);
    ParentNode *parent_node = ctx.parent_node + e;
    assert(!parent_node->first_child || parent_node->first_child == parent_node->last_child);
    return parent_node->first_child;
}

static void perform_layout(Elem e, Constraints *c) {
    assert(e);
    LayoutFunc layout_func = ctx.layout_func[e];
    assert(layout_func);
    layout_func(e, c);
}

static void constrain_and_grow_size(Elem e, Constraints *c) {
    Vec2 *size = ctx.size + e;
    if (c->max_size.x < INFINITY) { size->x = c->max_size.x; }
    if (c->max_size.y < INFINITY) { size->y = c->max_size.y; }
    if (size->x < c->min_size.x) { size->x = c->min_size.x; }
    if (size->y < c->min_size.y) { size->y = c->min_size.y; }
}
static void constrain_size(Elem e, Constraints *c) {
    Vec2 *size = ctx.size + e;
    if (size->x > c->max_size.x) { size->x = c->max_size.x; }
    if (size->y > c->max_size.y) { size->y = c->max_size.y; }
    if (size->x < c->min_size.x) { size->x = c->min_size.x; }
    if (size->y < c->min_size.y) { size->y = c->min_size.y; }
}


static void modify_padding_constraints_axis(Constraints *c, EdgeInsets *padding, Axis axis) {
    if ((&c->max_size.x)[axis] < INFINITY) {
        (&c->max_size.x)[axis] -= (&padding->left)[EDGE_LEFT + axis] + (&padding->left)[EDGE_RIGHT + axis];
        if ((&c->max_size.x)[axis] < 0) {
            (&c->max_size.x)[axis] = 0;
        }
        if ((&c->min_size.x)[axis] > (&c->max_size.x)[axis]) {
            (&c->min_size.x)[axis] = (&c->max_size.x)[axis];
        }
    }
}
static void perform_padding_layout(Elem e, Constraints *c) {
    EdgeInsets *padding = &ctx.args[e].padding;
    Vec2 child_size = (Vec2) { 0, 0 };
    Elem child = get_single_child(e);

    if (child) {
        Constraints nc = *c;
        modify_padding_constraints_axis(&nc, padding, AXIS_X);
        modify_padding_constraints_axis(&nc, padding, AXIS_Y);
        perform_layout(child, &nc);
        child_size = ctx.size[child];
        ctx.pos[child] = (Vec2) { padding->left, padding->top };
    }

    ctx.size[e] = (Vec2) {
        child_size.x + padding->left + padding->right,
        child_size.y + padding->top + padding->bottom
    };
    constrain_size(e, c);
}

static void perform_align_layout(Elem e, Constraints *c) {
    Elem child = get_single_child(e);
    if (!child) {
        constrain_and_grow_size(e, c);
        return;
    }

    perform_layout(child, &(Constraints) {
        .min_size = { 0, 0 },
        .max_size = { INFINITY, INFINITY }
    });
    constrain_and_grow_size(e, c);

    Vec2 *child_pos = ctx.pos + child;
    Vec2 *child_size = ctx.size + child;
    Vec2 *size = ctx.size + e;
    Vec2 *align = &ctx.args[e].align;

    child_pos->x = (size->x - child_size->x) * (align->x + 1.0f) * 0.5f;
    child_pos->y = (size->y - child_size->y) * (align->y + 1.0f) * 0.5f;
}

static void perform_sized_box_layout(Elem e, Constraints *c) {
    Vec2 *sized_box = &ctx.args[e].sized_box;
    Elem child = get_single_child(e);
    if (child) {
        Constraints nc = *c;
        if (sized_box->x > 0) { nc.min_size.x = nc.max_size.x = sized_box->x; }
        if (sized_box->y > 0) { nc.min_size.y = nc.max_size.y = sized_box->y; }
        perform_layout(child, &nc);
        ctx.size[e] = ctx.size[child];
    } else {
        ctx.size[e] = *sized_box;
        constrain_size(e, c);
    }
}

static void perform_layout_builder_layout(Elem e, Constraints *c) {
    LayoutBuilderArgs *args = &ctx.args[e].layout_builder;

    Elem child = args->build_func(e, c, args->userdata);
    assert(child == get_single_child(e));

    perform_layout(child, c);

    ctx.size[e] = ctx.size[child];
}

static void perform_flex_layout(Elem e, Constraints *c) {
    FlexArgs *args = &ctx.args[e].flex;
    Axis main_axis = args->main_axis;
    Axis cross_axis = !main_axis;

    float incoming_cross_axis_min = (&c->min_size.x)[cross_axis];
    float max_cross_axis_size = incoming_cross_axis_min;
    float sum_main_axis_flex_size = 0;
    float sum_main_axis_non_flex_size = 0;
    float sum_flex_factors = 0;
    
    Constraints nc = *c;
    (&nc.min_size.x)[cross_axis] = 0;
    (&nc.min_size.x)[main_axis] = 0;
    (&nc.max_size.x)[main_axis] = INFINITY;
    if (args->cross_axis_stretch > 0) {
        assert(args->cross_axis_stretch <= 1.0f);
        float incoming_cross_axis_max = (&c->max_size.x)[cross_axis];
        if (incoming_cross_axis_max < INFINITY) {
            (&nc.max_size.x)[cross_axis] = (&nc.min_size.x)[cross_axis] = incoming_cross_axis_max * args->cross_axis_stretch;
        }
    }

    for (Elem child = ctx.parent_node[e].first_child; child; child = ctx.node[child].next_sibling) {
        float flex_factor = ctx.flex_factor[child];
        assert(flex_factor >= 0);
        if (flex_factor > 0) {
            sum_flex_factors += flex_factor;
        } else {
            perform_layout(child, &nc);
            sum_main_axis_non_flex_size += (&ctx.size[child].x)[main_axis];
            float cross_axis_size = (&ctx.size[child].x)[cross_axis];
            if (cross_axis_size > max_cross_axis_size) {
                max_cross_axis_size = cross_axis_size;
            }
        }
    }

    float available_main_axis_flex_size = 0;
    float incoming_main_axis_max = (&c->max_size.x)[main_axis];
    if (incoming_main_axis_max < INFINITY) {
        available_main_axis_flex_size = incoming_main_axis_max - sum_main_axis_non_flex_size;
        if (available_main_axis_flex_size < 0) {
            available_main_axis_flex_size = 0;
        }
    }

    for (Elem child = ctx.parent_node[e].first_child; child; child = ctx.node[child].next_sibling) {
        float flex_factor = ctx.flex_factor[child];
        if (flex_factor > 0) {
            (&nc.max_size.x)[main_axis] = available_main_axis_flex_size * (flex_factor / sum_flex_factors);
            perform_layout(child, &nc);
            sum_main_axis_flex_size += (&ctx.size[child].x)[main_axis];
            float cross_axis_size = (&ctx.size[child].x)[cross_axis];
            if (cross_axis_size > max_cross_axis_size) {
                max_cross_axis_size = cross_axis_size;
            }
        }
    }

    float sum_main_axis_size = sum_main_axis_flex_size + sum_main_axis_non_flex_size;
    (&ctx.size[e].x)[main_axis] = sum_main_axis_size;
    (&ctx.size[e].x)[cross_axis] = max_cross_axis_size;

    float main_axis_offset = 0;
    for (Elem child = ctx.parent_node[e].first_child; child; child = ctx.node[child].next_sibling) {
        Vec2 *child_pos = ctx.pos + child;
        Vec2 *child_size = ctx.size + child;
        (&child_pos->x)[cross_axis] = (max_cross_axis_size - (&child_size->x)[cross_axis]) * (args->cross_axis_align + 1.0f) * 0.5f;
        (&child_pos->x)[main_axis] = main_axis_offset;
        main_axis_offset += (&child_size->x)[main_axis];
    }
}



InputState *ui_flip_input() {
    ctx.input_index = !ctx.input_index;
    return ctx.input + ctx.input_index;
}

static void calc_world_positions() {
    Node *node = ctx.node;
    Vec2 *pos = ctx.pos, *world_pos = ctx.world_pos;
    int elem_count = ctx.elem_count;

    world_pos[0] = pos[0];
    for (int e = 1; e < elem_count; ++e) {
        Elem parent = node[e].parent;
        world_pos[e].x = world_pos[parent].x + pos[e].x;
        world_pos[e].y = world_pos[parent].y + pos[e].y;
    }
}

static void draw_elements() {
    for (Elem e = 0; e < ui_context.elem_count; ++e) {
        Color color = ui_context.color[e];
        if (color.a) {
            Vec2 pos = ui_context.world_pos[e];
            Vec2 size = ui_context.size[e];
            set_fill_color(color.r, color.g, color.b, color.a);
            fill_rect(pos.x, pos.y, size.x, size.y);
        }
    }
}

void ui_frame_begin() {
    if (ctx.elem_count) {
        FOREACH_ELEM_COMPONENT(CLEAR_BUF)
        ctx.elem_count = 0;
    }
}

void ui_frame_end(float root_width, float root_height) {
    perform_layout(1, &(Constraints) {
        .min_size = { root_width, root_height},
        .max_size = { root_width, root_height},
    });
    calc_world_positions();
    draw_elements();
}


Elem elem_create(Elem parent) {
    if (ctx.elem_count >= ctx.elem_capacity) {
        ctx.elem_capacity = ctx.elem_capacity ? ctx.elem_capacity * 2 : 256;
        FOREACH_ELEM_COMPONENT(REALLOC_BUF)
    }
    Elem e;
    if (parent) {
        assert(parent < ctx.elem_count);
        e = ctx.elem_count++;
        ParentNode *parent_node = ctx.parent_node + parent;
        ctx.node[e].parent = parent;
        if (parent_node->last_child) {
            ctx.node[parent_node->last_child].next_sibling = e;
        } else {
            parent_node->first_child = e;
        }
        parent_node->last_child = e;
    } else {
        assert(ctx.elem_count == 0);
        e = 1; // skip element at index 0 (0 will mean invalid element / none)
        ctx.elem_count = 2;
    }
    ctx.layout_func[e] = constrain_size;
    return e;
}

Elem padding_create(Elem parent, float left, float top, float right, float bottom) {
    Elem e = elem_create(parent);
    ctx.layout_func[e] = perform_padding_layout;
    ctx.args[e].padding = (EdgeInsets) { left, top, right, bottom };
    return e;
}

Elem center_create(Elem parent) {
    return align_create(parent, 0, 0);
}
Elem align_create(Elem parent, float x, float y) {
    Elem e = elem_create(parent);
    ctx.layout_func[e] = perform_align_layout;
    ctx.args[e].align = (Vec2) { x, y };
    return e;
}

Elem sized_box_create(Elem parent, float w, float h) {
    Elem e = elem_create(parent);
    ctx.layout_func[e] = perform_sized_box_layout;
    ctx.args[e].sized_box = (Vec2) { w, h };
    return e;
}

Elem row_create(Elem parent) {
    return flex_create(parent, AXIS_X);
}
Elem column_create(Elem parent) {
    return flex_create(parent, AXIS_Y);
}
Elem flex_create(Elem parent, Axis main_axis) {
    Elem e = elem_create(parent);
    ctx.layout_func[e] = perform_flex_layout;
    ctx.args[e].flex.main_axis = main_axis;
    return e;
}
Elem spacer_create(Elem parent, float flex_factor) {
    Elem e = elem_create(parent);
    ctx.flex_factor[e] = flex_factor;
    return e;
}

Elem layout_builder_create(Elem parent, LayoutBuildFunc build_func, void *userdata) {
    Elem e = elem_create(parent);
    ctx.layout_func[e] = perform_layout_builder_layout;
    ctx.args[e].layout_builder = (LayoutBuilderArgs) { build_func, userdata };
    return e;
}








void test_ui(float root_width, float root_height) {
    ui_frame_begin();
    USE(padding_create(0, 100, 50, 100, 50));
    *color = (Color) {0, 255, 0, 255};
    {
        USE(center_create(_));
        *color = (Color) {255, 0, 0, 255};
        {
            USE(padding_create(_, 10, 10, 10, 10));
            *color = (Color) {0, 0, 0, 255};
            {
                USE(sized_box_create(_, 250, 150));
                *color = (Color) {128, 128, 128, 255};
                {
                    USE(row_create(_));
                    spacer_create(_, 1);
                    {
                        USE(elem_create(_));
                        *size = (Vec2) {100, 100};
                        *color = (Color) {0, 0, 255, 255};
                    }
                    spacer_create(_, 1);
                    {
                        USE(elem_create(_));
                        *size = (Vec2) {50, 50};
                        *color = (Color) {255, 255, 255, 255};
                    }
                    spacer_create(_, 1);
                }
            }
        }
    }
    ui_frame_end(root_width, root_height);
}
