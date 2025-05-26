#include "mikac.c"
#include "tigr.h"

#define FNL_IMPL
#include "FastNoiseLite.h"

typedef struct {
    Tigr* win;
    int width;
    int height;
    float dt;
    float scale;
    void* data;
    fnl_state noise;
} context;

typedef struct {
    float x, y;
} vect;

#define v(_x, _y) ((vect){.x = _x, .y = _y})
#define v_add(v1, v2) v(v1.x + v2.x, v1.y + v2.y)
#define v_sub(v1, v2) v(v1.x - v2.x, v1.y - v2.y)
#define v_mul(v0, m) v(v0.x * m, v0.y * m)
#define v_div(v0, m) v(v0.x / m, v0.y / m)
#define v_mod(v) (sqrt(pow(v.x, 2) + pow(v.y, 2)))

void init(str title, int w, int h, int flags, void* data,
    void (*step)(context*), void (*draw)(context*)){
    srand(clock());

    context* ctxt = alloc(context);

    ctxt->noise = fnlCreateState();
    ctxt->noise.noise_type = FNL_NOISE_OPENSIMPLEX2;

    ctxt->win = tigrWindow(w, h, title, flags);
    ctxt->width = w;
    ctxt->height = h;
    ctxt->data = data;

    
    int paused = 0;
    tigrSetPostFX(ctxt->win, 2.1f, 2.1f, 1, 4.2f);
    while(!tigrClosed(ctxt->win)){
        ctxt->dt = tigrTime();
        if(tigrKeyDown(ctxt->win, TK_ESCAPE)) break;
        if(tigrKeyDown(ctxt->win, TK_RETURN)) paused = !paused;
        if(!paused){
            step(ctxt);
        };
        draw(ctxt);
        tigrUpdate(ctxt->win);
    };

    tigrFree(ctxt->win);
    free(ctxt);
};


void clear(context* ctxt, TPixel col){
    tigrClear(ctxt->win, col);
};

void draw_l(context* ctxt, vect pI, vect pF, TPixel col){
    tigrLine(ctxt->win, pI.x, pI.y, pF.x, pF.y, col);
};

TPixel lrgb(long lit){
    return tigrRGB(lit >> 16, lit >> 8 & 0xff, lit & 0xff);
};

vect v_add_c(vect a, vect b, float max){
    if(fabs(a.x) + fabs(b.x) <= max && fabs(a.y) + fabs(b.y) <= max){
        if(fabs(a.x) + fabs(b.x) <= max){
            a.x += b.x;
        } else {
            const int sign = (b.x > 0) - (b.x < 0);
            a.x = max * sign;
        };
        if(fabs(a.y) + fabs(b.y) <= max){
            a.y += b.y;
        } else {
            const int sign = (b.y > 0) - (b.y < 0);
            a.y = max * sign;
        };
    };
    return a;
};

vect v_sub_z(vect a, vect b){
    if(fabs(a.x) - fabs(b.x) >= 0){
        a.x -= b.x;
    } else {
        a.x = 0;
    };
    if(fabs(a.y) - fabs(b.y) >= 0){
        a.y -= b.y;
    } else {
        a.y = 0;
    };
    return a;
};
