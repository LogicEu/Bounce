#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <photon.h>
#include <glee.h>
#include <utopia.h>

static float gravity = 1.1;
static float force = 10.0;

static int fullscreen = 0;
static vec2 resolution = {800.0, 600.0};
static vec2 mouse;

static array_t *circles, *cirColor, *cirVels;
static array_t *triangles, *triColor;

static unsigned int shaderBack, shaderCircle, shaderTri;
static unsigned int indices[3] = {0, 1, 2};
static unsigned int quadVAO, triVAO;

void triangle_bind(Tri2D* tri)
{
    glee_buffer_create_indexed(triVAO, tri, sizeof(Tri2D), &indices[0], sizeof(indices));
    glee_buffer_attribute_set(0, 2, 0, 0);
}

vec2 vec2_lerp(vec2 v1, vec2 v2)
{
    vec2 v = {lerpf(v1.x, v2.x, 0.5), lerpf(v1.y, v2.y, 0.5)};
    return v;
}

vec2 world_to_screen(vec2 p)
{
    vec2 v = {
        (p.x + 1.0) * 0.5 * resolution.x,
        (p.y + 1.0) * 0.5 * resolution.y
    };
    return v;
}

vec2 screen_to_world(vec2 p)
{
    vec2 v = {
        2.0 * p.x / resolution.x - 1.0,
        2.0 * p.y / resolution.y - 1.0
    };
    return v;
}

int intersect_edge(Circle* cir, vec2 a, vec2 b, vec2* n)
{
    vec2 c = _vec2_sub(cir->pos, a);
    vec2 e = _vec2_sub(b, a);

    float k = c.x * e.x + c.y * e.y;
    if (k > 0.0) {
        float len = e.x * e.x + e.y * e.y;
        k = k * k / len;

        if (k < len && c.x * c.x + c.y * c.y - k <= cir->radius * cir->radius) {
            *n = vec2_normal(vec2_cross(b, a));
            return 1;
        }
    }
    return 0;
}

int intersect(Tri2D* tri, Circle* cir, vec2* n)
{
    return (intersect_edge(cir, tri->a, tri->b, n) ||
            intersect_edge(cir, tri->b, tri->c, n) ||
            intersect_edge(cir, tri->c, tri->a, n));
}

void vertex_add()
{
    static unsigned int verts = 0;
    static vec2 tri[3];

    tri[(++verts) - 1] = mouse;
    if (verts > 2) {
        vec4 c = {randf_norm(), randf_norm(), randf_norm(), 1.0};
        array_push(triColor, &c);
        array_push(triangles, &tri[0]);
        verts = 0;
    }
}

void circle_add()
{
    Circle cr = circle_new(mouse, 40.0 * randf_norm() + 10.0);
    vec4 c = {randf_norm(), randf_norm(), randf_norm(), 1.0};
    vec2 v = {0.0, 0.0};
    array_push(circles, &cr);
    array_push(cirColor, &c);
    array_push(cirVels, &v);
}

void draw_background(float t)
{
    glBindVertexArray(quadVAO);
    glUseProgram(shaderBack);
    glee_shader_uniform_set(shaderBack, 1, "u_time", &t);
    glee_shader_uniform_set(shaderBack, 2, "u_mouse", &mouse);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void update_triangles()
{
    glUseProgram(shaderTri);
    glBindVertexArray(triVAO);
    for (int i = 0; i < triangles->used; i++) {
        Tri2D* triangle = (Tri2D*)triangles->data + i;
        vec4* c = (vec4*)triColor->data + i;
        
        Tri2D t1 = {
            screen_to_world(triangle->a),
            screen_to_world(triangle->b),
            screen_to_world(triangle->c),
        };

        triangle_bind(&t1);
        glee_shader_uniform_set(shaderTri, 4, "u_color", c);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    }
}

void update_circles(float deltaTime)
{
    glUseProgram(shaderCircle);
    glBindVertexArray(quadVAO);
    const int len = circles->used, tlen = triangles->used;
    for (int i = 0; i < len; i++) {
        Circle* circle = (Circle*)circles->data + i;
        vec4* c = (vec4*)cirColor->data + i;
        vec2* vel = (vec2*)cirVels->data + i;

        // Circle collision
        for (int j = 0; j < len; j++) {
            if (i == j) continue;
            
            Circle* c2 = (Circle*)circles->data + j;
            vec2* vel2 = (vec2*)cirVels->data + j;
            vec2 g = vec2_norm(vec2_sub(c2->pos, circle->pos));

            //vec2 vdif = vec2_sub(vec2_mult(*vel, deltaTime * force), vec2_mult(*vel2, deltaTime * force));

            if (circle_overlap_offset(*circle, *c2, *vel)) {
                //inters++;
                float M = circle->radius * c2->radius;
                vec2 v1 = *vel, v2 = *vel2;

                vel->x = ((circle->radius - c2->radius) / M) * v1.x + ((2.0 * c2->radius) / M) * v2.x;
                vel->y = ((circle->radius - c2->radius) / M) * v1.y + ((2.0 * c2->radius) / M) * v2.y;
                float mag1 = vec2_mag(*vel);
                vec2 dir1 = vec2_mult(g, -mag1);

                vel2->x = ((c2->radius - circle->radius) / M) * v2.x + ((2.0 * circle->radius) / M) * v1.x;
                vel2->y = ((c2->radius - circle->radius) / M) * v2.y + ((2.0 * circle->radius) / M) * v1.y;
                float mag2 = vec2_mag(*vel2);
                vec2 dir2 = vec2_mult(g, mag2);

                //*vel = vec2_mult(vec2_norm(vec2_lerp(dir1, *vel)), mag1);
                //*vel2 = vec2_mult(vec2_norm(vec2_lerp(dir2, *vel2)), mag2);
            }

            while(circle_overlap(*circle, *c2)) {
                vel->x -= g.x;
                vel->y -= g.y;
                circle->pos.x -= g.x;
                circle->pos.y -= g.y;
                c2->pos.x += g.x;
                c2->pos.y += g.y;
            }
        }

        // Triangle collision
        vec2 n;
        int inters = 0;
        for (int j = 0; j < tlen; j++) {
            Tri2D* T = (Tri2D*)triangles->data + j;
            while (intersect(T, circle, &n)) {
                circle->pos.x += n.x;
                circle->pos.y += n.y;
                inters++;
            }
            if (inters) {
                *vel = vec2_reflect(*vel, n);
                break;
            }
        }
        
        if (!inters) vel->y -= gravity * deltaTime;

        circle->pos.x += vel->x * deltaTime * force;
        circle->pos.y += vel->y * deltaTime * force;

        glee_shader_uniform_set(shaderCircle, 3, "u_pos", circle);
        glee_shader_uniform_set(shaderCircle, 4, "u_color", c);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}

void scene_deinit()
{
    array_destroy(circles);
    array_destroy(cirColor);
    array_destroy(cirVels);
    array_destroy(triangles);
    array_destroy(triColor);
}

void scene_init(unsigned int size)
{
    circles = array_new(size, sizeof(Circle));
    cirColor = array_new(size, sizeof(vec4));
    cirVels = array_new(size, sizeof(vec2));
    triangles = array_new(size, sizeof(Tri2D));
    triColor = array_new(size, sizeof(vec4));
}

void scene_reinit()
{
    array_free(circles);
    array_free(cirColor);
    array_free(cirVels);
    array_free(triangles);
    array_free(triColor);
}

int main(int argc, char** argv)
{
    if (argc > 1) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help")) {
            printf("---------------- BOUNCE -----------------\n");
            printf("Use -f or -fullscreen for total enjoyment\n");
            printf("\t\tversion 0.1.0\n");
            return EXIT_SUCCESS;
        } 
        else if (!strcmp(argv[1], "-f") || !strcmp(argv[1], "-fullscreen")) {
            resolution.x = 1440.0;
            resolution.y = 960.0;
            fullscreen++;
        }
    }

    srand(time(NULL));
    rand_seed(rand());

    scene_init(16);

    //Glee Init
    glee_init();
    glee_window_create("Bounce", resolution.x, resolution.y, fullscreen, 0);
    glee_screen_color(0.5, 0.5, 1.0, 1.0);

    triVAO = glee_buffer_id();
    quadVAO = glee_buffer_quad_create();

    shaderBack = glee_shader_load("shaders/vert.glsl", "shaders/frag.glsl");
    glee_shader_uniform_set(shaderBack, 2, "u_resolution", &resolution);

    shaderCircle = glee_shader_load("shaders/vert.glsl", "shaders/circle.glsl");
    glee_shader_uniform_set(shaderCircle, 2, "u_resolution", &resolution);

    shaderTri = glee_shader_load("shaders/vert.glsl", "shaders/triangle.glsl");
    glee_shader_uniform_set(shaderTri, 2, "u_resolution", &resolution);

    float t = glee_time_get(), deltaTime;

    while(glee_window_is_open()) {
        glee_screen_clear();
        deltaTime = glee_time_delta(&t) * 10.0;

        Circle* circle = circles->data;
        vec2* vel = cirVels->data;
        Tri2D* triangle = triangles->data;

        if (glee_key_pressed(GLFW_KEY_ESCAPE)) break;
        if (glee_key_pressed(GLFW_KEY_R)) scene_reinit();
        glee_mouse_pos(&mouse.x, &mouse.y);
        
        if (glee_key_pressed(GLFW_KEY_B)) circle_add();
        if (glee_key_pressed(GLFW_KEY_V)) vertex_add();

        draw_background(t);
        update_circles(deltaTime);
        update_triangles();

        glee_screen_refresh();
    }

    scene_deinit();
    glee_deinit();
    return EXIT_SUCCESS;
}
