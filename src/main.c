#include "engine.c"

typedef struct {
    TPixel color;
    vect pos;
    vect speed;
    float size;
    float velocity;
    float tick;
    int active;
} bolt;

typedef struct {
    vect pos;
    vect speed;
    float size;
    float decay;
    float tick;
} bubble;

typedef struct {
    TPixel color;
    vect pos;
    vect push;
    vect speed;
    float max_speed;
    float side;
    float rotation;
    float r_speed;

    float tick;
    float bubble_tick;

    int shooting;
    int cur_shot;
    bolt shot[8];

    int flying;
    int cur_bubble;
    bubble trail[8];
} ship;

typedef struct {
    int flying;
    int shooting;
    float direction;
    ship self;
} npc_ship;

dynamic_array(vect);
dynamic_array(npc_ship);

typedef struct {
    npc_ship_arr_t npcs;
} map_t;

typedef struct {
    ship main;
    map_t* map;
    float delta;
    int width;
    int height;
    vect view_point;
} state;

void ship_step(context*, ship*, int, int, int, vect*, int);
void ship_draw(context*, ship, int);

void step(context* ctxt){
    state* stt = ctxt->data;
    stt->delta = tigrTime();

    ship* main = &stt->main;
    ship_step(ctxt, main,
        tigrKeyHeld(ctxt->win, 'A') - tigrKeyHeld(ctxt->win, 'D'),
        tigrKeyHeld(ctxt->win, 'W'),
        tigrKeyHeld(ctxt->win, TK_SPACE), 
        &stt->view_point, -1);

    for(int i = 0; i < stt->map->npcs.size; i += 1){
        npc_ship* npc = &stt->map->npcs.items[i];
        const float dist = sqrt(pow((npc->self.pos.x + stt->view_point.x) - main->pos.x, 2) +
            pow((npc->self.pos.y + stt->view_point.y) - main->pos.y, 2));

        npc->flying = dist > npc->self.side + 4;
        npc->shooting = dist < npc->self.side * 16 && npc->flying;
        if(npc->flying){
            const vect vp = v_add(npc->self.pos, stt->view_point);
            const vect rv = v_sub(main->pos, vp);
            const float a = atan2(rv.x, rv.y) - 2.0944f;
            npc->direction = (a - npc->self.rotation > 0) - (a - npc->self.rotation < 0);
        };
        ship_step(ctxt, &npc->self, npc->direction,
            npc->flying, npc->shooting, &npc->self.pos, 1);
    };
};

void ship_step(context* ctxt, ship* s, int rotate, int fly, int shoot, vect* pos, int sign){
    state* stt = ctxt->data;
    if(fly && v_mod(s->speed) > s->max_speed * 0.8f){
        s->rotation += rotate * s->r_speed;
    };

    const vect point = v(sinf(s->rotation + 2.0944f), cosf(s->rotation + 2.0944f));
    const vect dir = v_mul(point, s->side);

    s->tick += stt->delta;
    s->bubble_tick += stt->delta;

    if(shoot && s->tick >= 0.4f){
        const int c = s->cur_shot;
        s->shot[c].active = 1;
        s->shot[c].pos = v_add(dir, s->push);
        const vect base_velocity = v_mul(s->shot[c].pos, s->shot[c].velocity);
        const vect boost = v_mul(s->speed, 0.01f);
        s->shot[c].speed = v_add(base_velocity, boost);
        s->shot[c].pos = v_add(s->shot[c].pos, s->pos);

        s->cur_shot += 1;
        s->cur_shot %= 8;
        s->shooting = 1;
    };

    if(fly){
        const float push_mod = v_mod(s->push);
        const float speed_mod = v_mod(s->speed);
        if(rotate){
            s->push = v_mul(point, push_mod);
            s->speed = v_mul(point, speed_mod);

            for(int i = 0; i < 8; i += 1){
                if(s->trail[i].size > 0){
                    s->trail[i].speed = v_mul(point, -speed_mod * 0.25f);
                };
            };
        };
        s->push = v_add_c(s->push, point, 6);
        s->speed = v_add_c(s->speed, point, s->max_speed);

        if(!s->flying) s->bubble_tick = 0.3f;
        s->flying = 1;

        if(s->bubble_tick >= 0.3f){
            const int c = s->cur_bubble;
            
            const vect dif = v_mul(dir, 0.8f);
            const vect ipos = v_add(s->pos, s->push);
            s->trail[c].pos = v_sub(ipos, dif);

            const float trail_speed = speed_mod > s->side ? -speed_mod : -s->side;

            s->trail[c].speed = v_mul(dif, trail_speed * 0.01f);
            s->trail[c].size = s->side / 4;

            s->cur_bubble += 1;
            s->cur_bubble %= 8;
        };
    } else if(s->push.x != 0 || s->push.y != 0){
        s->flying = 0;
        const vect new = v_sub_z(s->push, v_mul(s->push, 0.08f));
        if(fabs(s->push.x) < fabs(new.x) ||
            fabs(s->push.y) < fabs(new.y)) s->push = v(0, 0);
        else s->push = new;

        s->speed = v_sub_z(s->speed, v_mul(s->speed, 0.08f));
    };

    for(int i = 0; i < 8; i += 1){
        if(s->shot[i].active){
            s->shot[i].pos.x += s->shot[i].speed.x;
            s->shot[i].pos.y += s->shot[i].speed.y;

            s->shot[i].tick += stt->delta;
            if(s->shot[i].tick >= 0.8f){
                s->shot[i].active = 0;
                s->shot[i].tick = 0;
            };
        };
    };

    for(int i = 0; i < 8; i += 1){
        if(s->trail[i].size > 0){
            s->trail[i].pos.x += s->trail[i].speed.x;
            s->trail[i].pos.y += s->trail[i].speed.y;
            s->trail[i].tick += stt->delta;

            if(!s->flying && fabs(v_mod(s->speed)) <
                fabs(v_mod(s->trail[i].speed))){
                s->trail[i].speed = v_sub_z(s->trail[i].speed,
                    v_mul(s->trail[i].speed, .15f));
            };
            if(s->trail[i].tick >= 0.35f){
                s->trail[i].size -= s->trail[i].decay;
                if(s->trail[i].size < 0) s->trail[i].size = 0;
                s->trail[i].tick = 0;
            };
        };
    };
    if(s->tick >= 0.4f) s->tick = 0;
    if(s->bubble_tick >= 0.3f) s->bubble_tick = 0;

    if(s->speed.x != 0 || s->speed.y != 0){
        vect wrap = *pos;
        if(sign > 0){
            *pos = v_add(wrap, s->speed);
        } else {
            *pos = v_sub(wrap, s->speed);
        };
    };
};

const TPixel BG_COL = { 0x0f, 0x0f, 0x1f, 0xff };
const TPixel LOWCOL = { 0x3f, 0x3f, 0x4f, 0xff };
const TPixel LINCOL = { 0x9f, 0x9f, 0xaf, 0xff };
const TPixel MWHITE = { 0xf9, 0xf9, 0xf9, 0xff };
void draw(context* ctxt){
    clear(ctxt, BG_COL);
    
    const state* stt = ctxt->data;
    const ship main = stt->main;

    for(float x = 0; x < ctxt->width; x += 3.3f){
        for(float y = 0; y < ctxt->height; y += 3.3f){
            const float seed = fnlGetNoise2D(&ctxt->noise, (x - stt->view_point.x) * 1.2f, (y - stt->view_point.y) * 1.2f);
            if(!(seed > 0.9f)) continue;

            const vect star = v(x, y);
            float dist = sqrt(pow((star.x) - main.pos.x, 2) + pow((star.y) - main.pos.y, 2));

            int ok = dist > main.side * 1.5f;
            for(int i = 0; i < stt->map->npcs.size; i += 1){
                ship npc = stt->map->npcs.items[i].self;
                float dist = sqrt(pow((star.x) - (npc.pos.x + stt->view_point.x), 2) +
                    pow((star.y) - (npc.pos.y + stt->view_point.y), 2));
                if(!(dist > npc.side * 1.5f)) ok = 0;
            };
            if(ok){
                tigrFillCircle(ctxt->win, star.x, star.y, 2, MWHITE);
            };
        };
    };
    ship_draw(ctxt, main, 1);

    for(int i = 0; i < stt->map->npcs.size; i += 1){
        ship_draw(ctxt, stt->map->npcs.items[i].self, 0);
    };
};

void ship_draw(context* ctxt, ship s, int main){
    const state* stt = ctxt->data;
    vect pos = v_add(s.pos, s.push);
    if(!main) pos = v_add(s.pos, stt->view_point);

    const vect vertex[3] = {
        v(sinf(s.rotation), cosf(s.rotation)),
        v(sinf(s.rotation + 2.0944f), cosf(s.rotation + 2.0944f)),
        v(sinf(s.rotation + 4.1888f), cosf(s.rotation + 4.1888f)),
    };    
    const int max = 3;
    for(int v = 0; v < max; v += 1){
        const vect line_0 = v_mul(vertex[v], s.side);
        const vect line_1 = v_mul(vertex[(v + 1) % max], s.side);
        draw_l(ctxt, v_add(line_0, pos), v_add(line_1, pos), s.color);

        const vect outline_0 = v_mul(line_0, 1.3f);
        const vect outline_1 = v_mul(line_1, 1.3f);
        draw_l(ctxt, v_add(outline_0, pos),
            v_add(outline_1, pos), s.color);

        if(!v){
            const vect line_2 = v_mul(vertex[2], s.side);
            const int x_dif_l = line_1.x - line_0.x;
            const int y_dif_l = line_1.y - line_0.y;
            const int x_dif_r = line_1.x - line_2.x;
            const int y_dif_r = line_1.y - line_2.y;
            const int x_dif_m = line_2.x - line_0.x;
            const int y_dif_m = line_2.y - line_0.y;

            const vect anchor = v(line_0.x + x_dif_m / 2, line_0.y + y_dif_m / 2);

            for(float p = 0.1f; p < 0.5f; p += 0.1f){
                const vect l_trace_0 = v(line_2.x + x_dif_r * p, line_2.y + y_dif_r * p);
                const vect l_trace_1 = v(anchor.x + x_dif_l * p, anchor.y + y_dif_l * p);
                draw_l(ctxt, v_add(l_trace_0, pos), v_add(l_trace_1, pos), s.color);

                const vect r_trace_0 = v(anchor.x + x_dif_r * p, anchor.y + y_dif_r * p);
                const vect r_trace_1 = v(line_0.x + x_dif_l * p, line_0.y + y_dif_l * p);
                draw_l(ctxt, v_add(r_trace_0, pos), v_add(r_trace_1, pos), s.color);
            };

            const vect midle_0 = v(line_0.x + x_dif_l / 2, line_0.y + y_dif_l / 2);
            const vect midle_1 = v(line_2.x + x_dif_r / 2, line_2.y + y_dif_r / 2);

            draw_l(ctxt, v_add(midle_0, pos), v_add(anchor, pos), s.color);
            draw_l(ctxt, v_add(midle_1, pos), v_add(anchor, pos), s.color);
        };
    };
    for(int i = 0; i < 8; i += 1){
        if(s.shot[i].active){
            const vect spos = !main ? v_add(s.shot[i].pos, stt->view_point) : s.shot[i].pos;
            tigrCircle(ctxt->win, spos.x, spos.y, s.shot[i].size, s.shot[i].color);
        };
    };

    for(int i = 0; i < 8; i += 1){
        if(s.trail[i].size > 0){
            const vect tpos = !main ? v_add(s.trail[i].pos, stt->view_point) : s.trail[i].pos;
            tigrCircle(ctxt->win, tpos.x, tpos.y, s.trail[i].size, LINCOL);
        };
    };
};

void ship_init(ship* s, TPixel color, TPixel bolt_color, vect pos, float side, float max_speed, float r_speed){
    s->color = color;
    s->tick = 0;
    s->pos = pos;
    s->side = side;
    s->max_speed = max_speed;
    s->rotation = 0;
    s->r_speed = r_speed;

    s->cur_shot = 0;
    s->cur_bubble = 0;
    for(int i = 0; i < 8; i += 1){
        s->trail[i].size = 0;
        s->trail[i].decay = side / 10;

        s->shot[i].color = bolt_color;
        s->shot[i].size = side / 6;
        s->shot[i].active = 0;
        s->shot[i].velocity = side / 80;
    };
};

int main(){
    state* data = alloc(state);
    data->width = 760;
    data->height = 760;

    ship_init(&data->main, LINCOL, MWHITE, v(data->width / 2, data->height / 2), 20, 8, 0.02f);

    data->map = alloc(map_t);
    npc_ship_arr_init(&data->map->npcs, 128);
    const int max = rand() % 20 + 10;
    for(int i = 0; i < max; i += 1){
        ship s;
        ship_init(&s, lrgb(0xff297f), lrgb(0xff4f5f), v(rand() % (data->width * 2), rand() % (data->height * 2)), 20, 3, 0.02f);

        npc_ship npc;
        npc.direction = 0;
        npc.shooting = 0;

        npc.self = s;
        npc_ship_arr_append(&data->map->npcs, npc);
    };
    data->view_point = v(data->width / 3, data->height / 3);

    init("cosmic ocean", data->width, data->height,
        TIGR_AUTO | TIGR_NOCURSOR, data, &step, &draw);

    free(data->map->npcs.items);
    free(data->map);
    free(data);

    return 0;
};
