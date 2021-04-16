#include "framework.h"

//#include "includes.h"

#include <cassert>
#include <cmath> //for sqrt (square root) function
#include <math.h> //atan2
#include <vector>
#include <cstring>
#include <algorithm>
#include <iostream>

#define M_PI_2 1.57079632679489661923

//**************************************
float Vector2::distance(const Vector2& v)
{
	return (float)(v - *this).length();
}

float Vector2::dot( const Vector2& v )
{
	return x * v.x + y * v.y;
}

float Vector2::perpdot( const Vector2& v )
{
	return y * v.x + -x * v.y;
}

void Vector2::random(float range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (float)(rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
	y = (float)(rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
}

void Vector2::parseFromText(const char* text)
{
	int pos = 0;
	char num[255];
	const char* start = text;
	const char* current = text;

	while(1)
	{
		if (*current == ',' || (*current == '\0' && current != text) )
		{
			strncpy(num, start, current - start);
			num[current - start] = '\0';
			start = current + 1;
			switch(pos)
			{
				case 0: x = (float)atof(num); break;
				case 1: y = (float)atof(num); break;
				default: return; break;
			}
			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
}


Vector2 operator * (const Vector2& a, float v) { return Vector2(a.x * v, a.y * v); }
Vector2 operator + (const Vector2& a, const Vector2& b) { return Vector2(a.x + b.x, a.y + b.y); }
Vector2 operator - (const Vector2& a, const Vector2& b) { return Vector2(a.x - b.x, a.y - b.y); }


// **************************************

double Vector3::length() 
{
	return sqrt(x*x + y*y + z*z);
}

double Vector3::length() const
{
	return sqrt(x*x + y*y + z*z);
}

Vector3& Vector3::normalize()
{
	double len = length();
	assert(len > 0.00000000001 && "Cannot normalize a vector with module 0");
	x = (float)(x / len);
	y = (float)(y / len);
	z = (float)(z / len);
	return *this;
}

float Vector3::distance(const Vector3& v) const
{
	return (float)(v - *this).length();
}

Vector3 Vector3::cross( const Vector3& b ) const
{
	return Vector3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
}

float Vector3::dot( const Vector3& v ) const
{
	return x*v.x + y*v.y + z*v.z;
}

void Vector3::random(float range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (float)((rand() / (double)RAND_MAX) * 2 * range - range); //value between -range and range
	y = (float)((rand() / (double)RAND_MAX) * 2 * range - range); //value between -range and range
	z = (float)((rand() / (double)RAND_MAX) * 2 * range - range); //value between -range and range
}

void Vector3::random(Vector3 range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (float)((rand() / (double)RAND_MAX) * 2 * range.x - range.x); //value between -range and range
	y = (float)((rand() / (double)RAND_MAX) * 2 * range.y - range.y); //value between -range and range
	z = (float)((rand() / (double)RAND_MAX) * 2 * range.z - range.z); //value between -range and range
}

void Vector3::setMin(const Vector3 & v)
{
	if (v.x < x) x = v.x;
	if (v.y < y) y = v.y;
	if (v.z < z) z = v.z;
}

void Vector3::setMax(const Vector3 & v)
{
	if (v.x > x) x = v.x;
	if (v.y > y) y = v.y;
	if (v.z > z) z = v.z;
}

void Vector3::parseFromText(const char* text, const char separator)
{
	int pos = 0;
	char num[255];
	const char* start = text;
	const char* current = text;

	while(1)
	{
		if (*current == separator || (*current == '\0' && current != text) )
		{
			strncpy(num, start, current - start);
			num[current - start] = '\0';
			start = current + 1;
			if (num[0] != 'x') //�?
				switch(pos)
				{
					case 0: x = (float)atof(num); break;
					case 1: y = (float)atof(num); break;
					case 2: z = (float)atof(num); break;
					default: return; break;
				}

			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
};

float dot(const Vector3& a, const Vector3& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vector3 cross(const Vector3& a, const Vector3& b)
{
	return Vector3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

Vector3 lerp(const Vector3& a, const Vector3& b, float v)
{
	return a * (1.0 - v) + b * v;
}


//*********************************
const Matrix44 Matrix44::IDENTITY;

Matrix44::Matrix44()
{
	setIdentity();
}

Matrix44::Matrix44(const float* v)
{
	memcpy( (void*)m, (void*)v, sizeof(float) * 16);
}

#ifdef FIXEDPIPELINE
void Matrix44::set()
{
	glMatrixMode( GL_MODELVIEW );
	glMultMatrixf(m);
}

void Matrix44::load()
{
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf(m);
}
#endif

void Matrix44::clear()
{
	memset(m, 0, 16*sizeof(float));
}

void Matrix44::setIdentity()
{
	m[0]=1; m[4]=0; m[8]=0; m[12]=0;
	m[1]=0; m[5]=1; m[9]=0; m[13]=0;
	m[2]=0; m[6]=0; m[10]=1; m[14]=0;
	m[3]=0; m[7]=0; m[11]=0; m[15]=1;
}

void Matrix44::transpose()
{
   std::swap(m[1],m[4]); std::swap(m[2],m[8]); std::swap(m[3],m[12]);
   std::swap(m[6],m[9]); std::swap(m[7],m[13]); std::swap(m[11],m[14]);
}

Vector3 Matrix44::rotateVector(const Vector3& v) const
{
	return (*this * Vector4(v,0.0)).xyz();
}

void Matrix44::translateGlobal(float x, float y, float z)
{
	Matrix44 T;
	T.setTranslation(x, y, z);
	*this = *this * T;
}

void Matrix44::rotateGlobal( float angle_in_rad, const Vector3& axis )
{
	Matrix44 R;
	R.setRotation(angle_in_rad, axis);
	*this = *this * R;
}

void Matrix44::translate(float x, float y, float z)
{
	Matrix44 T;
	T.setTranslation(x, y, z);
	*this = T * *this;
}

void Matrix44::rotate( float angle_in_rad, const Vector3& axis )
{
	Matrix44 R;
	R.setRotation(angle_in_rad, axis);
	*this = R * *this;
}


void Matrix44::scale(float x, float y, float z)
{
	Matrix44 S;
	S.setScale(x, y, z);
	*this = S * *this;
}

void Matrix44::setScale(float x, float y, float z)
{
	setIdentity();
	m[0] = x;
	m[5] = y;
	m[10] = z;
}

//To create a traslation matrix
void Matrix44::setTranslation(float x, float y, float z)
{
	setIdentity();
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

Vector3 Matrix44::getTranslation()
{
	return Vector3(m[12],m[13],m[14]);
}

//To create a rotation matrix
void Matrix44::setRotation( float angle_in_rad, const Vector3& axis  )
{
	clear();
	Vector3 axis_n = axis;
	axis_n.normalize();

	float c = cos( angle_in_rad );
	float s = sin( angle_in_rad );
	float t = 1 - c;

	M[0][0] = t * axis_n.x * axis_n.x + c;
	M[0][1] = t * axis_n.x * axis_n.y - s * axis_n.z;
	M[0][2] = t * axis_n.x * axis_n.z + s * axis_n.y;

	M[1][0] = t * axis_n.x * axis_n.y + s * axis_n.z;
	M[1][1] = t * axis_n.y * axis_n.y + c;
	M[1][2] = t * axis_n.y * axis_n.z - s * axis_n.x;

	M[2][0] = t * axis_n.x * axis_n.z - s * axis_n.y;
	M[2][1] = t * axis_n.y * axis_n.z + s * axis_n.x;
	M[2][2] = t * axis_n.z * axis_n.z + c;

	M[3][3] = 1.0f;
}

Matrix44 Matrix44::getRotationOnly()
{
	Matrix44 trans = *this;
	trans.transpose();

	Matrix44 inv = *this;
	inv.inverse();

	return trans * inv;
}

bool Matrix44::getXYZ(float* euler) const
{
// Code adapted from www.geometrictools.com
//	Matrix3<Real>::EulerResult Matrix3<Real>::ToEulerAnglesXYZ 
    // +-           -+   +-                                        -+
    // | r00 r01 r02 |   |  cy*cz           -cy*sz            sy    |
    // | r10 r11 r12 | = |  cz*sx*sy+cx*sz   cx*cz-sx*sy*sz  -cy*sx |
    // | r20 r21 r22 |   | -cx*cz*sy+sx*sz   cz*sx+cx*sy*sz   cx*cy |
    // +-           -+   +-                                        -+
    if (_13 < 1.0f)
    {
        if (_13 > - 1.0f)
        {
            // y_angle = asin(r02)
            // x_angle = atan2(-r12,r22)
            // z_angle = atan2(-r01,r00)
            euler[1] = asinf(_13);
            euler[0] = atan2f(-_23,_33);
            euler[2] = atan2f(-_12,_11);
            return true;
        }
        else
        {
            // y_angle = -pi/2
            // z_angle - x_angle = atan2(r10,r11)
            // WARNING.  The solution is not unique.  Choosing z_angle = 0.
            euler[1] = (float)-M_PI_2;
            euler[0] = -atan2f(_21,_22);
            euler[2] = 0.0f;
            return false;
        }
    }
    else
    {
        // y_angle = +pi/2
        // z_angle + x_angle = atan2(r10,r11)
        // WARNING.  The solutions is not unique.  Choosing z_angle = 0.
        euler[1] = (float)M_PI_2;
        euler[0] = atan2f(_21,_22);
        euler[2] = 0.0f;
    }
	return false;
}


void Matrix44::lookAt(Vector3& eye, Vector3& center, Vector3& up)
{
	Vector3 front = (center - eye);
	if (fabs(front.length()) <= 0.00001)
		return;

	front.normalize();
	Vector3 right = front.cross(up);
	right.normalize();
	Vector3 top = right.cross(front);
	top.normalize();

	setIdentity();

	M[0][0] = right.x; M[0][1] = top.x; M[0][2] = -front.x;
	M[1][0] = right.y; M[1][1] = top.y; M[1][2] = -front.y;
	M[2][0] = right.z; M[2][1] = top.z; M[2][2] = -front.z;

	translate(-eye.x, -eye.y, -eye.z);
}

//double check this functions
void Matrix44::perspective(float fov, float aspect, float near_plane, float far_plane)
{
	setIdentity();

	float f = 1.0f / tan(fov * float(DEG2RAD) * 0.5f);

	M[0][0] = f / aspect;
	M[1][1] = f;
	M[2][2] = (far_plane + near_plane) / (near_plane - far_plane);
	M[2][3] = -1.0f;
	M[3][2] = 2.0f * (far_plane * near_plane) / (near_plane - far_plane);
	M[3][3] = 0.0f;
}

void Matrix44::ortho(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	clear();
	M[0][0] = 2.0f / (right - left);
	M[3][0] = -(right + left) / (right - left);
	M[1][1] = 2.0f / (top - bottom);
	M[3][1] = -(top + bottom) / (top - bottom);
	M[2][2] = -2.0f / (far_plane - near_plane);
	M[3][2] = -(far_plane + near_plane) / (far_plane - near_plane);
	M[3][3] = 1.0f;
}

//applies matrix projection to vector (returns in normalized coordinates)
Vector3 Matrix44::project(const Vector3& v)
{
	float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
	float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
	float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
	float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];

	return Vector3((x / w + 1.0f) / 2.0f, (y / w + 1.0f) / 2.0f, (z / w + 1.0f) / 2.0f);
}


//Multiply a matrix by another and returns the result
Matrix44 Matrix44::operator*(const Matrix44& matrix) const
{
	Matrix44 ret;

	unsigned int i,j,k;
	for (i=0;i<4;i++) 	
	{
		for (j=0;j<4;j++) 
		{
			ret.M[i][j]=0.0;
			for (k=0;k<4;k++) 
				ret.M[i][j] += M[i][k] * matrix.M[k][j];
		}
	}

	return ret;
}

//Multiplies a vector by a matrix and returns the new vector
Vector3 operator * (const Matrix44& matrix, const Vector3& v) 
{   
   float x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + matrix.m[12]; 
   float y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + matrix.m[13]; 
   float z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + matrix.m[14];
   return Vector3(x,y,z);
}

//Multiplies a vector by a matrix and returns the new vector
Vector4 operator * (const Matrix44& matrix, const Vector4& v)
{
	float x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + v.w * matrix.m[12];
	float y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + v.w * matrix.m[13];
	float z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + v.w * matrix.m[14];
	float w = matrix.m[3] * v.x + matrix.m[7] * v.y + matrix.m[11] * v.z + v.w * matrix.m[15];
	return Vector4(x, y, z, w);
}

void Matrix44::setUpAndOrthonormalize(Vector3 up)
{
	up.normalize();

	//put the up vector in the matrix
	m[4] = up.x;
	m[5] = up.y;
	m[6] = up.z;

	//orthonormalize
	Vector3 right,front;
	right = rightVector();

	if ( abs(right.dot( up )) < 0.99998 )
	{
		right = up.cross( frontVector() );
		front = right.cross( up );
	}
	else
	{
		front = Vector3(right).cross( up );
		right = up.cross( front );
	}

	right.normalize();
	front.normalize();

	m[8] = front.x;
	m[9] = front.y;
	m[10] = front.z;

	m[0] = right.x;
	m[1] = right.y;
	m[2] = right.z;
}

void Matrix44::setFrontAndOrthonormalize(Vector3 front)
{
	front.normalize();

	//put the up vector in the matrix
	m[8] = front.x;
	m[9] = front.y;
	m[10] = front.z;

	//orthonormalize
	Vector3 right,up;
	right = rightVector();

	if ( abs(right.dot( front )) < 0.99998 )
	{
		right = topVector().cross( front  );
		up = front.cross( right );
	}
	else
	{
		up = front.cross( right );
		right = up.cross( front );
	}

	right.normalize();
	up.normalize();

	m[4] = up.x;
	m[5] = up.y;
	m[6] = up.z;

	m[0] = right.x;
	m[1] = right.y;
	m[2] = right.z;
	
}

bool Matrix44::inverse()
{
   unsigned int i, j, k, swap;
   float t;
   Matrix44 temp, final;
   final.setIdentity();

   temp = (*this);

   unsigned int m,n;
   m = n = 4;
	
   for (i = 0; i < m; i++)
   {
      // Look for largest element in column

      swap = i;
      for (j = i + 1; j < m; j++)// m or n
	  {
		 if ( fabs(temp.M[j][i]) > fabs( temp.M[swap][i]) )
            swap = j;
	  }
   
      if (swap != i)
      {
         // Swap rows.
         for (k = 0; k < n; k++)
         {
			 std::swap( temp.M[i][k],temp.M[swap][k]);
			 std::swap( final.M[i][k], final.M[swap][k]);
         }
      }

      // No non-zero pivot.  The CMatrix is singular, which shouldn't
      // happen.  This means the user gave us a bad CMatrix.


#define MATRIX_SINGULAR_THRESHOLD 0.00001 //change this if you experience problems with matrices

      if ( fabsf(temp.M[i][i]) <= MATRIX_SINGULAR_THRESHOLD)
	  {
		  final.setIdentity();
         return false;
	  }
#undef MATRIX_SINGULAR_THRESHOLD

      t = 1.0f/temp.M[i][i];

      for (k = 0; k < n; k++)//m or n
      {
         temp.M[i][k] *= t;
         final.M[i][k] *= t;
      }

      for (j = 0; j < m; j++) // m or n
      {
         if (j != i)
         {
            t = temp.M[j][i];
            for (k = 0; k < n; k++)//m or n
            {
               temp.M[j][k] -= (temp.M[i][k] * t);
               final.M[j][k] -= (final.M[i][k] * t);
            }
         }
      }
   }

   *this = final;

   return true;
}

#ifdef FIXEDPIPELINE
void Matrix44::multGL()
{
	glMultMatrixf(m);
}

void Matrix44::loadGL()
{
	glLoadMatrixf(m);
}
#endif


Quaternion::Quaternion()
{
}

Quaternion::Quaternion(const Quaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w)
{
}

Quaternion::Quaternion(const float X, const float Y, const float Z, const float W) : x(X), y(Y), z(Z), w(W)
{
}

Quaternion Quaternion::invert() const
{
	return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::conjugate() const
{
	return Quaternion(-x, -y, -z, w);
}

void Quaternion::set(const float X, const float Y, const float Z, const float W)
{
	x = X;  y = Y; z = Z; w = W;
}

Quaternion::Quaternion(const float* q)
{
	x = q[0];
	y = q[1];
	z = q[2];
	w = q[3];
}

void Quaternion::identity()
{
	x = y = z = 0.0f; w = 1.0f;
}

void Quaternion::slerp(const Quaternion& b, float t)
{
	//quaternion a(*this);
	*this = Qslerp(*this, b, t);
}

void Quaternion::slerp(const Quaternion& q2, float t, Quaternion &q3) const
{
	q3 = Qslerp(*this, q2, t);
}

void Quaternion::lerp(const Quaternion& b, float t)
{
	*this = Qlerp(*this, b, t);
}

void Quaternion::lerp(const Quaternion& q2, float t, Quaternion &q3) const
{
	q3 = Qlerp(*this, q2, t);
}

void Quaternion::setAxisAngle(const Vector3& axis, const float angle)
{
	float s;
	s = sinf(angle * 0.5f);

	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = cosf(angle * 0.5f);
}

Quaternion::Quaternion(const Vector3& axis, float angle)
{
	setAxisAngle(axis, angle);
}

void Quaternion::operator *= (const Quaternion &q)
{
	Quaternion quaternion = *this * q;
	*this = quaternion;
}

void Quaternion::operator*=(const Vector3& v)
{
	*this = *this*v;
}

void Quaternion::operator += (const Quaternion &q)
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
}

Quaternion operator * (const Quaternion& q1, const Quaternion& q2)
{
	Quaternion q;

	q.x = q1.y*q2.z - q1.z*q2.y + q1.w*q2.x + q1.x*q2.w;
	q.y = q1.z*q2.x - q1.x*q2.z + q1.w*q2.y + q1.y*q2.w;
	q.z = q1.x*q2.y - q1.y*q2.x + q1.w*q2.z + q1.z*q2.w;
	q.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
	return q;
}

/*
//http://www.cs.yorku.ca/~arlene/aquasim/mymath.c
quaternion operator * (const quaternion& p, const quaternion& q)
{
return quaternion(
p.w*q.x + p.x*q.w + p.y*q.z - p.z*q.y,
p.w*q.y + p.y*q.w + p.z*q.x - p.x*q.z,
p.w*q.z + p.z*q.w + p.x*q.y - p.y*q.x,
p.w*q.w - p.x*q.x - p.y*q.y - p.z*q.z);
}
*/

Quaternion operator * (const Quaternion &q, const Vector3& v)
{
	return Quaternion
	(
		q.w*v.x + q.y*v.z - q.z*v.y,
		q.w*v.y + q.z*v.x - q.x*v.z,
		q.w*v.z + q.x*v.y - q.y*v.x,
		-(q.x*v.x + q.y*v.y + q.z*v.z)
	);
}

void Quaternion::operator *= (float f)
{
	x *= f;
	y *= f;
	z *= f;
	w *= f;
}

Quaternion operator * (const Quaternion &q, float f)
{
	Quaternion q1;
	q1.x = q.x*f;
	q1.y = q.y*f;
	q1.z = q.z*f;
	q1.w = q.w*f;
	return q1;
}

Quaternion operator * (float f, const Quaternion &q)
{
	Quaternion q1;
	q1.x = q.x*f;
	q1.y = q.y*f;
	q1.z = q.z*f;
	q1.w = q.w*f;
	return q1;
}

void Quaternion::normalize()
{
	float len = length();
	len = 1.0f / len;

	x *= len;
	y *= len;
	z *= len;
	w *= len;
}

void Quaternion::computeMinimumRotation(const Vector3& rotateFrom, const Vector3& rotateTo)
{
	// Check if the vectors are valid.
	//rotateFrom.GetLength()==0.0f
	//rotateTo.GetLength()==0.0f

	Vector3 from(rotateFrom);
	from.normalize();
	Vector3 to(rotateTo);
	to.normalize();

	const float _dot = dot(from, to);
	Vector3 crossvec = cross(from, to);
	const float crosslen = (float)crossvec.length();

	if (crosslen == 0.0f)
	{
		// Parallel vectors
		// Check if they are pointing in the same direction.
		if (_dot > 0.0f)
		{
			x = y = z = 0.0f;
			w = 1.0f;
		}
		// Ok, so they are parallel and pointing in the opposite direction
		// of each other.
		else
		{
			// Try crossing with x axis.
			Vector3 t = cross(from, Vector3(1.0f, 0.0f, 0.0f));
			// If not ok, cross with y axis.
			if (t.length() == 0.0f)
				cross(from, Vector3(0.0f, 1.0f, 0.0f));

			t.normalize();
			x = t[0];
			y = t[1];
			z = t[2];
			w = 0.0f;
		}
	}
	else
	{ // Vectors are not parallel
		crossvec.normalize();
		// The fabs() wrapping is to avoid problems when `dot' "overflows"
		// a tiny wee bit, which can lead to sqrt() returning NaN.
		crossvec *= (float)sqrt(0.5f * fabs(1.0f - _dot));
		// The fabs() wrapping is to avoid problems when `dot' "underflows"
		// a tiny wee bit, which can lead to sqrt() returning NaN.
		x = crossvec[0];
		y = crossvec[1];
		z = crossvec[2],
			w = (float)sqrt(0.5 * fabs(1.0 + _dot));
	}
}

Quaternion SimpleRotation(const Vector3 &a, const Vector3 &b)
{
	Vector3 axis = cross(a, b);
	float _dot = dot(a, b);
	if (_dot < -1.0f + /*DOT_EPSILON*/0.001f) return Quaternion(0, 1, 0, 0);

	Quaternion result(axis.x * 0.5f, axis.y * 0.5f, axis.z * 0.5f, (_dot + 1.0f) * 0.5f);
	result.normalize();
	//fast_normalize(&result);

	return result;
}


float Quaternion::length() const
{
	return sqrtf(w * w + x * x + y * y + z * z);
}

float Quaternion::squaredLength() const
{
	return w * w + x * x + y * y + z * z;
}

void Quaternion::toMatrix(Matrix44& matrix) const
{
	//from glmatrix
	float r = x;
	float e = z;
	float a = y;
	float u = w;
	float o = r + r;
	float i = a + a;
	float s = e + e;
	float c = r * o;
	float f = a * o;
	float M = a * i;
	float h = e * o;
	float l = e * i;
	float v = e * s;
	float d = u * o;
	float b = u * i;
	float m = u * s;
	matrix.m[0] = 1 - M - v;
	matrix.m[1] = f + m;
	matrix.m[2] = h - b,
	matrix.m[3] = 0;
	matrix.m[4] = f - m;
	matrix.m[5] = 1 - c - v;
	matrix.m[6] = l + d;
	matrix.m[7] = 0;
	matrix.m[8] = h + b;
	matrix.m[9] = l - d;
	matrix.m[10] = 1 - c - M;
	matrix.m[11] = 0;
	matrix.m[12] = 0;
	matrix.m[13] = 0;
	matrix.m[14] = 0;
	matrix.m[15] = 1;
	return;



	/*
	If q is guaranteed to be a unit quaternion, s will always
	be 2.  In that case, this calculation can be optimized out.
	*/
	//const float	norm = x*x + y*y + z*z + w*w;
	//const float s = (norm > 0) ? 2/norm : 0;

	/*
	const float s = 2;

	//Precalculate coordinate products
	const float xx = x * x * s;
	const float yy = y * y * s;
	const float zz = z * z * s;
	const float xy = x * y * s;
	const float xz = x * z * s;
	const float yz = y * z * s;
	const float wx = w * x * s;
	const float wy = w * y * s;
	const float wz = w * z * s;

	//Calculate 3x3 matrix from orthonormal basis

	//	x axis
	// >>
	matrix.M[0][0] = 1.0f - (yy + zz);
	//matrix.M[0][0] = x*x-y*y-z*z+w*w;
	// <<
	matrix.M[1][0] = xy + wz;
	matrix.M[2][0] = xz - wy;

	//	y axis
	matrix.M[0][1] = xy - wz;
	// >>
	matrix.M[1][1] = 1.0f - (xx + zz);
	//matrix.M[1][1] = -x*x+y*y-z*z+w*w;
	// <<
	matrix.M[2][1] = yz + wx;

	//	z axis
	matrix.M[0][2] = xz + wy;
	matrix.M[1][2] = yz - wx;
	matrix.M[2][2] = 1.0f - (xx + yy);

	matrix._14 = matrix._24 = matrix._34 = 0;
	matrix._41 = matrix._42 = matrix._43 = 0;
	matrix._44 = 1;
	*/
}

Vector3 transformQuat(const Vector3& a, const Quaternion& q)
{
	// benchmarks: https://jsperf.com/quaternion-transform-vec3-implementations-fixed
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float x = a.x, y = a.y, z = a.z;
	// var qvec = [qx, qy, qz];
	// var uv = vec3.cross([], qvec, a);
	float uvx = qy * z - qz * y,
		uvy = qz * x - qx * z,
		uvz = qx * y - qy * x;
	// var uuv = vec3.cross([], qvec, uv);
	float uuvx = qy * uvz - qz * uvy,
		uuvy = qz * uvx - qx * uvz,
		uuvz = qx * uvy - qy * uvx;
	// vec3.scale(uv, uv, 2 * w);
	float w2 = qw * 2;
	uvx *= w2;
	uvy *= w2;
	uvz *= w2;
	// vec3.scale(uuv, uuv, 2);
	uuvx *= 2;
	uuvy *= 2;
	uuvz *= 2;
	// return vec3.add(out, a, vec3.add(out, uv, uuv));

	Vector3 out;

	out[0] = x + uvx + uuvx;
	out[1] = y + uvy + uuvy;
	out[2] = z + uvz + uuvz;

	return out;
}

float DotProduct(const Quaternion& q1, const Quaternion& q2)
{
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

bool operator==(const Quaternion& q1, const Quaternion& q2)
{
	return ((q1.x == q2.x) && (q1.y == q2.y) &&
		(q1.z == q2.z) && (q1.w == q2.w));
}

bool operator!=(const Quaternion& q1, const Quaternion& q2)
{
	return ((q1.x != q2.x) || (q1.y != q2.y) ||
		(q1.z != q2.z) || (q1.w != q2.w));
}

/*
Logarithm of a quaternion, given as:
Qlog(q) = v*a where q = [cos(a),v*sin(a)]
*/
Quaternion Qlog(const Quaternion &q)
{
	float a = static_cast<float>(acos(q.w));
	float sina = static_cast<float>(sin(a));
	Quaternion ret;
	ret.w = 0;
	if (sina > 0)
	{
		ret.x = a*q.x / sina;
		ret.y = a*q.y / sina;
		ret.z = a*q.z / sina;
	}
	else
	{
		ret.x = ret.y = ret.z = 0;
	}
	return ret;
}

/*
e^quaternion given as:
Qexp(v*a) = [cos(a),vsin(a)]
*/
Quaternion Qexp(const Quaternion &q)
{
	float a = static_cast<float>(sqrt(q.x*q.x + q.y*q.y + q.z*q.z));
	float sina = static_cast<float>(sin(a));
	float cosa = static_cast<float>(cos(a));
	Quaternion ret;

	ret.w = cosa;
	if (a > 0)
	{
		ret.x = sina * q.x / a;
		ret.y = sina * q.y / a;
		ret.z = sina * q.z / a;
	}
	else
	{
		ret.x = ret.y = ret.z = 0;
	}

	return ret;
}

Quaternion operator + (const Quaternion &q1, const Quaternion& q2)
{
	return Quaternion(q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w);
}

/*
Linear interpolation between two quaternions
*/
Quaternion Qlerp(const Quaternion &q1, const Quaternion &q2, float t)
{
	Quaternion ret;
	//ret = q1 + t*(q2-q1);

	Quaternion b;
	if (DotProduct(q1, q2)< 0.0f)
		b.set(-q2.x, -q2.y, -q2.z, -q2.w);
	else b = q2;

	ret = q1*(1 - t) + b*t;

	ret.normalize();
	return ret;
}

Quaternion Qslerp(const Quaternion &q1, const Quaternion &q2, float t)
{
	Quaternion q3;
	float dot = DotProduct(q1, q2);

	//dot = cos(theta)
	//if (dot < 0), q1 and q2 are more than 90 degrees apart,
	//so we can invert one to reduce spinning

	if (dot < 0)
	{
		dot = -dot;
		q3.set(-q2.x, -q2.y, -q2.z, -q2.w);
	}
	else
	{
		q3 = q2;
	}

	if (dot < 0.95f)
	{
		float angle = static_cast<float>(acosf(dot));
		float sina, sinat, sinaomt;
		sina = sinf(angle);
		sinat = sinf(angle*t);
		sinaomt = sinf(angle*(1 - t));
		return (q1*sinaomt + q3*sinat) * (1.0f / sina);
	}

	//if the angle is small, use linear interpolation

	else
	{
		return Qlerp(q1, q3, t);
	}



	/*
	float fCos = DotProduct(q1,q2);
	if (fCos > 1)
	fCos = 1;
	if (fCos < -1)
	fCos = -1;
	float fAngle = acosf(fCos);
	if ( fabs(fAngle) < 0.0001 )
	{
	return q1;
	}
	float fSin = sinf(fAngle);
	float fInvSin = 1.0f/fSin;
	float fCoeff0 = sinf((1.0f-fT)*fAngle)*fInvSin;
	float fCoeff1 = sinf(fT*fAngle)*fInvSin;
	return fCoeff0*q1 + fCoeff1*q2;
	*/
}


/*
Given 3 quaternions, qn-1,qn and qn+1, calculate a control point to be used in spline interpolation
*/

Quaternion& Quaternion::operator -()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;

	return *this;
}

void Quaternion::getAxisAngle(Vector3 &v, float &angle) const
{
	angle = 2.0f * acosf(w);
	float factor = 1.0f / sqrtf(1 - w*w);
	v.x = x * factor;
	v.y = y * factor;
	v.z = z * factor;
}

Vector3 Quaternion::rotate(const Vector3& v) const
{
	Vector3 ret;
	Quaternion temp(-x, -y, -z, w);
	temp *= v;
	temp *= (*this);

	ret.x = temp.x;
	ret.y = temp.y;
	ret.z = temp.z;

	return ret;
}

void Quaternion::toEulerAngles(Vector3 &euler) const
{
	/// Local Variables ///////////////////////////////////////////////////////////
	float matrix[3][3];
	float cx, sx;
	float cy, sy;
	float cz, sz;

	const float y2 = y*y;
	///////////////////////////////////////////////////////////////////////////////
	// CONVERT QUATERNION TO MATRIX - I DON'T REALLY NEED ALL OF IT
	matrix[0][0] = 1.0f - (2.0f * y2) - (2.0f * z * z);
	//	matrix[0][1] = (2.0f * quat->x * quat->y) - (2.0f * quat->w * quat->z);
	//	matrix[0][2] = (2.0f * quat->x * quat->z) + (2.0f * quat->w * quat->y);
	matrix[1][0] = (2.0f * x * y) + (2.0f * w * z);
	//	matrix[1][1] = 1.0f - (2.0f * quat->x * quat->x) - (2.0f * quat->z * quat->z);
	//	matrix[1][2] = (2.0f * quat->y * quat->z) - (2.0f * quat->w * quat->x);
	matrix[2][0] = (2.0f * x * z) - (2.0f * w * y);
	matrix[2][1] = (2.0f * y * z) + (2.0f * w * x);
	matrix[2][2] = 1.0f - (2.0f * x * x) - (2.0f * y2);

	sy = -matrix[2][0];
	cy = sqrtf(1 - (sy * sy));
	euler.y = atan2f(sy, cy);
	//euler->y = (yr * 180.0f) / (float)M_PI;

	// AVOID DIVIDE BY ZERO ERROR ONLY WHERE Y= +-90 or +-270 
	// NOT CHECKING cy BECAUSE OF PRECISION ERRORS
	if (sy != 1.0f && sy != -1.0f)
	{
		cx = matrix[2][2] / cy;
		sx = matrix[2][1] / cy;
		//euler->x = ((float)atan2(sx,cx) * 180.0f) / (float)M_PI;	// RAD TO DEG
		euler.x = atan2f(sx, cx);

		cz = matrix[0][0] / cy;
		sz = matrix[1][0] / cy;
		//euler->z = ((float)atan2(sz,cz) * 180.0f) / (float)M_PI;	// RAD TO DEG
		euler.z = atan2f(sz, cz);
	}
	else
	{
		// SINCE Cos(Y) IS 0, I AM SCREWED.  ADOPT THE STANDARD Z = 0
		// I THINK THERE IS A WAY TO FIX THIS BUT I AM NOT SURE.  EULERS SUCK
		// NEED SOME MORE OF THE MATRIX TERMS NOW
		matrix[1][1] = 1.0f - (2.0f * x * x) - (2.0f * z * z);
		matrix[1][2] = (2.0f * y * z) - (2.0f * w * x);
		cx = matrix[1][1];
		sx = -matrix[1][2];
		//euler->x = ((float)atan2(sx,cx) * 180.0f) / (float)M_PI;	// RAD TO DEG
		euler.x = atan2f(sx, cx);

		cz = 1.0f;
		sz = 0.0f;
		//euler->z = ((float)atan2(sz,cz) * 180.0f) / (float)M_PI;	// RAD TO DEG
		euler.z = atan2f(sz, cz);
	}
}


void Quaternion::setAxisAngle(float ax, float ay, float az, float angle)
{
	const float halfAngle = angle * 0.5f;
	const float s = sinf(halfAngle);

	w = cosf(halfAngle);
	x = ax * s;
	y = ay * s;
	z = az * s;

}

float ComputeSignedAngle(Vector2 a, Vector2 b)
{
	a.normalize();
	b.normalize();
	return atan2(a.perpdot(b), a.dot(b));
}

bool RayPlaneCollision(const Vector3& plane_pos, const Vector3& plane_normal, const Vector3& ray_origin, const Vector3& ray_dir, Vector3& result)
{
	double D = plane_pos.dot(plane_normal);
	double numer = D - plane_normal.dot(ray_origin);
	double denom = plane_normal.dot(ray_dir);
	if (abs(denom) < 0.00001)//parallel
		return false;
	double t = (numer / denom);
	if (t < 0)//back
		return false;
	result = ray_origin + ray_dir * t;
	return true;
}

Vector3 normalize(Vector3 n)
{
	return n.normalize();
}

int planeBoxOverlap( const Vector4& plane, const Vector3& center, const Vector3& halfsize )
{
	Vector3 n = plane.xyz();
	float d = plane.w;
	float radius = abs(halfsize.x * n[0]) + abs(halfsize.y * n[1]) + abs(halfsize.z * n[2]);
	float distance = dot(n, center) + d;
	if (distance <= -radius)
		return CLIP_OUTSIDE;
	else if (distance <= radius)
		return CLIP_OVERLAP;
	return CLIP_INSIDE;
}

float signedDistanceToPlane( const Vector4& plane, const Vector3& point )
{
	return dot(plane.xyz(), point) + plane.w;
}

const Vector3 corners[] = { {1,1,1},  {1,1,-1},  {1,-1,1},  {1,-1,-1},  {-1,1,1},  {-1,1,-1},  {-1,-1,1},  {-1,-1,-1} };

BoundingBox transformBoundingBox(const Matrix44 m, const BoundingBox& box)
{
	Vector3 box_min(10000000.0f,1000000.0f, 1000000.0f);
	Vector3 box_max(-10000000.0f, -1000000.0f, -1000000.0f);

	for (int i = 0; i < 8; ++i)
	{
		Vector3 corner = corners[i];
		corner = box.halfsize * corner;
		corner = corner + box.center;
		corner = m * corner;
		box_min.setMin(corner);
		box_max.setMax(corner);
	}

	Vector3 halfsize = (box_max - box_min) * 0.5;
	return BoundingBox(box_max - halfsize, halfsize );
}

BoundingBox mergeBoundingBoxes(const BoundingBox& a, const BoundingBox& b)
{
	BoundingBox result;
	Vector3 bbmin = (a.center - a.halfsize);
	Vector3 bbmax = (a.center + a.halfsize);
	bbmin.setMin(b.center - b.halfsize);
	bbmax.setMax(b.center + b.halfsize);
	result.center = (bbmax + bbmin) * 0.5;
	result.halfsize = bbmax - result.center;
	return result;
}

//from https://github.com/erich666/GraphicsGems/blob/master/gems/RayBox.c
bool RayBoundingBoxCollision(const BoundingBox& box, const Vector3& ray_origin, const Vector3& ray_dir, Vector3& coll)
{
	const int NUMDIM = 3;
	const int RIGHT = 0;
	const int LEFT = 1;
	const int MIDDLE = 2;

	char inside = true;
	char quadrant[NUMDIM];
	int i;
	int whichPlane;
	double maxT[NUMDIM];
	double candidatePlane[NUMDIM];
	Vector3 minB = box.center - box.halfsize;
	Vector3 maxB = box.center + box.halfsize;

	/* Find candidate planes; this loop can be avoided if
	rays cast all from the eye(assume perpsective view) */
	for (i = 0; i < NUMDIM; i++)
		if (ray_origin.v[i] < minB[i]) {
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			inside = false;
		}
		else if (ray_origin.v[i] > maxB[i]) {
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			inside = false;
		}
		else {
			quadrant[i] = MIDDLE;
		}

	/* Ray origin inside bounding box */
	if (inside) {
		coll = ray_origin;
		return (true);
	}


	/* Calculate T distances to candidate planes */
	for (i = 0; i < NUMDIM; i++)
		if (quadrant[i] != MIDDLE && ray_dir.v[i] != 0.)
			maxT[i] = (candidatePlane[i] - ray_origin.v[i]) / ray_dir.v[i];
		else
			maxT[i] = -1.;

	/* Get largest of the maxT's for final choice of intersection */
	whichPlane = 0;
	for (i = 1; i < NUMDIM; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	/* Check final candidate actually inside box */
	if (maxT[whichPlane] < 0.) return (false);
	for (i = 0; i < NUMDIM; i++)
		if (whichPlane != i) {
			coll.v[i] = ray_origin.v[i] + maxT[whichPlane] * ray_dir.v[i];
			if (coll.v[i] < minB[i] || coll[i] > maxB[i])
				return (false);
		}
		else {
			coll.v[i] = candidatePlane[i];
		}
	return (true);				/* ray hits box */
}

bool BoundingBoxSphereOverlap(const BoundingBox& box, const Vector3& center, float radius)
{
	// arvo's algorithm from gamasutra
	// http://www.gamasutra.com/features/19991018/Gomez_4.htm

	float s, d = 0.0;
	//find the square of the distance
	//from the sphere to the box
	Vector3 vmin = box.center - box.halfsize;
	Vector3 vmax = box.center + box.halfsize;
	for (int i = 0; i < 3; ++i)
	{
		if (center.v[i] < vmin.v[i])
		{
			s = center.v[i] - vmin.v[i];
			d += s * s;
		}
		else if (center.v[i] > vmax.v[i])
		{
			s = center.v[i] - vmax.v[i];
			d += s * s;
		}
	}
	//return d <= r*r

	float radiusSquared = radius * radius;
	if (d <= radiusSquared)
	{
		return true; //overlaps
		/*
		// this is used just to know if it overlaps or is just inside, but I dont care
		// make an aabb aabb test with the sphere aabb to test inside state
		var halfsize = vec3.fromValues( radius, radius, radius );
		var sphere_bbox = BBox.fromCenterHalfsize( center, halfsize );
		if ( geo.testBBoxBBox(bbox, sphere_bbox) )
			return INSIDE;
		return OVERLAP;
		*/
	}

	return false; //OUTSIDE;
}