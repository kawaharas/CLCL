/*
 *--------------------------------------------------------
 * Sample code : Snowfall.c
 *
 * by Akira Kageyama, 1998
 * modified by Nobuaki Ohno, 2005
 *
 *--------------------------------------------------------
*/

#include <cave_ogl.h>
#include <GL/glu.h>
#include <stdlib.h>
#ifdef IRIX
#include <unistd.h>
#include <strings.h>
#else
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#endif

#define NFLAKES 500
#define XMAX 20.0
#define YMAX 20.0
#define ZMAX 10.0
#define XMIN (-20.0)
#define YMIN (-5.0)
#define ZMIN (-20.0)

/* The data that will be shared between processes */
struct _snowdata
{
	float xpos;
	float ypos;
	float zpos;
	float xaxis;
	float yaxis;
	float zaxis;
	float spin;
};

void init_gl(void), draw(struct _snowdata *);
struct _snowdata *init_shmem(void);
void compute(struct _snowdata *);

static GLuint flake_indx;

int
main(int argc, char **argv)
{
	struct _snowdata *snows;
	CAVEConfigure(&argc, argv, NULL);

	snows = init_shmem();

	CAVEInit();
	CAVEInitApplication(init_gl, 0);
	CAVEDisplay((CAVECALLBACK)draw, 1, snows);

	while (!CAVEgetbutton(CAVE_ESCKEY))
	{
		compute(snows);
#ifdef IRIX
		sginap(1);
#else
		CAVEUSleep(10);
#endif
	}

	CAVEExit();

	return 0;
}

float
randmf(void)
{
	float r, rndmax;

	rndmax = (float)(RAND_MAX + 1);
	r = rand() / rndmax;

	return r;
}

struct _snowdata *
	init_shmem(void)
{
	int i;
	struct _snowdata *snows;

	snows = (_snowdata *)CAVEMalloc(NFLAKES * sizeof(struct _snowdata));
	bzero(snows, NFLAKES * sizeof(struct _snowdata));

	for (i = 0; i < NFLAKES; i++)
	{
		snows[i].xpos = XMIN + (XMAX - XMIN) * randmf();
		snows[i].ypos = YMIN + (YMAX - YMIN) * randmf();
		snows[i].zpos = ZMIN + (ZMAX - ZMIN) * randmf();
		snows[i].xaxis = randmf();
		snows[i].yaxis = randmf();
		snows[i].zaxis = randmf();
	}

	return snows;
}

void
compute(struct _snowdata *snows)
{
	int i;
	float y, angle, velocity = 0.5;
	float t = CAVEGetTime();
	static float prev_t = 0.0;

	for (i = 0; i < NFLAKES; i++)
	{
		y = snows[i].ypos - velocity * (t - prev_t);
		if (y < YMIN) y = YMAX;
		snows[i].ypos = y;
		angle = t * 50;

		angle = (int)angle % 360;

		snows[i].spin = angle;
	}
	prev_t = t;
}

void
init_gl(void)
{
	float flake_size = 0.5;
	GLuint tmp_indx;

	glDisable(GL_LIGHTING);
	glColor3f(1.0, 1.0, 1.0);
	glClearColor(0., 0., 0., 0.);

	tmp_indx = glGenLists(1);
	glNewList(tmp_indx, GL_COMPILE);
	glBegin(GL_POLYGON);
	glVertex3f(flake_size / 2, 0.0, 0.0);
	glVertex3f(0.0, flake_size * sqrt(3.0) / 2.0, 0.0);
	glVertex3f(-flake_size / 2, 0.0, 0.0);
	glEnd();
	glEndList();

	flake_indx = glGenLists(1);
	glNewList(flake_indx, GL_COMPILE);
	glPushMatrix();
	glCallList(tmp_indx);
	glPushMatrix();
	glRotatef(180.0, 1.0, 0.0, 0.0);
	glTranslatef(0.0, -flake_size / sqrt(3.0), 0.0);
	glCallList(tmp_indx);
	glPopMatrix();
	glEndList();
}

void
draw(struct _snowdata *snows)
{
	int i;

#ifndef USE_CLCL_OCULUS_SDK
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
#endif

	for (i = 0; i < NFLAKES; i++)
	{
		glPushMatrix();
		glTranslatef(snows[i].xpos, snows[i].ypos, snows[i].zpos);
		glRotatef(snows[i].spin, snows[i].xaxis, snows[i].yaxis, snows[i].zaxis);
		glCallList(flake_indx);
		glPopMatrix();
	}
}
