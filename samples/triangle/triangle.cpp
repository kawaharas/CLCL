/*
 *--------------------------------------------------------
 * Sample code : triangle.c
 *
 * by Nobuaki Ohno, 2005
 *
 *--------------------------------------------------------
*/

#include <cave_ogl.h>
#ifdef IRIX
#include <unistd.h>
#endif

void
init_gl(void)
{
	glClearColor(0., 0., 0., 0.);
}

void
draw(void)
{
#ifndef USE_CLCL_OCULUS_SDK
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
#endif

	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glTranslatef(0.0, 5.0, -5.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(-2.0, 0.0, 0.0);
	glVertex3f(2.0, 0.0, 0.0);
	glVertex3f(0.0, 4.0, 0.0);
	glEnd();
	glPopMatrix();
}

int
main(int argc, char **argv)
{
	CAVEConfigure(&argc, argv, NULL);

	CAVEInit();
	CAVEInitApplication(init_gl, 0);
	CAVEDisplay(draw, 0);

	while (!CAVEgetbutton(CAVE_ESCKEY))
	{
#ifdef IRIX
		sginap(10);
#else
		CAVEUSleep(10);
#endif
	}

	CAVEExit();

	return 0;
}
