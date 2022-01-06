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

static unsigned int shaderBack, shaderCircle, shaderTri;
static unsigned int indices[3] = {0, 1, 2};
static unsigned int quadVAO, triVAO;

void triangle_bind(Tri2D* tri)
{
    glee_buffer_create_indexed(triVAO, tri, sizeof(Tri2D), &indices[0], sizeof(indices));
    glee_buffer_attribute_set(0, 2, 0, 0);
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

        if (k < len) {
            if (c.x * c.x + c.y * c.y - k <= cir->radius * cir->radius) {
                *n = vec2_norm(vec2_cross(b, a));
                return 1;
            }
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

int main(int argc, char** argv)
{

    int count = 1;

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
            if (argc > 2) count = atoi(argv[2]);
        }
        else count = atoi(argv[1]);
    }

    srand(time(NULL));
    rand_seed(rand());

    unsigned int circleCount, triangleCount;
    circleCount = triangleCount = count;
    array_t* circles = array_new(circleCount, sizeof(Circle));
    array_t* cirColor = array_new(circleCount, sizeof(vec4));
    array_t* cirVels = array_new(circleCount, sizeof(vec2));
    array_t* triangles = array_new(triangleCount, sizeof(Tri2D));
    array_t* triColor = array_new(triangleCount, sizeof(vec4));
    
    Tri2D tri;
    tri.a = world_to_screen(vec2_new(1.0, -1.0));
    tri.b = world_to_screen(vec2_new(0.0, -0.8));
    tri.c = world_to_screen(vec2_new(-1.0, -1.0));
    array_push(triangles, &tri);

    Circle cir;
    cir.pos = world_to_screen(vec2_new(randf_signed(), randf_norm()));
    cir.radius = (randf_norm() * 0.1 + 0.01) * resolution.y;
    array_push(circles, &cir);

    for (int i = 0; i < circles->used; i++) {
        vec4 c, v = {0.0, 0.0};
        c.x = randf_norm();
        c.y = randf_norm();
        c.z = randf_norm();
        c.w = 1.0;
        array_push(cirColor, &c);
        array_push(cirVels, &v);
    }

    for (int i = 0; i < triangles->used; i++) {
        vec4 c;
        c.x = randf_norm();
        c.y = randf_norm();
        c.z = randf_norm();
        c.w = 1.0;
        array_push(triColor, &c);
    }

    printf("Circles: %d of %d\n", circles->used, circles->size);
    printf("Triangles: %d of %d\n", triangles->used, triangles->size);

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
    glee_shader_uniform_set(shaderCircle, 3, "u_pos", &cir);

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
        if (glee_key_pressed(GLFW_KEY_R)) {
            circle->pos = world_to_screen(vec2_new(randf_signed(), randf_norm()));
            *vel = vec2_uni(0.0);
        }
        glee_mouse_pos(&mouse.x, &mouse.y);
        if (glee_mouse_down(GLFW_MOUSE_BUTTON_LEFT)) {
            circle->pos = mouse;
            *vel = vec2_uni(0.0);
        }

        // Update
        vec2 n;
        int inters = intersect(triangle, circle, &n);
        if (!inters) {
            vel->y -= gravity * deltaTime;
        } else {

            *vel = vec2_reflect(*vel, n);

            vec4* c = cirColor->data;
            c->x = 1.0 - c->x;
            c->y = 1.0 - c->y;
            c->z = 1.0 - c->z;
        }

        circle->pos.x += vel->x * deltaTime * force;
        circle->pos.y += vel->y * deltaTime * force;

        // Draw background
        glUseProgram(shaderBack);
        glee_shader_uniform_set(shaderBack, 1, "u_time", &t);
        glee_shader_uniform_set(shaderBack, 2, "u_mouse", &mouse);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Triangles
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

        // Circles
        glUseProgram(shaderCircle);
        glBindVertexArray(quadVAO);
        for (int i = 0; i < circles->used; i++) {
            Circle* circle = (Circle*)circles->data + i;
            vec4* c = (vec4*)cirColor->data + i;

            glee_shader_uniform_set(shaderCircle, 3, "u_pos", circle);
            glee_shader_uniform_set(shaderCircle, 4, "u_color", c);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glee_screen_refresh();
    }

    glee_deinit();
    return EXIT_SUCCESS;

}
