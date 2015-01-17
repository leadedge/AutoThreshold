//
//		AutoThreshold.cpp
//
//		A freeframe effect for thresholding
//
//		- Automatic detection of threshold level with changing content
//		- Smoothness of threshold boundary
//		- Two tone or chroma effect
//
//		Threshold
//			Default is a simple black and white threshold
//			Adjust the threshold with the "Threshold" slider
//			Alpha of the black is controlled by "Alpha 1"
//			Alpha of the white is controlled by "Alpha 2"
//
//		Smoothness
//			The black and white cutoff is made over a range of values
//			At the limit there is full range and the image is simply grey scale.
//			Adjust between sharp cutoff and a range of grey levels for the desired effect.
//
//		Auto
//			The threshold level is calculated from the brightness levels
//			detected from the image gradient. Dark and light scenes will maintain
//			a threshold independent of the relative brightness.
//			The level can be modulated with	the "Threshold" control to be
//			darker or lighter, but will still adapt to content.
//
//			Gradient method is generally slightly higher threshold than other methods
//			but can be adjusted by the user threshold setting.
//			Next lower is Entropy, then Otsu. All methods work and can be tested.
//			Gradient method seems most reliable.
//
//		Two tone
//			The black and white areas are replaced with two colours.
//			The RGBA of each can be adjusted with the parameter controls.
//
//		Chroma
//			The original image chroma is mixed back into the thresholded result.
//			Alpha of the black is controlled by "Alpha 1"
//			Alpha of the white is controlled by "Alpha 2"
//
//		------------------------------------------------------------
//		Revisions :
//		12-01-15	Version 1.000
//
//		------------------------------------------------------------
//
//		Copyright (C) 2015. Lynn Jarvis, Leading Edge. Pty. Ltd.
//
//		This program is free software: you can redistribute it and/or modify
//		it under the terms of the GNU Lesser General Public License as published by
//		the Free Software Foundation, either version 3 of the License, or
//		(at your option) any later version.
//
//		This program is distributed in the hope that it will be useful,
//		but WITHOUT ANY WARRANTY; without even the implied warranty of
//		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//		GNU Lesser General Public License for more details.
//
//		You will receive a copy of the GNU Lesser General Public License along 
//		with this program.  If not, see http://www.gnu.org/licenses/.
//		--------------------------------------------------------------
//
//
#include <FFGL.h>
#include <FFGLLib.h>
#include <stdio.h>

#include "AutoThreshold.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define FFPARAM_Threshold  (0)
#define FFPARAM_Smoothness (1)
#define FFPARAM_Auto       (2)
#define FFPARAM_TwoTone    (3)
#define FFPARAM_Chroma     (4)
#define FFPARAM_Red1       (5)
#define FFPARAM_Grn1       (6)
#define FFPARAM_Blu1       (7)
#define FFPARAM_Alf1       (8)
#define FFPARAM_Red2       (9)
#define FFPARAM_Grn2       (10)
#define FFPARAM_Blu2       (11)
#define FFPARAM_Alf2       (12)

#define STRINGIFY(A) #A

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo ( 
	AutoThreshold::CreateInstance,				// Create method
	"LJ02",										// Plugin unique ID											
	"AutoThreshold",							// Plugin name											
	1,						   					// API major version number 													
	000,										// API minor version number	
	1,											// Plugin major version number
	000,										// Plugin minor version number
	FF_EFFECT,									// Plugin type
	"Auto Threshold - with smoothness, 2-tone and chroma", // Plugin description
	"by Lynn Jarvis - spout.zeal.co"			// About
);


char *vertexShaderCode = STRINGIFY (
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
} );


// 2 toner threshold
char *fragmentShaderCode = STRINGIFY (
uniform sampler2D tex1;
uniform float Threshold;
uniform float Smoothness;
uniform int TwoTone;
uniform int Chroma;
uniform float Red1;
uniform float Grn1;
uniform float Blu1;
uniform float Red2;
uniform float Grn2;
uniform float Blu2;
uniform float Alf1;
uniform float Alf2;
const vec4 grayScaleWeights = vec4(0.30, 0.59, 0.11, 0.0);

float minChannel(in vec3 v)
{
  float t = (v.x<v.y) ? v.x : v.y;
  t = (t<v.z) ? t : v.z;
  return t;
}

float maxChannel(in vec3 v)
{
  float t = (v.x>v.y) ? v.x : v.y;
  t = (t>v.z) ? t : v.z;
  return t;
}

vec3 rgbToHsv(in vec3 rgb)
{
  vec3  hsv = vec3(0.0);
  float minVal = minChannel(rgb);
  float maxVal = maxChannel(rgb);
  float delta = maxVal - minVal;

  hsv.z = maxVal;

  if (delta != 0.0) {
    hsv.y = delta / maxVal;
    vec3 delRGB;
    delRGB = (((vec3(maxVal) - rgb) / 6.0) + (delta/2.0)) / delta;

    if (rgb.x == maxVal) {
      hsv.x = delRGB.z - delRGB.y;
    } else if (rgb.y == maxVal) {
      hsv.x = ( 1.0/3.0) + delRGB.x - delRGB.z;
    } else if (rgb.z == maxVal) {
      hsv.x = ( 2.0/3.0) + delRGB.y - delRGB.x;
    }

    if ( hsv.x < 0.0 ) { 
      hsv.x += 1.0; 
    }
    if ( hsv.x > 1.0 ) { 
      hsv.x -= 1.0; 
    }
  }
  return hsv;
}

vec3 hsvToRgb(in vec3 hsv)
{
  vec3 rgb = vec3(hsv.z);
  if ( hsv.y != 0.0 ) {
    float var_h = hsv.x * 6.0;
    float var_i = floor(var_h);   // Or ... var_i = floor( var_h )
    float var_1 = hsv.z * (1.0 - hsv.y);
    float var_2 = hsv.z * (1.0 - hsv.y * (var_h-var_i));
    float var_3 = hsv.z * (1.0 - hsv.y * (1.0 - (var_h-var_i)));

    switch (int(var_i)) {
      case  0: rgb = vec3(hsv.z, var_3, var_1); break;
      case  1: rgb = vec3(var_2, hsv.z, var_1); break;
      case  2: rgb = vec3(var_1, hsv.z, var_3); break;
      case  3: rgb = vec3(var_1, var_2, hsv.z); break;
      case  4: rgb = vec3(var_3, var_1, hsv.z); break;
      default: rgb = vec3(hsv.z, var_1, var_2); break;
    }
  }
  return rgb;
}

void main (void) {

	vec4 c1 = vec4(Red1, Grn1, Blu1, Alf1);
	vec4 c2 = vec4(Red2, Grn2, Blu2, Alf2);

	 // lookup input color
	vec2 texCoord = gl_TexCoord[0].st;
	vec4 c0 = texture2D(tex1, texCoord);

	// Convert to HSV for combining chroma
	vec3 hsv = rgbToHsv(vec3(c0.r, c0.g, c0.b));

	// calculate luminance
	float luminance = dot(c0, grayScaleWeights);
	
	// Threshold with smoothing
	float f = smoothstep(Threshold, Threshold+Smoothness, luminance);

	// Alpha for thresholded result
	//   - black is alpha1
	//   - white is alpha2
	float alf = Alf1;
	if(f > 0.5) alf = Alf2;

	if(TwoTone > 0 && Chroma <= 0)	{ // 2-tone
		gl_FragColor = f*c1 + (1.0-f)*c2;
	}
	else if(Chroma > 0 && TwoTone <= 0) { // chroma
		// Now we have luminance so convert back from HSV to RGB
		vec3 c = hsvToRgb(vec3(hsv.x, hsv.y, f));
		gl_FragColor = vec4(c, alf);
	}
	else { // B&W
		gl_FragColor = vec4(f, f, f, alf);
	}

} );


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

AutoThreshold::AutoThreshold()
:CFreeFrameGLPlugin(),
 m_initResources(1),
 m_thresholdLocation(-1),
 m_smoothnessLocation(-1),
 m_twotoneLocation(-1),
 m_ChromaLocation(-1),
 m_red1Location(-1),
 m_grn1Location(-1),
 m_blu1Location(-1),
 m_red2Location(-1),
 m_grn2Location(-1),
 m_blu2Location(-1),
 m_alf1Location(-1),
 m_alf2Location(-1)
{

	/*
	// Debug console window so printf works
	FILE* pCout; // should really be freed on exit 
	AllocConsole();
	freopen_s(&pCout, "CONOUT$", "w", stdout); 
	printf("AutoThreshold\n");
	*/

	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	SetParamInfo(FFPARAM_Threshold,  "Threshold",  FF_TYPE_STANDARD, 0.5f);  m_UserThreshold = 0.5f;
	SetParamInfo(FFPARAM_Smoothness, "Smoothness", FF_TYPE_STANDARD, 0.0f);	 m_Smoothness = 0.0f;
	SetParamInfo(FFPARAM_Auto,       "Auto",       FF_TYPE_BOOLEAN, false);  m_Auto = 0;
	SetParamInfo(FFPARAM_TwoTone,    "Two tone",   FF_TYPE_BOOLEAN, false);  m_TwoTone = 0;
	SetParamInfo(FFPARAM_Chroma,     "Chroma",     FF_TYPE_BOOLEAN, false);  m_Chroma = 0;
	SetParamInfo(FFPARAM_Red1,       "Red 1",      FF_TYPE_STANDARD, 0.0f);  m_Red1 = 1.0f;
	SetParamInfo(FFPARAM_Grn1,       "Green 1",    FF_TYPE_STANDARD, 0.82f); m_Grn1 = 0.82f;
	SetParamInfo(FFPARAM_Blu1,       "Blue 1",     FF_TYPE_STANDARD, 1.0f);  m_Blu1 = 1.0f;
	SetParamInfo(FFPARAM_Alf1,       "Alpha 1",    FF_TYPE_STANDARD, 1.0f);  m_Alf1 = 1.0f;
	SetParamInfo(FFPARAM_Red2,       "Red 2",      FF_TYPE_STANDARD, 0.93f); m_Red2 = 0.93f;
	SetParamInfo(FFPARAM_Grn2,       "Green 2",    FF_TYPE_STANDARD, 0.0f);  m_Grn2 = 0.0f;
	SetParamInfo(FFPARAM_Blu2,       "Blue 2",     FF_TYPE_STANDARD, 0.0f);  m_Blu2 = 0.0f;
	SetParamInfo(FFPARAM_Alf2,       "Alpha 2",    FF_TYPE_STANDARD, 1.0f);  m_Alf2 = 1.0f;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD AutoThreshold::InitGL(const FFGLViewportStruct *vp)
{
	// initialize gl extensions and
	// make sure required features are supported
	m_extensions.Initialize();
	if (m_extensions.multitexture==0 || m_extensions.ARB_shader_objects==0)
		return FF_FAIL;

	// initialize gl shader
	m_shader.SetExtensions(&m_extensions);
	if (!m_shader.Compile(vertexShaderCode, fragmentShaderCode))
	  MessageBoxA(NULL, "Shader compile error", "Error", MB_OK);
 
	// activate our shader
	bool success = false;
	if (m_shader.IsReady()) {
		if (m_shader.BindShader())
			success = true;
	}

	if (!success){
		MessageBoxA(NULL, "Shader bind error", "Error", MB_OK);
	}

	m_Threshold = 0.0;
	m_UserThreshold = 0.0;
	m_AutoThreshold = 0.0;
    
	// lookup location of the uniforms
	m_thresholdLocation   = m_shader.FindUniform("Threshold");
	m_smoothnessLocation  = m_shader.FindUniform("Smoothness");
	m_twotoneLocation     = m_shader.FindUniform("TwoTone");
	m_ChromaLocation      = m_shader.FindUniform("Chroma");
	m_red1Location        = m_shader.FindUniform("Red1");
	m_grn1Location        = m_shader.FindUniform("Grn1");
	m_blu1Location        = m_shader.FindUniform("Blu1");
	m_alf1Location        = m_shader.FindUniform("Alf1");
	m_red2Location        = m_shader.FindUniform("Red2");
	m_grn2Location        = m_shader.FindUniform("Grn2");
	m_blu2Location        = m_shader.FindUniform("Blu2");
	m_alf2Location        = m_shader.FindUniform("Alf2");

	m_shader.UnbindShader();

	return FF_SUCCESS;
}

DWORD AutoThreshold::DeInitGL()
{
	m_shader.FreeGLResources();
	return FF_SUCCESS;
}

DWORD AutoThreshold::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	if (pGL->numInputTextures < 1) return FF_FAIL;
	if (pGL->inputTextures[0] == NULL) return FF_FAIL;

	FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

	// For auto threshold, use the threshold from the last frame, modified by the user entry
	// printf("Auto=%f, User = %f, Thresh=%f\n", m_AutoThreshold, m_UserThreshold, m_Threshold);
	if(m_Auto)
		m_Threshold = m_AutoThreshold*m_UserThreshold*2.0f;
	else
		m_Threshold = m_UserThreshold;

	if(m_Threshold < 0.0) m_Threshold = 0.0;
	if(m_Threshold > 1.0) m_Threshold = 1.0;

	// activate our shader
	m_shader.BindShader();

	// Set the threshold to the shader
	m_extensions.glUniform1fARB(m_thresholdLocation, m_Threshold);
	m_extensions.glUniform1fARB(m_smoothnessLocation, m_Smoothness);
	m_extensions.glUniform1iARB(m_twotoneLocation, m_TwoTone);
	m_extensions.glUniform1iARB(m_ChromaLocation, m_Chroma);
	m_extensions.glUniform1fARB(m_red1Location, m_Red1);
	m_extensions.glUniform1fARB(m_grn1Location, m_Grn1);
	m_extensions.glUniform1fARB(m_blu1Location, m_Blu1);
	m_extensions.glUniform1fARB(m_alf1Location, m_Alf1);
	m_extensions.glUniform1fARB(m_red2Location, m_Red2);
	m_extensions.glUniform1fARB(m_grn2Location, m_Grn2);
	m_extensions.glUniform1fARB(m_blu2Location, m_Blu2);
	m_extensions.glUniform1fARB(m_alf2Location, m_Alf2);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);

	glBegin(GL_QUADS);
		//lower left
		glTexCoord2f(0.0, 0.0);
		glVertex2f(-1.0, -1.0);
		//upper left
		glTexCoord2f(0.0, (float)maxCoords.t);
		glVertex2f(-1.0, 1.0);
		//upper right
		glTexCoord2f((float)maxCoords.s, (float)maxCoords.t);
		glVertex2f(1.0, 1.0);
		//lower right
		glTexCoord2f((float)maxCoords.s, 0.0);
		glVertex2f(1.0, -1.0);
	glEnd();

	// unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);
  
	// unbind the shader
	m_shader.UnbindShader();

	//
	// Auto threshold option
	// TODO - make more efficient
	//
	if(m_Auto) {

		// Allocate an image buffer
		unsigned char *buffer = (unsigned char *)malloc(Texture.Width*Texture.Height*4*sizeof(unsigned char)); // assume ARGB
		
		// Load the buffer with the texture pixels via PBO
		LoadFromTexture(Texture.Handle, GL_TEXTURE_2D, Texture.Width, Texture.Height, buffer);

		// Gradient method
		m_AutoThreshold = gradient(buffer, Texture.Width, Texture.Height); 
		// printf("Gradient %5.3f\n", m_AutoThreshold);

		// Entropy split method
		// http://rsb.info.nih.gov/ij/plugins/download/AutoThresholder.java
		// http://fiji.sc/wiki/index.php/Auto_Threshold#RenyiEntropy
		// histo(buffer, (int)Texture.Width, (int)Texture.Height, m_histogram);
		// int iThresh = entropySplit(m_histogram); // entropy auto threshold
		// m_AutoThreshold = ((float)iThresh)/256;
		// printf("Entropy %5.3f\n", m_AutoThreshold);

		// Otsu method
		// http://www.labbookpages.co.uk/software/imgProc/otsuThreshold.html
		// http://cis.k.hosei.ac.jp/~wakahara/otsu_th.c
		// int iThresh = otsu((int)Texture.Width, (int)Texture.Height, m_histogram);
		// m_AutoThreshold = ((float)iThresh)/256;
		// printf("Otsu %5.3f\n", m_AutoThreshold);

		free((void*)buffer);

	}

  
	return FF_SUCCESS;
}

DWORD AutoThreshold::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	switch (dwIndex) {

		case FFPARAM_Threshold:
			*((float *)(unsigned)&dwRet) = m_UserThreshold;
			return dwRet;

		case FFPARAM_Smoothness:
			*((float *)(unsigned)&dwRet) = m_Smoothness;
			return dwRet;

		case FFPARAM_Auto:
			*((float *)(unsigned)&dwRet) = (float)m_Auto;
			return dwRet;

		case FFPARAM_TwoTone:
			*((float *)(unsigned)&dwRet) = (float)m_TwoTone;
			return dwRet;

		case FFPARAM_Chroma:
			*((float *)(unsigned)&dwRet) = (float)m_Chroma;
			return dwRet;

		case FFPARAM_Red1:
			*((float *)(unsigned)&dwRet) = m_Red1;
			return dwRet;

		case FFPARAM_Grn1:
			*((float *)(unsigned)&dwRet) = m_Grn1;
			return dwRet;

		case FFPARAM_Blu1:
			*((float *)(unsigned)&dwRet) = m_Blu1;
			return dwRet;

		case FFPARAM_Alf1:
			*((float *)(unsigned)&dwRet) = m_Alf1;
			return dwRet;

		case FFPARAM_Red2:
			*((float *)(unsigned)&dwRet) = m_Red2;
			return dwRet;

		case FFPARAM_Grn2:
			*((float *)(unsigned)&dwRet) = m_Grn2;
			return dwRet;

		case FFPARAM_Blu2:
			*((float *)(unsigned)&dwRet) = m_Blu2;
			return dwRet;

		case FFPARAM_Alf2:
			*((float *)(unsigned)&dwRet) = m_Alf2;
			return dwRet;

		default:
			return FF_FAIL;

	}
}

DWORD AutoThreshold::SetParameter(const SetParameterStruct* pParam)
{

	if (pParam != NULL) {
		
		switch (pParam->ParameterNumber) {

			case FFPARAM_Threshold:
				m_UserThreshold = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Smoothness:
				m_Smoothness = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Auto:
				if(pParam->NewParameterValue > 0)
					m_Auto = 1;
				else
					m_Auto = 0;
				break;

			case FFPARAM_TwoTone:
				if(pParam->NewParameterValue > 0)
					m_TwoTone = 1;
				else
					m_TwoTone = 0;
				break;


			case FFPARAM_Chroma:
				if(pParam->NewParameterValue > 0)
					m_Chroma = 1;
				else
					m_Chroma = 0;
				break;

			case FFPARAM_Red1:
				m_Red1 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Grn1:
				m_Grn1 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Blu1:
				m_Blu1 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Alf1:
				m_Alf1 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Red2:
				m_Red2 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Grn2:
				m_Grn2 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Blu2:
				m_Blu2 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			case FFPARAM_Alf2:
				m_Alf2 = *((float *)(unsigned)&(pParam->NewParameterValue));
				break;

			default:
				return FF_FAIL;
		}

		return FF_SUCCESS;
	
	}

	return FF_FAIL;
}

// All this is so that the process does not stall OpenGL
bool AutoThreshold::LoadFromTexture(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height, unsigned char *data)
{
	GLuint pboID1 = 0;
	GLuint tempFBO = 0;

	void *pboMemory;
	m_extensions.glGenBuffers(1, &pboID1);
	m_extensions.glGenFramebuffersEXT(1, &tempFBO); 

	// GL_PIXEL_PACK_BUFFER_ARB is for transferring pixel data from OpenGL to your application,
	// and GL_PIXEL_UNPACK_BUFFER_ARB means transferring pixel data from an application to OpenGL.
	// OpenGL refers to these tokens to determine the best memory space of a PBO, for example,
	// a video memory for uploading (unpacking) textures, or system memory for reading (packing)
	// the framebuffer.

	// Bind buffer
	m_extensions.glBindBuffer (GL_PIXEL_PACK_BUFFER, pboID1);

	// If you specify a NULL pointer to the source array in glBufferDataARB(), 
	// then PBO allocates only a memory space with the given data size. 
	// The last parameter of glBufferDataARB() is another performance hint
	// for PBO to provide how the buffer object will be used. 
	// GL_STREAM_DRAW_ARB is for streaming texture upload and GL_STREAM_READ_ARB
	// is for asynchronous framebuffer read-back. 

	
	// Note that glMapBufferARB() causes sync issue.
	// If GPU is working with this buffer, glMapBufferARB() will wait(stall)
	// until GPU to finish its job. To avoid waiting (idle), you can call
	// first glBufferDataARB() with NULL pointer before glMapBufferARB().
	// If you do that, the previous data in PBO will be discarded and
	// glMapBufferARB() returns a new allocated pointer immediately
	// even if GPU is still working with the previous data.
	// LJ noted a vsync problem full screen - cured by setting NVIDIA "adaptive"
	m_extensions.glBufferData(GL_PIXEL_PACK_BUFFER, width*height*4, NULL, GL_STREAM_READ);

	// while PBO is still bound, transfer the data from the texture to the PBO
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tempFBO); 
	m_extensions.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, TextureTarget, TextureID, 0);
	glReadPixels(0, 0, width, height, GL_RGBA,  GL_UNSIGNED_BYTE, 0);
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	m_extensions.glBindBuffer (GL_PIXEL_PACK_BUFFER, 0);

	// To download data to the CPU, bind the buffer to the GL_PIXEL_UNPACK_BUFFER target
	// using glBindBufferARB(). The glTexImage2D() command will then unpack (read) their data
	// from the buffer object. 

	// Now bind for readback
	m_extensions.glBindBuffer (GL_PIXEL_UNPACK_BUFFER, pboID1);

	pboMemory = m_extensions.glMapBuffer (GL_PIXEL_UNPACK_BUFFER, GL_READ_ONLY);

	// Read the data from PBO memory into the CPU buffer
	CopyMemory((void *)data, pboMemory, width*height*4);

	// Unmap buffer, indicating we are done reading data from it
	m_extensions.glUnmapBuffer (GL_PIXEL_UNPACK_BUFFER);

	// Unbind buffer
	m_extensions.glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);

	if(pboID1) m_extensions.glDeleteBuffers(1, &pboID1);
	if(tempFBO) m_extensions.glDeleteFramebuffersEXT(1, &tempFBO);

	return true;

}

//
// requires an RGBA image
//
// Looks at the variance around sampled pixels
//
// TODO - more efficient
float AutoThreshold::gradient (unsigned char *buffer, int Width,  int Height)
{
	
	int i, t;
	int left, right;
	int top, bot, mid, j;
	int ex, ey, exy;
	int sum_lo, sum_hi, sum_exy_lo, sum_exy_hi;
	double sum_exy_fxy, sum_exy;
	unsigned char *linePtr;
	unsigned char *lp;

	sum_exy_lo  = 0;
	sum_exy_hi  = 0;
	sum_lo      = 0;
	sum_hi      = 0;
	sum_exy_fxy = 0;
	sum_exy     = 0;

	lp = buffer; // local pointer

	for(i=4; i<Height-4; i+=4) { // every fourth line for speed

		linePtr = buffer + i*(Width *4); // start of the line

		for(j=4; j<Width-4; j+=4) { // every fourth column for speed

			lp = linePtr + j*4; // start of the pixel

			// neighbours
			// Move across the line 4 pixels at a time
			// Pointer starts at R (*)
			//
			// [R G B A][R G B A][R G B A]
			// [R G B A][* G B A][R G B A]
			// [R G B A][R G B A][R G B A]
			//
			// Left
			left =  0;
			left += (int)*(lp-4); // R
			left += (int)*(lp-3); // G
			left += (int)*(lp-2); // B

			// Centre
			mid =  0;
			mid += (int)*(lp  ); // R
			mid += (int)*(lp+1); // G
			mid += (int)*(lp+2); // B

			// Right
			right = 0;
			right += (int)*(lp+4); // R
			right += (int)*(lp+5); // G
			right += (int)*(lp+6); // B

			// Top
			top = 0;
			top += (int)*(lp-Width*4  );
			top += (int)*(lp-Width*4+1);
			top += (int)*(lp-Width*4+2);

			// Bot
			bot = 0;
			top += (int)*(lp+Width*4  );
			top += (int)*(lp+Width*4+1);
			top += (int)*(lp+Width*4+2);

			// calculate variance of neighbourhood
			ex           = abs(left - right);
			ey           = abs(top - bot);
			exy          = MAX(ex, ey);
			sum_exy      = sum_exy + (double)exy;
			sum_exy_fxy  = sum_exy_fxy + (double)(exy*mid);

		} // end of all pixels in this line

	} // end of all lines

	// Calculate the threshold
	t = 0;
	sum_exy = sum_exy + (double)1.0;
	t = (int)(sum_exy_fxy/sum_exy);

	return (float)t/(3*256); // 256 levels and RGB pixels

} // end gradient


//
// TODO - luminance
//
void AutoThreshold::histo(unsigned char *buffer, int Width,  int Height, unsigned short histogram[256])
{
	int i, j, k;
	unsigned char *linePtr;
	unsigned char *lp;

	memset((void *)histogram, 0, 256*sizeof(unsigned short));

	for(i=0; i<Height; i++) { // every fourth line for speed
		linePtr = buffer + i*(Width *4); // start of the line
		for(j=0; j<Width; j++) { // every fourth column for speed
			lp = linePtr + j*4; // start of the pixel
			// [R G B A]
			k  = (int)*lp++; // R
			k += (int)*lp++; // G
			k += (int)*lp;   // B
			histogram[k/3]++;
		}
	}
}



/**
* Automatic thresholding technique based on the entopy of the histogram.
* See: P.K. Sahoo, S. Soltani, K.C. Wong and, Y.C. Chen "A Survey of
* Thresholding Techniques", Computer Vision, Graphics, and Image Processing
* Vol. 41, pp.233-260, 1988.
*/
int AutoThreshold::entropySplit(unsigned short histogram[256])
{
    double hist[256];
    double normalizedHist[256];
    double pT[256];
    double hB[256];
    double hW[256];
    double sum;
    double epsilon;
    double hhB;
    double pTW;
    double hhW;
    double djMax;
    double dj;
    int i, t, tMax;

    // Build floating point histogram
    for(i=0; i<256; i++)
      hist[i] = (double)histogram[i];

    // Normalize histogram, that is makes the sum of all bins equal to 1.
    sum = 0;
    for(i=0; i<256; i++)
     sum += hist[i];

    // This should not normally happen, but...
    if(sum == 0)
     return 0;

    for(i=0; i<256; i++)
      normalizedHist[i] = hist[i] / sum;

    pT[0] = normalizedHist[0];
    for(i=1; i<256; i++)
      pT[i] = pT[i-1] + normalizedHist[i];

    // Entropy for black and white parts of the histogram
    epsilon = 0.0;
    for(t=0; t<256; t++) {
      // Black entropy
      if (pT[t] > epsilon) {
        hhB = 0;
        for(i=0; i<=t; i++) {
          if (normalizedHist[i] > epsilon) {
            hhB -= normalizedHist[i] / pT[t] * log(normalizedHist[i] / pT[t]);
          }
        }
        hB[t] = hhB;
      } else {
        hB[t] = 0;
      }

      // White  entropy
      pTW = 1 - pT[t];
      if (pTW > epsilon) {
        hhW = 0;
        for(i=t+1; i<256; i++) {
          if (normalizedHist[i] > epsilon) {
            hhW -= normalizedHist[i] / pTW * log(normalizedHist[i] / pTW);
          }
        }
        hW[t] = hhW;
      } else {
        hW[t] = 0;
      }
    } // end for all histogram

    // Find histogram index with maximum entropy
    djMax = hB[0] + hW[0];
    tMax = 0;
    for(t=1; t<256; t++) {
      dj = hB[t] + hW[t];
      if(dj > djMax) {
        djMax = dj;
        tMax = t;
      }
    }
    return tMax;
}

//
int AutoThreshold::otsu(int Width, int Height, unsigned short hist[256])
{
	double prob[256], omega[256]; // prob of graylevels
	double myu[256];   // mean value for separation
	double max_sigma, sigma[256]; // inter-class variance
	int i, x, y; // Loop variable
	int threshold; // threshold for binarization
  
	// calculation of probability density 
	for ( i = 0; i < 256; i ++ ) {
		prob[i] = (double)hist[i] / (Width * Height);
	}
  
	// omega & myu generation
	omega[0] = prob[0];
	myu[0] = 0.0;       // 0.0 times prob[0] equals zero
	for (i = 1; i < 256; i++) {
		omega[i] = omega[i-1] + prob[i];
		myu[i] = myu[i-1] + i*prob[i];
	}
  
	// sigma maximization
	// sigma stands for inter-class variance 
	// and determines optimal threshold value
	threshold = 0;
	max_sigma = 0.0;
	for (i = 0; i < 256-1; i++) {
		if (omega[i] != 0.0 && omega[i] != 1.0)
			sigma[i] = pow(myu[256-1]*omega[i] - myu[i], 2) / (omega[i]*(1.0 - omega[i]));
		else
			sigma[i] = 0.0;
		if (sigma[i] > max_sigma) {
			max_sigma = sigma[i];
			threshold = i;
		}
	}

 	return threshold;

}
