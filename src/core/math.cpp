#include "math.h"

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


template<typename T> Vector2<T> normalize(const Vector2<T>& n)
{
	Vector2<T> v = n;
	v.normalize();
	return v;
}

template<typename T> void Vector2<T>::random(T range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (T)(rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
	y = (T)(rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
}

template<typename T> void Vector2<T>::parseFromText(const char* text)
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
				case 0: x = (T)atof(num); break;
				case 1: y = (T)atof(num); break;
				default: return; break;
			}
			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
}


// **************************************

template<typename T> T Vector3<T>::length() 
{
	return sqrt(x*x + y*y + z*z);
}

template<typename T> T Vector3<T>::length() const
{
	return sqrt(x*x + y*y + z*z);
}

template<typename T> Vector3<T>& Vector3<T>::normalize()
{
	T len = length();
	assert(len > 0.00000000001 && "Cannot normalize a vector with module 0");
	x = (T)(x / len);
	y = (T)(y / len);
	z = (T)(z / len);
	return *this;
}


template<typename T> Vector3<T> Vector3<T>::cross( const Vector3<T>& b ) const
{
	return Vector3<T>(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
}

template<typename T> T Vector3<T>::dot( const Vector3<T>& v ) const
{
	return x*v.x + y*v.y + z*v.z;
}

template<typename T> void Vector3<T>::random(T range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (float)((rand() / (double)RAND_MAX) * 2 * range - range); //value between -range and range
	y = (float)((rand() / (double)RAND_MAX) * 2 * range - range); //value between -range and range
	z = (float)((rand() / (double)RAND_MAX) * 2 * range - range); //value between -range and range
}

template<typename T> void Vector3<T>::random(const Vector3<T>& range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (T)((rand() / (T)RAND_MAX) * 2 * range.x - range.x); //value between -range and range
	y = (T)((rand() / (T)RAND_MAX) * 2 * range.y - range.y); //value between -range and range
	z = (T)((rand() / (T)RAND_MAX) * 2 * range.z - range.z); //value between -range and range
}

template<typename T> void Vector3<T>::setMin(const Vector3<T> & v)
{
	if (v.x < x) x = v.x;
	if (v.y < y) y = v.y;
	if (v.z < z) z = v.z;
}

template<typename T> void Vector3<T>::setMax(const Vector3<T> & v)
{
	if (v.x > x) x = v.x;
	if (v.y > y) y = v.y;
	if (v.z > z) z = v.z;
}

template<typename T> void Vector3<T>::parseFromText(const char* text, const char separator)
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
					case 0: x = (T)atof(num); break;
					case 1: y = (T)atof(num); break;
					case 2: z = (T)atof(num); break;
					default: return; break;
				}

			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
};

template void Vector3<float>::parseFromText(const char* text, const char separator);

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

Vector3f Matrix44::rotateVector(const Vector3f& v) const
{
	return (*this * Vector4f(v,0.0)).xyz();
}

void Matrix44::translateGlobal(float x, float y, float z)
{
	Matrix44 T;
	T.setTranslation(x, y, z);
	*this = *this * T;
}

void Matrix44::rotateGlobal( float angle_in_rad, const Vector3f& axis )
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

void Matrix44::rotate( float angle_in_rad, const Vector3f& axis )
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

Vector3f Matrix44::getTranslation()
{
	return Vector3f(m[12],m[13],m[14]);
}

//To create a rotation matrix
void Matrix44::setRotation( float angle_in_rad, const Vector3f& axis  )
{
	clear();
	Vector3f axis_n = axis;
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

//not tested
Vector3f Matrix44::getScale()
{
	return Vector3f((float)Vector3f(m[0], m[1], m[2]).length(),
		(float)Vector3f(m[4], m[5], m[6]).length(),
		(float)Vector3f(m[8], m[9], m[10]).length() );
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


void Matrix44::lookAt(Vector3f& eye, Vector3f& center, Vector3f& up)
{
	Vector3f front = (center - eye);
	if (fabs(front.length()) <= 0.00001)
		return;

	front.normalize();
	Vector3f right = front.cross(up);
	right.normalize();
	Vector3f top = right.cross(front);
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
Vector3f Matrix44::project(const Vector3f& v)
{
	float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
	float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
	float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
	float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];

	return Vector3f((x / w + 1.0f) / 2.0f, (y / w + 1.0f) / 2.0f, (z / w + 1.0f) / 2.0f);
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
Vector3f operator * (const Matrix44& matrix, const Vector3f& v) 
{   
   float x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + matrix.m[12]; 
   float y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + matrix.m[13]; 
   float z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + matrix.m[14];
   return Vector3f(x,y,z);
}

//Multiplies a vector by a matrix and returns the new vector
Vector4f operator * (const Matrix44& matrix, const Vector4f& v)
{
	float x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + v.w * matrix.m[12];
	float y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + v.w * matrix.m[13];
	float z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + v.w * matrix.m[14];
	float w = matrix.m[3] * v.x + matrix.m[7] * v.y + matrix.m[11] * v.z + v.w * matrix.m[15];
	return Vector4f(x, y, z, w);
}

void Matrix44::setUpAndOrthonormalize(Vector3f up)
{
	up.normalize();

	//put the up vector in the matrix
	m[4] = up.x;
	m[5] = up.y;
	m[6] = up.z;

	//orthonormalize
	Vector3f right,front;
	right = rightVector();

	if ( abs(right.dot( up )) < 0.99998 )
	{
		right = up.cross( frontVector() );
		front = right.cross( up );
	}
	else
	{
		front = Vector3f(right).cross( up );
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

void Matrix44::setFrontAndOrthonormalize(Vector3f front)
{
	front.normalize();

	//put the up vector in the matrix
	m[8] = front.x;
	m[9] = front.y;
	m[10] = front.z;

	//orthonormalize
	Vector3f right,up;
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
	// http://www.geometrictools.com/LibFoundation/Mathematics/Wm4Matrix4.inl
	double A0 = m[0] * m[5] - m[1] * m[4];
	double A1 = m[0] * m[6] - m[2] * m[4];
	double A2 = m[0] * m[7] - m[3] * m[4];
	double A3 = m[1] * m[6] - m[2] * m[5];
	double A4 = m[1] * m[7] - m[3] * m[5];
	double A5 = m[2] * m[7] - m[3] * m[6];
	double B0 = m[8] * m[13] - m[9] * m[12];
	double B1 = m[8] * m[14] - m[10] * m[12];
	double B2 = m[8] * m[15] - m[11] * m[12];
	double B3 = m[9] * m[14] - m[10] * m[13];
	double B4 = m[9] * m[15] - m[11] * m[13];
	double B5 = m[10] * m[15] - m[11] * m[14];
	double det = A0 * B5 - A1 * B4 + A2 * B3 + A3 * B2 - A4 * B1 + A5 * B0;

	// std::numeric_limits<T>::epsilon() does not work with orthographic matrix with znear/far 0.1, 2000
	// 1e-10 does not work with ortographic matrix znear/zfar -2000, 2000
	auto threshold = (double)1e-11;
	if (std::abs(det) <= threshold)
	{
		setIdentity();
		return false;
	}

	auto rdet = double(1) / det;

	Matrix44 tmp;
	tmp.M[0][0] = +m[5] * B5 - m[6] * B4 + m[7] * B3;
	tmp.M[1][0] = -m[4] * B5 + m[6] * B2 - m[7] * B1;
	tmp.M[2][0] = +m[4] * B4 - m[5] * B2 + m[7] * B0;
	tmp.M[3][0] = -m[4] * B3 + m[5] * B1 - m[6] * B0;
	tmp.M[0][1] = -m[1] * B5 + m[2] * B4 - m[3] * B3;
	tmp.M[1][1] = +m[0] * B5 - m[2] * B2 + m[3] * B1;
	tmp.M[2][1] = -m[0] * B4 + m[1] * B2 - m[3] * B0;
	tmp.M[3][1] = +m[0] * B3 - m[1] * B1 + m[2] * B0;
	tmp.M[0][2] = +m[13] * A5 - m[14] * A4 + m[15] * A3;
	tmp.M[1][2] = -m[12] * A5 + m[14] * A2 - m[15] * A1;
	tmp.M[2][2] = +m[12] * A4 - m[13] * A2 + m[15] * A0;
	tmp.M[3][2] = -m[12] * A3 + m[13] * A1 - m[14] * A0;
	tmp.M[0][3] = -m[9] * A5 + m[10] * A4 - m[11] * A3;
	tmp.M[1][3] = +m[8] * A5 - m[10] * A2 + m[11] * A1;
	tmp.M[2][3] = -m[8] * A4 + m[9] * A2 - m[11] * A0;
	tmp.M[3][3] = +m[8] * A3 - m[9] * A1 + m[10] * A0;
	m[0] = tmp.m[0] * rdet;
	m[1] = tmp.m[1] * rdet;
	m[2] = tmp.m[2] * rdet;
	m[3] = tmp.m[3] * rdet;
	m[4] = tmp.m[4] * rdet;
	m[5] = tmp.m[5] * rdet;
	m[6] = tmp.m[6] * rdet;
	m[7] = tmp.m[7] * rdet;
	m[8] = tmp.m[8] * rdet;
	m[9] = tmp.m[9] * rdet;
	m[10] = tmp.m[10] * rdet;
	m[11] = tmp.m[11] * rdet;
	m[12] = tmp.m[12] * rdet;
	m[13] = tmp.m[13] * rdet;
	m[14] = tmp.m[14] * rdet;
	m[15] = tmp.m[15] * rdet;
	return true;
}

Quaternion::Quaternion()
{
	x = y = z = 0.0f; w = 1.0f;
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

void Quaternion::setAxisAngle(const Vector3f& axis, const float angle)
{
	float s;
	s = sinf(angle * 0.5f);

	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = cosf(angle * 0.5f);
}

Quaternion::Quaternion(const Vector3f& axis, float angle)
{
	setAxisAngle(axis, angle);
}

void Quaternion::operator *= (const Quaternion &q)
{
	Quaternion quaternion = *this * q;
	*this = quaternion;
}

void Quaternion::operator*=(const Vector3f& v)
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

Quaternion operator * (const Quaternion &q, const Vector3f& v)
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

void Quaternion::computeMinimumRotation(const Vector3f& rotateFrom, const Vector3f& rotateTo)
{
	// Check if the vectors are valid.
	//rotateFrom.GetLength()==0.0f
	//rotateTo.GetLength()==0.0f

	Vector3f from(rotateFrom);
	from.normalize();
	Vector3f to(rotateTo);
	to.normalize();

	const float _dot = dot(from, to);
	Vector3f crossvec = cross(from, to);
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
			Vector3f t = cross(from, Vector3f(1.0f, 0.0f, 0.0f));
			// If not ok, cross with y axis.
			if (t.length() == 0.0f)
				cross(from, Vector3f(0.0f, 1.0f, 0.0f));

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

Quaternion SimpleRotation(const Vector3f &a, const Vector3f &b)
{
	Vector3f axis = cross(a, b);
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
	/*
	//from https://glmatrix.net/docs/mat4.js.html#line1420
	float* out = matrix.m;
	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2;
	float yx = y * x2;
	float yy = y * y2;
	float zx = z * x2;
	float zy = z * y2;
	float zz = z * z2;
	float wx = w * x2;
	float wy = w * y2;
	float wz = w * z2;
	out[0] = 1 - yy - zz;
	out[1] = yx + wz;
	out[2] = zx - wy;
	out[3] = 0;
	out[4] = yx - wz;
	out[5] = 1 - xx - zz;
	out[6] = zy + wx;
	out[7] = 0;
	out[8] = zx + wy;
	out[9] = zy - wx;
	out[10] = 1 - xx - yy;
	out[11] = 0;
	out[12] = 0;
	out[13] = 0;
	out[14] = 0;
	out[15] = 1;
	//*/

	//from glmatrix
	//*
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
	//*/

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
	//*/
}

void Quaternion::fromMatrix(Matrix44& matrix)
{
	//from https://raw.githubusercontent.com/mrdoob/three.js/dev/src/math/Quaternion.js
	float* te = matrix.m,
	m11 = te[0], m12 = te[4], m13 = te[8],
	m21 = te[1], m22 = te[5], m23 = te[9],
	m31 = te[2], m32 = te[6], m33 = te[10],
	trace = m11 + m22 + m33;

	if (trace > 0) {

		float s = 0.5f / sqrtf(trace + 1.0f);

		this->w = 0.25f / s;
		this->x = (m32 - m23) * s;
		this->y = (m13 - m31) * s;
		this->z = (m21 - m12) * s;

	}
	else if (m11 > m22 && m11 > m33) {

		float s = 2.0f * sqrt(1.0f + m11 - m22 - m33);

		this->w = (m32 - m23) / s;
		this->x = 0.25f * s;
		this->y = (m12 + m21) / s;
		this->z = (m13 + m31) / s;

	}
	else if (m22 > m33) {

		float s = 2.0f * sqrt(1.0f + m22 - m11 - m33);

		this->w = (m13 - m31) / s;
		this->x = (m12 + m21) / s;
		this->y = 0.25f * s;
		this->z = (m23 + m32) / s;

	}
	else {

		float s = 2.0f * sqrt(1.0f + m33 - m11 - m22);

		this->w = (m21 - m12) / s;
		this->x = (m13 + m31) / s;
		this->y = (m23 + m32) / s;
		this->z = 0.25f * s;
	}

	normalize();
	//from https://glmatrix.net/docs/quat.js.html#line416

	/*
	float* m = matrix.m;
	float* out = this->q;
 // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
  // article "Quaternion Calculus and Fast Animation".
	float fTrace = m[0] + m[4] + m[8];
	float fRoot;
	if (fTrace > 0.0) {
		// |w| > 1/2, may as well choose w > 1/2
		fRoot = sqrt(fTrace + 1.0); // 2w
		out[3] = 0.5 * fRoot;
		fRoot = 0.5 / fRoot; // 1/(4w)
		out[0] = (m[5] - m[7]) * fRoot;
		out[1] = (m[6] - m[2]) * fRoot;
		out[2] = (m[1] - m[3]) * fRoot;
	}
	else {
		// |w| <= 1/2
		int i = 0;
		if (m[4] > m[0]) i = 1;
		if (m[8] > m[i * 3 + i]) i = 2;
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;
		fRoot = sqrt(m[i * 3 + i] - m[j * 3 + j] - m[k * 3 + k] + 1.0);
		out[i] = 0.5 * fRoot;
		fRoot = 0.5 / fRoot;
		out[3] = (m[j * 3 + k] - m[k * 3 + j]) * fRoot;
		out[j] = (m[j * 3 + i] + m[i * 3 + j]) * fRoot;
		out[k] = (m[k * 3 + i] + m[i * 3 + k]) * fRoot;
	}
	//*/


	/*
	float* m = matrix.m;
	float trace = m[0] + m[5] + m[10];
	if (trace > 0.0)
	{
		float s = sqrt(trace + 1.0);
		q[3] = s * 0.5;//w
		float recip = 0.5 / s;
		q[0] = (m[9] - m[6]) * -recip; //2,1  1,2
		q[1] = (m[8] - m[2]) * -recip; //0,2  2,0
		q[2] = (m[4] - m[1]) * -recip; //1,0  0,1
	}
	else
	{
		int i = 0;
		if (m[5] > m[0])
			i = 1;
		if (m[10] > m[i * 4 + i])
			i = 2;
		int j = (i + 1) % 3;
		int k = (j + 1) % 3;
		float s = sqrt(m[i * 4 + i] - m[j * 4 + j] - m[k * 4 + k] + 1.0);
		q[i] = 0.5 * s;
		float recip = 0.5 / s;
		q[3] = (m[k * 4 + j] - m[j * 4 + k]) * -recip;//w
		q[j] = (m[j * 4 + i] + m[i * 4 + j]) * -recip;
		q[k] = (m[k * 4 + i] + m[i * 4 + k]) * -recip;
	}
	normalize();
	//*/
}

Vector3f transformQuat(const Vector3f& a, const Quaternion& q)
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

	Vector3f out;

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

void Quaternion::getAxisAngle(Vector3f &v, float &angle) const
{
	angle = 2.0f * acosf(w);
	float factor = 1.0f / sqrtf(1 - w*w);
	v.x = x * factor;
	v.y = y * factor;
	v.z = z * factor;
}

Vector3f Quaternion::rotate(const Vector3f& v) const
{
	Vector3f ret;
	Quaternion temp(-x, -y, -z, w);
	temp *= v;
	temp *= (*this);

	ret.x = temp.x;
	ret.y = temp.y;
	ret.z = temp.z;

	return ret;
}

void Quaternion::fromEuler(Vector3f& vec)
{
	float heading = vec.v[0];
	float attitude = vec.v[1];
	float bank = vec.v[2];

	float C1 = cos(heading); //yaw
	float C2 = cos(attitude); //pitch
	float C3 = cos(bank); //roll
	float S1 = sin(heading);
	float S2 = sin(attitude);
	float S3 = sin(bank);

	float w = sqrt(1.0f + C1 * C2 + C1 * C3 - S1 * S2 * S3 + C2 * C3) * 0.5f;
	if (w == 0.0)
	{
		w = 0.000001f;
		//quat.set(out, 0,0,0,1 );
		//return out;
	}

	float x = (C2 * S3 + C1 * S3 + S1 * S2 * C3) / (4.0f * w);
	float y = (S1 * C2 + S1 * C3 + C1 * S2 * S3) / (4.0f * w);
	float z = (-S1 * S3 + C1 * S2 * C3 + S2) / (4.0f * w);
	set(x, y, z, w);
	normalize();
}

void Quaternion::toEulerAngles(Vector3f &euler) const
{
	float heading = atan2(2 * q[1] * q[3] - 2 * q[0] * q[2], 1 - 2 * q[1] * q[1] - 2 * q[2] * q[2]);
	float attitude = asin(2 * q[0] * q[1] + 2 * q[2] * q[3]);
	float bank = atan2(2 * q[0] * q[3] - 2 * q[1] * q[2], 1 - 2 * q[0] * q[0] - 2 * q[2] * q[2]);
	euler.set( heading, attitude, bank );

	/*
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
	*/
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

float ComputeSignedAngle(Vector2f a, Vector2f b)
{
	a.normalize();
	b.normalize();
	return atan2(a.perpdot(b), a.dot(b));
}

bool RaySphereCollision(const Vector3f& center, const float& radius, const Vector3f& ray_origin, const Vector3f& ray_dir, Vector3f& coll, float& t)
{
	Vector3f m = ray_origin - center;
	float b = dot(m, ray_dir);
	float c = dot(m, m) - radius * radius;

	// Exit if rs origin outside s (c > 0) and r pointing away from s (b > 0) 
	if (c > 0.0f && b > 0.0f)
		return false;
	float discr = b * b - c;

	// A negative discriminant corresponds to ray missing sphere 
	if (discr < 0.0f)
		return false;

	// Ray now found to intersect sphere, compute smallest t value of intersection
	t = -b - sqrt(discr);

	// If t is negative, ray started inside sphere so clamp t to zero 
	if (t < 0.0f)
		t = 0.0f;
	coll = ray_origin + t * ray_dir;

	return true;
}

bool RayPlaneCollision(const Vector3f& plane_pos, const Vector3f& plane_normal, const Vector3f& ray_origin, const Vector3f& ray_dir, Vector3f& result)
{
	double D = plane_pos.dot(plane_normal);
	double numer = D - plane_normal.dot(ray_origin);
	double denom = plane_normal.dot(ray_dir);
	if (abs(denom) < 0.00001)//parallel
		return false;
	double t = (numer / denom);
	if (t < 0)//back
		return false;
	result = ray_origin + ray_dir * (float)t;
	return true;
}

int planeBoxOverlap( const Vector4f& plane, const Vector3f& center, const Vector3f& halfsize )
{
	Vector3f n = plane.xyz();
	float d = plane.w;
	float radius = abs(halfsize.x * n[0]) + abs(halfsize.y * n[1]) + abs(halfsize.z * n[2]);
	float distance = dot(n, center) + d;
	if (distance <= -radius)
		return CLIP_OUTSIDE;
	else if (distance <= radius)
		return CLIP_OVERLAP;
	return CLIP_INSIDE;
}

float signedDistanceToPlane( const Vector4f& plane, const Vector3f& point )
{
	return dot(plane.xyz(), point) + plane.w;
}

const Vector3f corners[] = { {1,1,1},  {1,1,-1},  {1,-1,1},  {1,-1,-1},  {-1,1,1},  {-1,1,-1},  {-1,-1,1},  {-1,-1,-1} };

BoundingBox transformBoundingBox(const Matrix44 m, const BoundingBox& box)
{
	Vector3f box_min(10000000.0f,1000000.0f, 1000000.0f);
	Vector3f box_max(-10000000.0f, -1000000.0f, -1000000.0f);

	for (int i = 0; i < 8; ++i)
	{
		Vector3f corner = corners[i];
		corner = box.halfsize * corner;
		corner = corner + box.center;
		corner = m * corner;
		box_min.setMin(corner);
		box_max.setMax(corner);
	}

	Vector3f halfsize = (box_max - box_min) * 0.5f;
	return BoundingBox(box_max - halfsize, halfsize );
}

BoundingBox mergeBoundingBoxes(const BoundingBox& a, const BoundingBox& b)
{
	BoundingBox result;
	Vector3f bbmin = (a.center - a.halfsize);
	Vector3f bbmax = (a.center + a.halfsize);
	bbmin.setMin(b.center - b.halfsize);
	bbmax.setMax(b.center + b.halfsize);
	result.center = (bbmax + bbmin) * 0.5f;
	result.halfsize = bbmax - result.center;
	return result;
}

//from https://github.com/erich666/GraphicsGems/blob/master/gems/RayBox.c
bool RayBoundingBoxCollision(const BoundingBox& box, const Vector3f& ray_origin, const Vector3f& ray_dir, Vector3f& coll)
{
	const int NUMDIM = 3;
	const int RIGHT = 0;
	const int LEFT = 1;
	const int MIDDLE = 2;

	char inside = true;
	char quadrant[NUMDIM];
	int i;
	int whichPlane;
	float maxT[NUMDIM];
	float candidatePlane[NUMDIM];
	Vector3f minB = box.center - box.halfsize;
	Vector3f maxB = box.center + box.halfsize;

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

bool BoundingBoxSphereOverlap(const BoundingBox& box, const Vector3f& center, float radius)
{
	// arvo's algorithm from gamasutra
	// http://www.gamasutra.com/features/19991018/Gomez_4.htm

	float s, d = 0.0;
	//find the square of the distance
	//from the sphere to the box
	Vector3f vmin = box.center - box.halfsize;
	Vector3f vmax = box.center + box.halfsize;
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


std::ostream& operator<<(std::ostream& os, const Vector3f& v)
{
	os << v.x << ',' << v.y << ',' << v.z;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Vector4f& v)
{
	os << v.x << ',' << v.y << ',' << v.z << ',' << v.w;
	return os;
}
