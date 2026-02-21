#include <stdio.h>

#include <GL/glut.h>

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);

    glColor3f(1.f, 0.f, 0.f);
    glVertex2f(-.5f, -.5f);

    glColor3f(0.f, 1.f, 0.f);
    glVertex2f(.5f, -.5f);

    glColor3f(0.f, 0.f, 1.f);
    glVertex2f(0.f, .5f);

    glEnd();

    glFlush();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    {
        const float left = -1.f;
        const float right = 1.f;
        const float bottom = -1.f;
        const float top = 1.f;
        const float near_val = -1.f;
        const float far_val = 1.f;
        glOrtho(left, right, bottom, top, near_val, far_val);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Tanks");

    glewExperimental = true;
    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "glewInit: %s\n", glewGetErrorString(err));
        return -1;
    }

    printf("OpenGL %s\n", glGetString(GL_VERSION));

    glClearColor(0.f, 0.f, 0.f, 0.f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();
}
