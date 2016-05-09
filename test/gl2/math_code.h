/*
 * math_code.h
 *
 *  Created on: 21.03.2011
 *      Author: ttcheblokov
 */
#include <math.h>

#ifndef MATH_CODE_H_
#define MATH_CODE_H_

// vector and matrix math functions
 void cutLowerHalfspace(float result[4][4], float level, float position, float viewmatrix[4][4], float projectionmatrix[4][4]);
 void cutUpperHalfspace(float result[4][4], float level, float position, float viewmatrix[4][4], float projectionmatrix[4][4]);
 void vec3CrossProductNormalized(float result[3], float a[3], float b[3]);
 void vec4Sub(float a[4], float b[4], float c[4]);
 void vec4Add(float a[4], float b[4], float c[4]);
 float vec3DotProduct(float a[3], float b[3]);
 float vec4DotProduct(float a[4], float b[4]);
 void vec4Normalize(float a[4]);
 void vec3Normalize(float a[3]);
 void mat4Add(float result[4][4], float a[4][4], float b[4][4]);
 void mat4ConstMul(float result[4][4], float a[4][4], float b);
 void mat4Transpose(float result[4][4], float a[4][4]);
 void mat4Mat4Mul(float result[4][4], float a[4][4], float b[4][4]);
 void vec4Mat4Mul(float result[4], float a[4], float b[4][4]);
 void mat4Vec4Mul(float result[4], float a[4][4], float b[4]);
 void mat4CreateIdentity (float result[4][4]);
 void mat4CreateScale (float result[4][4], float x, float y, float z);
 void mat4CreateTranslation (float result[4][4], float x, float y, float z);
 void mat4CreateView(float result[4][4], float eyepoint[3], float lookatpoint[3]);
 void mat4CreateProjection (float result[4][4], float w, float h, float zn, float zf);
 void mat4CreateOrthoProjection (float result[4][4], float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
 void mat4CreateRotation (float result[4][4], float angle, char axis);
 void mat4Inverse(float result[4][4],float source[4][4]);
 void mat4Mat4Copy (float result[4][4], float source[4][4]);

 float sgn (float a);
 float det2x2(float a,float b, float c,float d);
 float det3x3(float a1,float a2,float a3,
              float b1,float b2,float b3,
              float c1,float c2,float c3);
 float det4x4(float m[4][4]);
 void adjoint(float adj[4][4],float m[4][4]);




#endif /* MATH_CODE_H_ */
