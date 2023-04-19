/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	Here we define all the mathematical classes like Vector3, Matrix44 and some extra useful geometrical functions
*/

#ifndef FRAMEWORK //macros to ensure the code is included once
#define FRAMEWORK

#include <vector>
#include <cmath>
#include <ostream>

#ifndef PI
	#define PI 3.14159265359
#endif
#define DEG2RAD 0.0174532925
#define RAD2DEG 57.295779513

//more standard type definition
typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long int64;
typedef unsigned long uint64;
typedef float f32;
typedef double f64;

inline float clamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerp(float a, float b, float v ) { return a*(1.0f-v) + b*v; }

enum {
	CLIP_OUTSIDE = 0,
	CLIP_OVERLAP,
	CLIP_INSIDE
};

//** VEC2 *******************************************************

template<typename T>
class Vector2
{
public:
	union
	{
		struct { T x,y; };
		T value[2];
	};

	Vector2() { x = y = 0.0f; }
	Vector2(T x, T y) { this->x = x; this->y = y; }
	template<typename S> Vector2(const Vector2<S>& v) { this->x = (T)v.x; this->y = (T)v.y; }

	T length() { return sqrt(x*x + y*y); }
	T length() const { return sqrt(x*x + y*y); }

	T dot( const Vector2<T>& v ) { return x * v.x + y * v.y; }
	T perpdot( const Vector2<T>& v ) { return y * v.x + -x * v.y; }

	void set(T x, T y) { this->x = x; this->y = y; }

	Vector2<T>& normalize() { *this *= (T)length(); return *this; }

	T distance(const Vector2<T>& v) { return (T)(v - *this).length(); }
	void random(T range);
	void parseFromText(const char* text);

	void operator *= (T v) { x*=v; y*=v; }
};

template<typename T> inline Vector2<T> operator * (const Vector2<T>& a, T v) { return Vector2<T>(a.x * v, a.y * v); };
template<typename T> inline Vector2<T> operator + (const Vector2<T>& a, const Vector2<T>& b) { return Vector2<T>(a.x + b.x, a.y + b.y); }
template<typename T> inline Vector2<T> operator - (const Vector2<T>& a, const Vector2<T>& b) { return Vector2<T>(a.x - b.x, a.y - b.y); }

template<typename T> Vector2<T> normalize(const Vector2<T>& n);
template<typename T> inline Vector2<T> lerp(const Vector2<T>& a, const Vector2<T>& b, T v) { return a*((T)(1.0) - v) + b*v; }

typedef Vector2<float> Vector2f;
typedef Vector2<unsigned int> Vector2ui;

//** VEC3 *******************************

template<typename T>
class Vector3
{
public:
	union
	{
		struct { T x,y,z; };
		T v[3];
	};

	Vector3() { x = y = z = (T)0.0; }
	Vector3(T v) { x = y = z = v; }
	Vector3(T x, T y, T z) { this->x = x; this->y = y; this->z = z;	}

	T length();
	T length() const;

	void set(T x, T y, T z) { this->x = x; this->y = y; this->z = z; }

	void setMin(const Vector3<T> & v);
	void setMax(const Vector3<T>& v);

	Vector3<T>& normalize();
	void random(T range);
	void random(const Vector3<T>& range);

	T distance(const Vector3<T>& v) const { return (T)(v - *this).length();	}

	Vector3<T> cross( const Vector3<T>& v ) const;
	T dot( const Vector3<T>& v ) const;

	void parseFromText(const char* text, const char separator);

	T& operator [] (int n) { return v[n]; }
	void operator += (const Vector3<T>& v) { x += v.x; y += v.y; z += v.z; }
	void operator -= (const Vector3<T>& v) { x -= v.x; y -= v.y; z -= v.z; }
	void operator /= (const Vector3<T>& v) { x /= v.x; y /= v.y; z /= v.z; }
	void operator *= (T v) { x *= v; y *= v; z *= v; }
	void operator /= (T v) { x /= v; y /= v; z /= v; }
	void operator = (T* v) { x = v[0]; y = v[1]; z = v[2]; }
};

template<typename T> inline Vector3<T> operator + (const Vector3<T>& a, const Vector3<T>& b) { return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z); }
template<typename T> inline Vector3<T> operator - (const Vector3<T>& a, const Vector3<T>& b) { return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z); }
template<typename T> inline Vector3<T> operator * (const Vector3<T>& a, const Vector3<T>& b) { return Vector3<T>(a.x * b.x, a.y * b.y, a.z * b.z); }
template<typename T> inline Vector3<T> operator * (const Vector3<T>& a, T v) { return Vector3<T>(a.x * v, a.y * v, a.z * v); }
template<typename T> inline Vector3<T> operator * (float v, const Vector3<T>& a) { return Vector3<T>(a.x * v, a.y * v, a.z * v); }

template<typename T> inline Vector3<T> normalize(Vector3<T> n) { return n.normalize(); }
template<typename T> inline T dot(const Vector3<T>& a, const Vector3<T>& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
template<typename T> inline Vector3<T> cross(const Vector3<T>& a, const Vector3<T>& b) { return Vector3<T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
template<typename T> inline Vector3<T> lerp(const Vector3<T>& a, const Vector3<T>& b, float v) { return a * ((T)1.0 - v) + b * v; }

typedef Vector3<float> Vector3f;

//vector3 for unsigned ints (used for mesh indices triplets)
class Vector3u
{
public:
	union
	{
		struct {
			unsigned int x;
			unsigned int y;
			unsigned int z;
		};
		unsigned int v[3];
	};
	Vector3u() { x = y = z = 0; }
	Vector3u(unsigned int x, unsigned int y, unsigned int z) { this->x = x; this->y = y; this->z = z; }
	void set(unsigned int x, unsigned int y, unsigned int z) { this->x = x; this->y = y; this->z = z; }
};


//** VEC4 *************************************************

template<typename T>
class Vector4
{
public:
	union
	{
		struct { T x,y,z,w; };
		T v[4];
	};

	Vector4() { x = y = z = w = 0.0; }
	Vector4(T x, T y, T z, T w) { this->x = x; this->y = y; this->z = z; this->w = w; }
	Vector4(const Vector3<T>& v, T _w) { x = v.x; y = v.y; z = v.z; w = _w; }
	Vector4(const T* v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }

	Vector3<T> xyz() const { return Vector3<T>(x, y, z); }
	void set(T x, T y, T z, T w) { this->x = x; this->y = y; this->z = z; this->w = w; }
	void set(const Vector3f& v, T w) { x = v.x; y = v.y; z = v.z; this->w = w; }
	void operator = (T* v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }
};

template <typename T> inline Vector4<T> operator * (const Vector4<T>& a, T v) { return Vector4<T>(a.x * v, a.y * v, a.z * v, a.w * v); }
template <typename T> inline Vector4<T> operator + (const Vector4<T>& a, const Vector4<T>& b) { return Vector4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
template <typename T> inline Vector4<T> lerp(const Vector4<T>& a, const Vector4<T>& b, float v) { return a*(1.0f - v) + b*v; }

typedef Vector4<float> Vector4f;

//can be used to store colors
class Vector4ub
{
public:
	union
	{
		struct {
			unsigned char x;
			unsigned char y;
			unsigned char z;
			unsigned char w;
		};
		struct {
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};
		unsigned char v[4];
	};
	Vector4ub() { x = y = z = 0; }
	Vector4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w = 0) { this->x = x; this->y = y; this->z = z; this->w = w; }
	void set(unsigned char x, unsigned char y, unsigned char z, unsigned char w = 0) { this->x = x; this->y = y; this->z = z; this->w = w; }
	Vector4ub operator = (const Vector4f& a) { x = (unsigned char)a.x; y = (unsigned char)a.y; z = (unsigned char)a.z; w = (unsigned char)a.w; return *this;  }
	Vector4f toVector4f() { return Vector4f(x, y, z, w); }
};

inline Vector4ub operator + (const Vector4ub& a, const Vector4ub& b) { return Vector4ub(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w ); }
inline Vector4ub operator * (const Vector4ub& a, float v) { return Vector4ub((unsigned char)(a.x * v), (unsigned char)(a.y * v), (unsigned char)(a.z * v), (unsigned char)(a.w * v)); }
inline bool operator == (const Vector4ub& a, const Vector4ub& b) { return a.x == b.x && a.y == b.y && a.z == b.z; } //only colors, no alpha
inline Vector4ub lerp(const Vector4ub& a, const Vector4ub& b, float v) { return a*(1.0f - v) + b*v; }

typedef Vector4ub Color;

//****************************
//Matrix44 class
class Matrix44
{
	public:
		static const Matrix44 IDENTITY;

		//This matrix works in 
		union { //allows to access the same var using different ways
			struct
			{
				float        _11, _12, _13, _14;
				float        _21, _22, _23, _24;
				float        _31, _32, _33, _34;
				float        _41, _42, _43, _44;
			};
			float M[4][4]; //[row][column]
			float m[16];
		};

		Matrix44();
		Matrix44(const float* v);

		void set(); //multiply with opengl matrix
		void load(); //load in opengl matrix
		void clear();
		void setIdentity();
		void transpose();
		void normalizeAxis();

		//get base vectors
		Vector3f rightVector() { return Vector3f(m[0],m[1],m[2]); }
		Vector3f topVector() { return Vector3f(m[4],m[5],m[6]); }
		Vector3f frontVector() { return Vector3f(m[8],m[9],m[10]); }

		bool inverse();
		void setUpAndOrthonormalize(Vector3f up);
		void setFrontAndOrthonormalize(Vector3f front);

		Matrix44 getRotationOnly(); //used when having scale

		//rotate only
		Vector3f rotateVector( const Vector3f& v) const;

		//transform using local coordinates
		void translate(float x, float y, float z);
		void rotate( float angle_in_rad, const Vector3f& axis  );
		void scale(float x, float y, float z);

		//transform using global coordinates
		void translateGlobal(float x, float y, float z);
		void rotateGlobal( float angle_in_rad, const Vector3f& axis  );

		//create a transformation matrix from scratch
		void setTranslation(float x, float y, float z);
		void setRotation( float angle_in_rad, const Vector3f& axis );
		void setScale(float x, float y, float z);

		Vector3f getTranslation();
		Vector3f getScale();

		bool getXYZ(float* euler) const; //not sure which axis...

		void lookAt(Vector3f& eye, Vector3f& center, Vector3f& up);
		void perspective(float fov, float aspect, float near_plane, float far_plane);
		void ortho(float left, float right, float bottom, float top, float near_plane, float far_plane);

		Vector3f project(const Vector3f& v);

		Matrix44 operator * (const Matrix44& matrix) const;
};

//Operators, they are our friends
//Matrix44 operator * ( const Matrix44& a, const Matrix44& b );
Vector3f operator * (const Matrix44& matrix, const Vector3f& v);
Vector4f operator * (const Matrix44& matrix, const Vector4f& v);

//** QUAT ********************************************************

class Quaternion
{
public:

	union
	{
		struct { float x; float y; float z; float w; };
		float q[4];
	};

public:
	Quaternion();
	Quaternion(const float* q);
	Quaternion(const Quaternion& q);
	Quaternion(const float X, const float Y, const float Z, const float W);
	Quaternion(const Vector3f& axis, float angle);

	void identity();
	Quaternion invert() const;
	Quaternion conjugate() const;

	void set(const float X, const float Y, const float Z, const float W);
	void slerp(const Quaternion& b, float t);
	void slerp(const Quaternion& q2, float t, Quaternion &q3) const;

	void lerp(const Quaternion& b, float t);
	void lerp(const Quaternion& q2, float t, Quaternion &q3) const;

public:
	void setAxisAngle(const Vector3f& axis, const float angle);
	void setAxisAngle(float x, float y, float z, float angle);
	void getAxisAngle(Vector3f &v, float &angle) const;

	Vector3f rotate(const Vector3f& v) const;

	void operator*=(const Vector3f& v);
	void operator *= (const Quaternion &q);
	void operator += (const Quaternion &q);

	friend Quaternion operator + (const Quaternion &q1, const Quaternion& q2);
	friend Quaternion operator * (const Quaternion &q1, const Quaternion& q2);

	friend Quaternion operator * (const Quaternion &q, const Vector3f& v);

	friend Quaternion operator * (float f, const Quaternion &q);
	friend Quaternion operator * (const Quaternion &q, float f);

	Quaternion& operator -();


	friend bool operator==(const Quaternion& q1, const Quaternion& q2);
	friend bool operator!=(const Quaternion& q1, const Quaternion& q2);

	void operator *= (float f);

	void computeMinimumRotation(const Vector3f& rotateFrom, const Vector3f& rotateTo);

	void normalize();
	float squaredLength() const;
	float length() const;
	void toMatrix(Matrix44 &) const;
	void fromMatrix(Matrix44&);

	void fromEuler(Vector3f& euler); //yaw, pitch, roll
	void toEulerAngles(Vector3f &euler) const; //yaw, pitch, roll

	float& operator[] (unsigned int i) { return q[i]; }
};

float DotProduct(const Quaternion &q1, const Quaternion &q2);
Quaternion Qlerp(const Quaternion &q1, const Quaternion &q2, float t);
Quaternion Qslerp(const Quaternion &q1, const Quaternion &q2, float t);
Quaternion Qsquad(const Quaternion &q1, const Quaternion &q2, const Quaternion &a, const Quaternion &b, float t);
Quaternion Qsquad(const Quaternion &q1, const Quaternion &q2, const Quaternion &a, float t);
Quaternion Qspline(const Quaternion &q1, const Quaternion &q2, const Quaternion &q3);
Quaternion QslerpNoInvert(const Quaternion &q1, const Quaternion &q2, float t);
Quaternion Qexp(const Quaternion &q);
Quaternion Qlog(const Quaternion &q);
Quaternion SimpleRotation(const Vector3f &a, const Vector3f &b);
Vector3f transformQuat(const Vector3f& a, const Quaternion& q); //to euler

//** Boundings ********************************************************

class BoundingBox
{
public:
	Vector3f center;
	Vector3f halfsize;
	BoundingBox() {}
	BoundingBox(Vector3f center, Vector3f halfsize) { this->center = center; this->halfsize = halfsize; };
	float getArea() { return halfsize.x * halfsize.y * halfsize.z * 2.0f; }
};

//applies a transform to a AABB from object to world
BoundingBox mergeBoundingBoxes(const BoundingBox& a, const BoundingBox& b);
BoundingBox transformBoundingBox(const Matrix44 m, const BoundingBox& box);


//** RAY ********************************************************
class Ray
{
public:
	Vector3f origin;
	Vector3f direction;
};

// ** Global Operations *********************************************

float signedDistanceToPlane(const Vector4f& plane, const Vector3f& point);
int planeBoxOverlap( const Vector4f& plane, const Vector3f& center, const Vector3f& halfsize );
float ComputeSignedAngle( Vector2f a, Vector2f b); //returns the angle between both vectors in radians
inline float ease(float f) { return f*f*f*(f*(f*6.0f - 15.0f) + 10.0f); }
bool RaySphereCollision(const Vector3f& center, const float& radius, const Vector3f& ray_origin, const Vector3f& ray_dir, Vector3f& coll, float& t);
bool RayPlaneCollision( const Vector3f& plane_pos, const Vector3f& plane_normal, const Vector3f& ray_origin, const Vector3f& ray_dir, Vector3f& result );
bool RayBoundingBoxCollision(const BoundingBox& box, const Vector3f& ray_origin, const Vector3f& ray_dir, Vector3f& coll);
bool BoundingBoxSphereOverlap(const BoundingBox& box, const Vector3f& center, float radius );
inline Vector3f reflect(const Vector3f& I, const Vector3f& N) { return I - N * 2.0f * dot(N, I); }

//value between 0 and 1
inline float random(float range = 1.0f, int offset = 0) { return ((rand() % 1000) / (1000.0f)) * range + offset; }

std::ostream& operator << (std::ostream& os, const Vector3f& v);
std::ostream& operator << (std::ostream& os, const Vector4f& v);

//generic types

typedef Vector2f vec2;
typedef Vector3f vec3;
typedef Vector4f vec4;
typedef Matrix44 mat4;
typedef Quaternion quat;

#endif