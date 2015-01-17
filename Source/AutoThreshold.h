#ifndef AutoThreshold_H
#define AutoThreshold_H

#include <FFGLShader.h>
#include "../FFGLPluginSDK.h"
#include <math.h>
// #include "histogram_ext.h"

class AutoThreshold :
public CFreeFrameGLPlugin
{
public:
	AutoThreshold();
  virtual ~AutoThreshold() {}

	///////////////////////////////////////////////////
	// FreeFrameGL plugin methods
	///////////////////////////////////////////////////
	
	DWORD SetParameter(const SetParameterStruct* pParam);		
	DWORD GetParameter(DWORD dwIndex);					
	DWORD ProcessOpenGL(ProcessOpenGLStruct* pGL);
	DWORD InitGL(const FFGLViewportStruct *vp);
	DWORD DeInitGL();

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)  {
  	*ppOutInstance = new AutoThreshold();
	if (*ppOutInstance != NULL)
		return FF_SUCCESS;

	return FF_FAIL;
}

protected:	

	// Parameters
	float m_Threshold;
	float m_UserThreshold;
	float m_AutoThreshold;
	float m_Smoothness;
	int   m_TwoTone;
	int   m_Chroma;
	int   m_Auto;
	
	float m_Red1;
	float m_Grn1;
	float m_Blu1;
	float m_Alf1;

	float m_Red2;
	float m_Grn2;
	float m_Blu2;
	float m_Alf2;
	
	int m_initResources;
	FFGLExtensions m_extensions;
    FFGLShader m_shader;

	GLint m_thresholdLocation;
	GLint m_smoothnessLocation;
	GLint m_twotoneLocation;
	GLint m_ChromaLocation;
	
	GLint m_red1Location;
	GLint m_grn1Location;
	GLint m_blu1Location;
	GLint m_alf1Location;
	
	GLint m_red2Location;
	GLint m_grn2Location;
	GLint m_blu2Location;
	GLint m_alf2Location;
	
	unsigned char *image;
	unsigned short m_histogram[256];

	bool LoadFromTexture(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height, unsigned char *data);
	float gradient (unsigned char *lp, int Width,  int Height);

	int entropySplit(unsigned short histogram[256]);
	int otsu(int Width, int Height, unsigned short histogram[256]);
	void histo(unsigned char* pix, int width, int height, unsigned short histogram[256]);


};


#endif
