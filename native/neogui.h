#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <assert.h>

#if __EMSCRIPTEN__
#include <emscripten/em_macros.h>
#define IMPORT(name) EM_IMPORT(name)
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define IMPORT(name)
#define EXPORT
#endif
#define IGNORE_UNUSED __attribute__((unused))

extern void set_fill_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) IMPORT(set_fill_color);
extern void fill_rect(float x, float y, float w, float h) IMPORT(fill_rect);


typedef enum Axis {
    AXIS_X,
    AXIS_Y,
} Axis;

typedef enum Edge {
    EDGE_LEFT,
    EDGE_TOP,
    EDGE_RIGHT,
    EDGE_BOTTOM,
} Edge;

typedef struct Vec2 {
    float x, y;
} Vec2;

typedef struct Rect {
    Vec2 pos, size;
} Rect;

typedef struct EdgeInsets {
    float left, top, right, bottom;
} EdgeInsets;

typedef struct Constraints {
    Vec2 min_size, max_size;
} Constraints;

typedef struct Color {
    uint8_t r, g, b, a;
} Color;


typedef uint32_t Elem;

typedef struct Node {
    Elem parent;
    Elem next_sibling;
} Node;

typedef struct ParentNode {
    Elem first_child;
    Elem last_child;
} ParentNode;

typedef void (*LayoutFunc)(Elem e, Constraints *c);


typedef Elem (*LayoutBuildFunc)(Elem parent, Constraints *c, void *userdata);

typedef struct LayoutBuilderArgs {
    LayoutBuildFunc build_func;
    void *userdata;
} LayoutBuilderArgs;

typedef struct FlexArgs {
    Axis main_axis;
    float cross_axis_align;
    float cross_axis_stretch;
} FlexArgs;

typedef union ArgsUnion {
    EdgeInsets padding;
    Vec2 align; // from -1 to 1 along each axis, centered being 0, -1 being start/left/top
    Vec2 sized_box;
    FlexArgs flex;
    LayoutBuilderArgs layout_builder;
} ArgsUnion;


#define FOREACH_ELEM_COMPONENT(X) \
    X(Node, node) \
    X(ParentNode, parent_node) \
    X(Vec2, size) \
    X(Vec2, pos) \
    X(Vec2, world_pos) \
    X(Color, color) \
    X(float, flex_factor) \
    X(LayoutFunc, layout_func) \
    X(ArgsUnion, args)


#define DECL_ELEM_COMPONENT_MEMBER(type, name) type *name;
#define DECL_ELEM_COMPONENT_LOCAL_ALIAS(type, name) IGNORE_UNUSED type *name = ui_context.name + _;

#define USE(e, ...) Elem __temp = e; Elem _ = __temp; FOREACH_ELEM_COMPONENT(DECL_ELEM_COMPONENT_LOCAL_ALIAS)


typedef struct InputState {
    uint8_t key_down[256];
} InputState;

typedef struct UIContext {
    uint32_t elem_count;
    uint32_t elem_capacity;
    FOREACH_ELEM_COMPONENT(DECL_ELEM_COMPONENT_MEMBER)

    int input_index;
    InputState input[2]; // double buffer input states (to allow easy diffing)
} UIContext;


extern UIContext ui_context;

EXPORT InputState *ui_flip_input();

EXPORT void ui_frame_begin();
EXPORT void ui_frame_end(float root_width, float root_height);

EXPORT Elem elem_create(Elem parent);

EXPORT Elem padding_create(Elem parent, float left, float top, float right, float bottom);

EXPORT Elem center_create(Elem parent);
EXPORT Elem align_create(Elem parent, float x, float y);

EXPORT Elem sized_box_create(Elem parent, float w, float h);

EXPORT Elem row_create(Elem parent);
EXPORT Elem column_create(Elem parent);
EXPORT Elem flex_create(Elem parent, Axis main_axis);
EXPORT Elem spacer_create(Elem parent, float flex_factor);

EXPORT Elem layout_builder_create(Elem parent, LayoutBuildFunc build_func, void *userdata);


EXPORT void test_ui(float root_width, float root_height);
