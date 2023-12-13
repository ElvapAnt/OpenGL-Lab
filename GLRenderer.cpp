#include "pch.h"
#include "GLRenderer.h"
#include "GL\gl.h"
#include "GL\glu.h"
#include "GL\glaux.h"
#include "GL\glut.h"
#define _USE_MATH_DEFINES
#include <cmath>
//#pragma comment(lib, "GL\\glut32.lib")

CGLRenderer::CGLRenderer(void)
{
}

CGLRenderer::~CGLRenderer(void)
{
}

bool CGLRenderer::CreateGLContext(CDC* pDC)
{
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int nPixelFormat = ChoosePixelFormat(pDC->m_hDC, &pfd);

	if (nPixelFormat == 0) return false;

	BOOL bResult = SetPixelFormat(pDC->m_hDC, nPixelFormat, &pfd);

	if (!bResult) return false;

	m_hrc = wglCreateContext(pDC->m_hDC);

	if (!m_hrc) return false;

	return true;
}

void CGLRenderer::PrepareScene(CDC* pDC)
{
	wglMakeCurrent(pDC->m_hDC, m_hrc);
	//---------------------------------
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.32f, 0.75f, 0.90f, 1.0f);
	//---------------------------------
	wglMakeCurrent(NULL, NULL);
}

void CGLRenderer::DrawScene(CDC* pDC, double angle)
{
	wglMakeCurrent(pDC->m_hDC, m_hrc);
	//---------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float distance = 8; // Distance from the origin

	float azimuthRad = m_pitch * (M_PI / 180.0f);
	float elevationRad = m_yaw * (M_PI / 180.0f);

	// Calculate camera position using spherical coordinates
	float x = distance * cos(elevationRad) * sin(azimuthRad);
	float y = distance * sin(elevationRad);
	float z = distance * cos(elevationRad) * cos(azimuthRad);


	glLoadIdentity();
	gluLookAt(x, y, z,
		0.0, 2, 0.0,
		0.0, 1.0, 0.0);

	//glRotatef(m_pitch, 1.0f, 0.0f, 0.0f); // X-axis rotation (pitch)
	//glRotatef(m_yaw, 0.0f, 1.0f, 0.0f); // Y-axis rotation (yaw)

	DrawFigure(angle);
	DrawAxis(30.0);
	glColor3f(0.9, 0.9, 0.9);
	DrawGrid(7.0, 7.0, 10, 10);
	glFlush();
	//---------------------------------
	SwapBuffers(pDC->m_hDC);
	wglMakeCurrent(NULL, NULL);
}

void CGLRenderer::Reshape(CDC* pDC, int w, int h)
{
	wglMakeCurrent(pDC->m_hDC, m_hrc);
	//---------------------------------
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)w / (double)h, 1.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
	//---------------------------------
	wglMakeCurrent(NULL, NULL);
}

void CGLRenderer::DestroyScene(CDC* pDC)
{
	wglMakeCurrent(pDC->m_hDC, m_hrc);
	// ... 
	wglMakeCurrent(NULL, NULL);
	if (m_hrc)
	{
		wglDeleteContext(m_hrc);
		m_hrc = NULL;
	}
}

void CGLRenderer::DrawSphere(double r, int nSegAlpha, int nSegBeta)
{
	for (int i = 0; i <= nSegAlpha; i++) {
		double lat0 = M_PI * (-0.5 + (double)(i - 1) / nSegAlpha);
		double z0 = sin(lat0);
		double zr0 = cos(lat0);

		double lat1 = M_PI * (-0.5 + (double)i / nSegAlpha);
		double z1 = sin(lat1);
		double zr1 = cos(lat1);

		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= nSegBeta; j++) {
			double lng = 2 * M_PI * (double)j / nSegBeta;
			double x = cos(lng);
			double y = sin(lng);

			glNormal3d(x * zr0, y * zr0, z0);
			glVertex3d(r * x * zr0, r * y * zr0, r * z0);
			glNormal3d(x * zr1, y * zr1, z1);
			glVertex3d(r * x * zr1, r * y * zr1, r * z1);
		}
		glEnd();
	}
}

void CGLRenderer::DrawCylinder(double h, double r1, double r2, int nSeg)
{
	double step = 2 * M_PI / nSeg;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= nSeg; i++) {
		double angle = i * step;
		double x = cos(angle);
		double z = sin(angle);

		glNormal3d(x, 0.0, z);
		glVertex3d(r2 * x, h, r2 * z);
		glVertex3d(r1 * x, 0.0, r1 * z);
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0.0, h, 0.0); 
	for (int i = 0; i <= nSeg; i++) {
		double angle = i * step;
		glVertex3d(r2 * cos(angle), h, r2 * sin(angle));
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0.0, 0.0, 0.0); 
	for (int i = 0; i <= nSeg; i++) {
		double angle = i * step;
		glVertex3d(r1 * cos(angle), 0.0, r1 * sin(angle));
	}
	glEnd();
}

void CGLRenderer::DrawCone(double h, double r, int nSeg)
{
	double step = 2 * M_PI / nSeg;
	double nx, ny, nz; 

	double slope = sqrt(r * r + h * h); 
	nx = r / slope;
	nz = r / slope;
	ny = h / slope;


	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0.0, h, 0.0); 
	for (int i = 0; i <= nSeg; i++) {
		double angle = i * step;
		double x = cos(angle);
		double z = sin(angle);

		glNormal3d(nx * x, ny, nz * z);
		glVertex3d(r * x, 0.0, r * z); 
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glVertex3d(0.0, 0.0, 0.0); 
	for (int i = 0; i <= nSeg; i++) {
		double angle = i * step;
		glVertex3d(r * cos(angle), 0.0, r * sin(angle));
	}
	glEnd();
}

void CGLRenderer::DrawAxis(double width)
{
	glBegin(GL_LINES);
	glColor3d(1.0, 0.0, 0.0);
	glVertex3d(0, 0, 0);
	glVertex3d(width, 0, 0);

	glColor3d(0.0, 1.0, 0.0);
	glVertex3d(0, 0, 0);
	glVertex3d(0, width, 0);
	
	glColor3d(0.0, 0.0, 1.0);
	glVertex3d(0, 0, 0);
	glVertex3d(0, 0, width);
	glEnd();
}
void CGLRenderer::DrawGrid(double width, double height, int nSegW, int nSegH)
{
	double halfWidth = width / 2.0;
	double halfHeight = height / 2.0;
	double xStep = width / nSegW;
	double zStep = height / nSegH;

	glBegin(GL_LINES);

	for (int i = 0; i <= nSegW; ++i) {
		double x = -halfWidth + i * xStep;
		glVertex3f(x, 0.0f, -halfHeight);
		glVertex3f(x, 0.0f, halfHeight);
	}

	for (int j = 0; j <= nSegH; ++j) {
		double z = -halfHeight + j * zStep;
		glVertex3f(-halfWidth, 0.0f, z);
		glVertex3f(halfWidth, 0.0f, z);
	}

	glEnd();
}
void CGLRenderer::DrawFigure(double angle)
{
	double sR = 0.18;
	double cH = 0.85;
	glColor3d(1.0, 0.5, 0.0);
	DrawCylinder(0.5, 0.7, 0.74, 9);
	DrawCylinder(cH, 0.5, 0.9, 9);
	
	//pocetak
	glPushMatrix();
	glTranslatef(0.0, cH, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCylinder(cH, 0.40, 0.40, 10);
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	//-----kraj pocetka

	//leva ruka cocktusa
	glPushMatrix();
	
	glRotatef(45, 1.0, 0.0, 0.0);
	
	glTranslatef(0.0, 2*sR-0.1, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCylinder(cH, 0.25, 0.25, 10);

	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);

	glTranslatef(0.0, sR, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCone(cH, 0.25, 10);

	glTranslatef(0.0, cH+sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	glPopMatrix();
	//-----kraj leve ruke
	
	//desna ruka cocktusa
	glPushMatrix();
	glRotatef(-45, 1.0, 0.0, 0.0);
	
	glTranslatef(0.0, 2 * sR - 0.1, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCylinder(cH, 0.25, 0.25, 10);
	
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);

	glTranslatef(0.0, sR, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCone(cH, 0.25, 10);

	glTranslatef(0.0, cH+sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	
	glTranslatef(0.0, sR, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCone(cH, 0.25, 10);
	
	glTranslatef(0.0, cH+sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);

	glPopMatrix();
	//-----kraj desne ruke

	glTranslatef(0.0, 2*sR - 0.05, 0.0);
	glColor3d(0.0, 0.8, 0.0);
	DrawCylinder(cH, 0.25, 0.25, 10);
	
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);

	//gornja leva pokretljiva
	glPushMatrix();
	glRotatef(angle+45, 1.0, 0.0, 0.0);
	glTranslatef(0.0, sR, 0.0);
	glColor3d(1.0, 1.0, 0.0);
	DrawCylinder(cH, 0.25, 0.25, 10);
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	glTranslatef(0.0, sR, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCone(cH, 0.25, 10);
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	glPopMatrix();
	//-----kraj gornje leve pokretljive

	//gornja desna 
	glPushMatrix();
	glRotatef(- 45, 1.0, 0.0, 0.0);
	glTranslatef(0.0, sR, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCylinder(cH, 0.25, 0.25, 10);
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	glTranslatef(0.0, sR, 0.0);
	glColor3d(0.0, 0.75, 0.0);
	DrawCylinder(cH, 0.25, 0.25, 10);
	glTranslatef(0.0, cH + sR, 0.0);
	glColor3d(0.0, 0.55, 0.0);
	DrawSphere(sR, 30, 30);
	glPopMatrix();
	//-----kraj gornje desne

	glPopMatrix();
}


