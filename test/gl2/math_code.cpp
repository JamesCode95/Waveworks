/*
 * math_code.cpp
 *
 *  Created on: 21.03.2011
 *      Author: ttcheblokov
 */

#include "math_code.h"

void cutLowerHalfspace(float result[4][4], float level, float position, float viewmatrix[4][4], float projectionmatrix[4][4])
{
	float viewprojectionmatrix[4][4],tm[4][4],n[4][4]={1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
	float cplane[4],clip_plane[4];

	cplane[0]=0.0f;
	cplane[1]=0.1f;
	cplane[2]=0.0f;
	cplane[3]=-level*0.1f;//+0.1f;

	mat4Mat4Mul(viewprojectionmatrix, projectionmatrix, viewmatrix);
	mat4Inverse(tm,viewprojectionmatrix);
	mat4Vec4Mul(clip_plane,tm,cplane);

	//clip_plane[0]/=cplane[1];
	//clip_plane[1]/=cplane[1];
	//clip_plane[2]/=cplane[1];
	//clip_plane[3]/=cplane[1];

    clip_plane[3] -= 1.0f;

	if((level<position))
	{
        clip_plane[0] *= -1.0f;
		clip_plane[1] *= -1.0f;
		clip_plane[2] *= -1.0f;
		clip_plane[3] *= -1.0f;
	}

	n[0][2]=clip_plane[0];
	n[1][2]=clip_plane[1];
	n[2][2]=clip_plane[2];
	n[3][2]=clip_plane[3];

	mat4Mat4Mul(result,n,projectionmatrix);
}

void cutUpperHalfspace(float result[4][4], float level, float position, float viewmatrix[4][4], float projectionmatrix[4][4])
{
	float viewprojectionmatrix[4][4],tm[4][4],n[4][4]={1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
	float cplane[4],clip_plane[4];

	cplane[0]=0.0f;
	cplane[1]=-0.1f;
	cplane[2]=0.0f;
	cplane[3]=level*0.1f;//-0.1f;

	mat4Mat4Mul(viewprojectionmatrix, projectionmatrix, viewmatrix);
	mat4Inverse(tm,viewprojectionmatrix);
	mat4Vec4Mul(clip_plane,tm,cplane);

	//clip_plane[0]/=cplane[1];
	//clip_plane[1]/=cplane[1];
	//clip_plane[2]/=cplane[1];
	//clip_plane[3]/=cplane[1];

    clip_plane[3] -= 1.0f;

	if((level>position))
	{
        clip_plane[0] *= -1.0f;
		clip_plane[1] *= -1.0f;
		clip_plane[2] *= -1.0f;
		clip_plane[3] *= -1.0f;
	}

	n[0][2]=clip_plane[0];
	n[1][2]=clip_plane[1];
	n[2][2]=clip_plane[2];
	n[3][2]=clip_plane[3];

	mat4Mat4Mul(result,n,projectionmatrix);
}

void vec3CrossProductNormalized(float result[3], float a[3], float b[3])
 {
	 float length;
	 result[0]=a[1]*b[2]-a[2]*b[1];
	 result[1]=a[2]*b[0]-a[0]*b[2];
	 result[2]=a[0]*b[1]-a[1]*b[0];
	 length=sqrt(result[0]*result[0]+result[1]*result[1]+result[2]*result[2]);
	 result[0]/=length;
	 result[1]/=length;
	 result[2]/=length;
 }

 void vec4Sub(float a[4], float b[4], float c[4])
 {
 	a[0]=b[0]-c[0];
 	a[1]=b[1]-c[1];
 	a[2]=b[2]-c[2];
 	a[3]=b[3]-c[3];
 }
 void vec4Add(float a[4], float b[4], float c[4])
 {
 	a[0]=b[0]+c[0];
 	a[1]=b[1]+c[1];
 	a[2]=b[2]+c[2];
 	a[3]=b[3]+c[3];
 }
  float vec4DotProduct(float a[4], float b[4])
  {
   return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3]);
  }

  float vec3DotProduct(float a[3], float b[3])
  {
   return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);
  }

  void vec4Normalize(float a[4])
  {
   float length;
   length=sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]+a[3]*a[3]);
   a[0]/=length;
   a[1]/=length;
   a[2]/=length;
   a[3]/=length;
  }
  void vec3Normalize(float a[4])
  {
   float length;
   length=sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
   a[0]/=length;
   a[1]/=length;
   a[2]/=length;
  }
 void mat4Add(float result[4][4], float a[4][4], float b[4][4])
 {
 	int i,j;
 	for (i=0;i<4;++i)
 		for (j=0;j<4;++j)
 			result[i][j]=a[i][j]+b[i][j];
 }
 void mat4ConstMul(float result[4][4], float a[4][4], float b)
 {
 	int i,j;
 	for (i=0;i<4;++i)
 		for (j=0;j<4;++j)
 			result[i][j]=a[i][j]*b;
 }
 void mat4Transpose(float result[4][4], float a[4][4])
 {
 	int i,j;
 	for (i=0;i<4;++i)
 		for (j=0;j<4;++j)
 			result[i][j]=a[j][i];
 }
 void mat4Mat4Mul(float result[4][4], float a[4][4], float b[4][4])
 {
 	int i,j,k;
 	float temp;

 	for (i=0;i<4;++i)
 		for (j=0;j<4;++j)
 		{
 			temp=0;
 			for(k=0;k<4;++k)
 			{
 				temp+=a[k][j]*b[i][k];
 			}
 			result[i][j]=temp;
 		}
 }
 void vec4Mat4Mul(float result[4], float a[4], float b[4][4])
 {
 	int i,k;
 	float temp;

 	for (i=0;i<4;++i)
 		{
 			temp=0;
 			for(k=0;k<4;++k)
 			{
 				temp+=a[k]*b[k][i];
 			}
 			result[i]=temp;
 		}
 }
 void mat4Vec4Mul(float result[4], float a[4][4], float b[4])
 {
 	int i,k;
 	float temp;
 	for (i=0;i<4;++i)
 		{
 			temp=0;
 			for(k=0;k<4;++k)
 			{
 				temp+=a[i][k]*b[k];
 			}
 			result[i]=temp;
 		}
 }
 void mat4CreateIdentity (float result[4][4])
 {
 	int i;
 	for (i=0;i<4;i++)
 	{
 		result[i][0] = 0;
 		result[i][1] = 0;
 		result[i][2] = 0;
 		result[i][3] = 0;
 	}
 	result [0][0] = 1;
 	result [1][1] = 1;
 	result [2][2] = 1;
 	result [3][3] = 1;
 }
 void mat4CreateScale (float result[4][4], float x, float y, float z)
 {
 	int i;
 	for (i=0;i<4;i++)
 	{
 		result[i][0] = 0;
 		result[i][1] = 0;
 		result[i][2] = 0;
 		result[i][3] = 0;
 	}
 	result [0][0] = x;
 	result [1][1] = y;
 	result [2][2] = z;
 	result [3][3] = 1;
 }

 void mat4CreateTranslation (float result[4][4], float x, float y, float z)
 {
 	int i;
 	for (i=0;i<4;i++)
 	{
 		result[i][0] = 0;
 		result[i][1] = 0;
 		result[i][2] = 0;
 		result[i][3] = 0;
 	}
 	result [3][0] = x;
 	result [3][1] = y;
 	result [3][2] = z;
 	result [0][0] = 1;
 	result [1][1] = 1;
 	result [2][2] = 1;
 	result [3][3] = 1;

 }

 void mat4CreateProjection (float result[4][4], float w, float h, float zn, float zf)
 {
 	for (int i=0;i<4;i++)
 	{
 		result[i][0] = 0;
 		result[i][1] = 0;
 		result[i][2] = 0;
 		result[i][3] = 0;
 	}
 	result [0][0] = zn/w;
 	result [1][1] = zn/h;
 	result [2][2] = -(zf+zn)/(zf-zn);
 	result [3][2] = -2.0f*zf*zn/(zf-zn);
 	result [2][3] = -1.0f;
 }

 void mat4CreateView (float result[4][4], float eyepoint[3], float lookatpoint[3])
 {
	float xaxis[3];
	float yaxis[3];
	float zaxis[3];
	float up[3] = {0.0f,0.0f, 1.0f};

	zaxis[0] = - lookatpoint[0] + eyepoint[0];
	zaxis[1] = - lookatpoint[1] + eyepoint[1];
	zaxis[2] = - lookatpoint[2] + eyepoint[2];

	vec3Normalize(zaxis);
	vec3CrossProductNormalized(xaxis, up, zaxis);
	vec3CrossProductNormalized(yaxis, zaxis, xaxis);

	result[0][0] = xaxis[0];
	result[0][1] = yaxis[0];
	result[0][2] = zaxis[0];
	result[0][3] = 0;
	result[1][0] = xaxis[1];
	result[1][1] = yaxis[1];
	result[1][2] = zaxis[1];
	result[1][3] = 0;
	result[2][0] = xaxis[2];
	result[2][1] = yaxis[2];
	result[2][2] = zaxis[2];
	result[2][3] = 0; 
	result[3][0] = -vec3DotProduct(xaxis,eyepoint);
	result[3][1] = -vec3DotProduct(yaxis,eyepoint);
	result[3][2] = -vec3DotProduct(zaxis,eyepoint);
	result[3][3] = 1.0f;
 }

 void mat4CreateOrthoProjection (float result[4][4], float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
 {
	 for(int i=0;i<4;i++)
	 {
		 result[i][0]=0;
		 result[i][1]=0;
		 result[i][2]=0;
		 result[i][3]=0;
	 }
	 result[0][0] = 2.0f/(xmax-xmin);
	 result[1][1] = 2.0f/(ymax-ymin);
	 result[2][2] = -2.0f/(zmax-zmin);
	 result[3][3] = 1.0f;
	 result[3][0] = - (xmax + xmin) / (xmax - xmin);
	 result[3][1] = - (ymax + ymin) / (ymax - ymin);
	 result[3][2] = - (zmax + zmin) / (zmax - zmin);

 }

 void mat4CreateRotation (float result[4][4], float angle, char axis)
 {
 	int i;
 	for (i=0;i<4;i++)
 	{
 		result[i][0] = 0;
 		result[i][1] = 0;
 		result[i][2] = 0;
 		result[i][3] = 0;
 	}
 	result [3][3] = 1;

 	switch (axis)
 	{
 		case 'x':
             result[0][0]=1.0f;
 			result[0][1]=0.0f;
 			result[0][2]=0.0f;
 			result[1][0]=0.0f;
 			result[1][1]=cos(angle);
 			result[1][2]=sin(angle);
 			result[2][0]=0.0f;
 			result[2][1]=-sin(angle);
 			result[2][2]=cos(angle);
 		break;
 		case 'y':
             result[0][0]=cos(angle);
 			result[0][1]=0.0f;
 			result[0][2]=-sin(angle);
 			result[1][0]=0.0f;
 			result[1][1]=1.0f;
 			result[1][2]=0.0f;
 			result[2][0]=sin(angle);
 			result[2][1]=0.0f;
 			result[2][2]=cos(angle);
 		break;
 		case 'z':
             result[0][0]=cos(angle);
 			result[0][1]=sin(angle);
 			result[0][2]=0.0f;
 			result[1][0]=-sin(angle);
 			result[1][1]=cos(angle);
 			result[1][2]=0.0f;
 			result[2][0]=0.0f;
 			result[2][1]=0.0f;
 			result[2][2]=1.0f;
 		break;
 	}

 }
 float sgn (float a)
 {
 	if(a>0.0f) return (1.0f);
 	if(a<0.0f) return (-1.0f);
 	return(0.0f);
 }

 float det2x2(float a,float b,
                     float c,float d)
 {
     return a * d - b * c;
 }

 float det3x3(float a1,float a2,float a3,
              float b1,float b2,float b3,
              float c1,float c2,float c3)
 {
     float   ans;
     ans = a1 * det2x2( b2, b3, c2, c3)
           - b1 * det2x2( a2, a3, c2, c3)
           + c1 * det2x2( a2, a3, b2, b3);
     return ans;
 }

 float det4x4(float m[4][4])
 {
     float   a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;


 	a1 = m[0][0]; b1 = m[0][1];
     c1 = m[0][2]; d1 = m[0][3];

     a2 = m[1][0]; b2 = m[1][1];
     c2 = m[1][2]; d2 = m[1][3];

     a3 = m[2][0]; b3 = m[2][1];
     c3 = m[2][2]; d3 = m[2][3];

     a4 = m[3][0]; b4 = m[3][1];
     c4 = m[3][2]; d4 = m[3][3];

     return  a1 * det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
           - b1 * det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
           + c1 * det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
           - d1 * det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
 }

 void adjoint(float adj[4][4],float m[4][4])
 {
     float   a1,a2,a3,a4,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4;

 	a1 = m[0][0]; b1 = m[0][1];
     c1 = m[0][2]; d1 = m[0][3];

     a2 = m[1][0]; b2 = m[1][1];
     c2 = m[1][2]; d2 = m[1][3];

     a3 = m[2][0]; b3 = m[2][1];
     c3 = m[2][2]; d3 = m[2][3];

     a4 = m[3][0]; b4 = m[3][1];
     c4 = m[3][2]; d4 = m[3][3];

     adj[0][0] =   det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
     adj[1][0] = - det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
     adj[2][0] =   det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
     adj[3][0] = - det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);

     adj[0][1] = - det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
     adj[1][1] =   det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
     adj[2][1] = - det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
     adj[3][1] =   det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);

     adj[0][2] =   det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
     adj[1][2] = - det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
     adj[2][2] =   det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
     adj[3][2] = - det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);

     adj[0][3] = - det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
     adj[1][3] =   det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
     adj[2][3] = - det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
     adj[3][3] =   det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
 }

 void mat4Inverse(float result[4][4],float source[4][4])
 {
     float   adj[4][4];
 	float det;
 	int i,j;

     adjoint(adj,source);

     det = det4x4(source);
     if (fabs(det) < 1e-8f)
 	{
         return ;
 	}
     else
 	{
         det = 1 / det;
         for (i = 0; i < 4; i++)
             for (j = 0; j < 4; j++)
 				result[i][j] = adj[i][j] * det;
     }
 }

 void mat4Mat4Copy (float result[4][4], float source[4][4])
 {
 	int i;
 	for (i=0;i<4;i++)
 	{
 		result[i][0] = source[i][0];
 		result[i][1] = source[i][1];
 		result[i][2] = source[i][2];
 		result[i][3] = source[i][3];
 	}
 }
