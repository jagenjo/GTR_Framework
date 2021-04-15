/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	Here we define all the mathematical classes like Vector3, Matrix44 and some extra useful geometrical functions
*/

#ifndef FRAMEWORK //macros to ensure the code is included once
#define FRAMEWORK

#include <vector>
#include <cmath>

#ifndef PI
	#define PI 3.14159265359
#endif
#define DEG2RAD 0.0174532925
#define RAD2DEG 57.295779513

//more standard type definition
typedef char int8;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef short int16;
typedef int int32;
typedef unsigned int uint32;

inline float clamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerp(float a, float b, float v ) { return a*(1.0f-v) + b*v; }

enum {
	CLIP_OUTSIDE = 0,
	CLIP_OVERLAP,
	CLIP_INSIDE
};

class Vector2
{
public:
	union
	{
		struct { float x,y; };
		float value[2];
	};

	Vector2() { x = y = 0.0f; }
	Vector2(float x, float y) { this->x = x; this->y = y; }

	double length() { return sqrt(x*x + y*y); }
	double length() const { return sqrt(x*x + y*y); }

	float dot( const Vector2& v );
	float perpdot( const Vector2& v );

	void set(float x, float y) { this->x = x; this->y = y; }

	Vector2& normalize() { *this *= (float)length(); return *this; }

	float distance(const Vector2& v);
	void random(float range);
	void parseFromText(const char* text);

	void operator *= (float v) { x*=v; y*=v; }
};

Vector2 operator * (const Vector2& a, float v);
Vector2 operator + (const Vector2& a, const Vector2& b);
Vector2 operator - (const Vector2& a, const Vector2& b);

Vector2 normalize(Vector2 n);
inline Vector2 lerp(const Vector2& a, const Vector2& b, float v) { return a*(1.0f - v) + b*v; }

class Vector3u
{
public:
	union
	{
		struct { unsigned int x;
				 unsigned int y;
				 unsigned int z; };
		unsigned int v[3];
	};
	Vector3u() { x = y = z = 0; }
	Vector3u(unsigned int x, unsigned int y, unsigned int z) { this->x = x; this->y = y; this->z = z; }
	void set(unsigned int x, unsigned int y, unsigned int z) { this->x = x; this->y = y; this->z = z; }
};

//*********************************

class Vector3
{
public:
	union
	{
		struct { float x,y,z; };
		float v[3];
	};

	Vector3() { x = y = z = 0.0f; }
	Vector3(float x, float y, float z) { this->x = x; this->y = y; this->z = z;	}

	double length();
	double length() const;

	void set(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

	void setMin(const Vector3 & v);
	void setMax(const Vector3 & v);

	Vector3& normalize();
	void random(float range);
	void random(Vector3 range);

	float distance(const Vector3& v) const;
	

	Vector3 cross( const Vector3& v ) const;
	float dot( const Vector3& v ) const;

	void parseFromText(const char* text, const char separator);

	float& operator [] (int n) { return v[n]; }
	void operator += (const Vector3& v) { x += v.x; y += v.y; z += v.z; }
	void operator -= (const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; }
	void operator /= (const Vector3& v) { x /= v.x; y /= v.y; z /= v.z; }
	void operator *= (float v) { x *= v; y *= v; z *= v; }
	void operator /= (float v) { x /= v; y /= v; z /= v; }
	void operator = (float* v) { x = v[0]; y = v[1]; z = v[2]; }
};

Vector3 normalize(Vector3 n);
float dot( const Vector3& a, const Vector3& b);
Vector3 cross(const Vector3&a, const Vector3& b);
Vector3 lerp(const Vector3& a, const Vector3& b, float v);

inline Vector3 operator + (const Vector3& a, const Vector3& b) { return Vector3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vector3 operator - (const Vector3& a, const Vector3& b) { return Vector3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vector3 operator * (const Vector3& a, const Vector3& b) { return Vector3(a.x * b.x, a.y * b.y, a.z * b.z); }
inline Vector3 operator * (const Vector3& a, float v) { return Vector3(a.x * v, a.y * v, a.z * v); }
inline Vector3 operator * (float v, const Vector3& a) { return Vector3(a.x * v, a.y * v, a.z * v); }

class Vector4
{
public:
	union
	{
		struct { float x,y,z,w; };
		float v[4];
	};

	Vector4() { x = y = z = w = 0.0; }
	Vector4(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }
	Vector4(const Vector3& v, float w) { x = v.x; y = v.y; z = v.z; this->w = w; }
	Vector4(const float* v) { x = v[0]; x = v[1]; x = v[2]; x = v[3]; }

	Vector3 xyz() const { return Vector3(x, y, z); }
	void set(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }
	void operator = (float* v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }
};

inline Vector4 operator * (const Vector4& a, float v) { return Vector4(a.x * v, a.y * v, a.z * v, a.w * v); }
inline Vector4 operator + (const Vector4& a, const Vector4& b) { return Vector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
inline Vector4 lerp(const Vector4& a, const Vector4& b, float v) { return a*(1.0f - v) + b*v; }

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
	Vector4ub operator = (const Vector4& a) { x = (unsigned char)a.x; y = (unsigned char)a.y; z = (unsigned char)a.z; w = (unsigned char)a.w; return *this;  }
	Vector4 toVector4() { return Vector4(x, y, z, w); }
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
		Vector3 rightVector() { return Vector3(m[0],m[1],m[2]); }
		Vector3 topVector() { return Vector3(m[4],m[5],m[6]); }
		Vector3 frontVector() { return Vector3(m[8],m[9],m[10]); }

		bool inverse();
		void setUpAndOrthonormalize(Vector3 up);
		void setFrontAndOrthonormalize(Vector3 front);

		Matrix44 getRotationOnly(); //used when having scale

		//rotate only
		Vector3 rotateVector( const Vector3& v) const;

		//transform using local coordinates
		void translate(float x, float y, float z);
		void rotate( float angle_in_rad, const Vector3& axis  );
		void scale(float x, float y, float z);

		//transform using global coordinates
		void translateGlobal(float x, float y, float z);
		void rotateGlobal( float angle_in_rad, const Vector3& axis  );

		//create a transformation matrix from scratch
		void setTranslation(float x, float y, float z);
		void setRotation( float angle_in_rad, const Vector3& axis );
		void setScale(float x, float y, float z);

		Vector3 getTranslation();

		bool getXYZ(float* euler) const;

		void lookAt(Vector3& eye, Vector3& center, Vector3& up);
		void perspective(float fov, float aspect, float near_plane, float far_plane);
		void ortho(float left, float right, float bottom, float top, float near_plane, float far_plane);

		Vector3 project(const Vector3& v);

		//old fixed pipeline (do not used if possible)
		void multGL();
		void loadGL();

		Matrix44 operator * (const Matrix44& matrix) const;
};

//Operators, they are our friends
//Matrix44 operator * ( const Matrix44& a, const Matrix44& b );
Vector3 operator * (const Matrix44& matrix, const Vector3& v);
Vector4 operator * (const Matrix44& matrix, const Vector4& v); 


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
	Quaternion(const Vector3& axis, float angle);

	void identity();
	Quaternion invert() const;
	Quaternion conjugate() const;

	void set(const float X, const float Y, const float Z, const float W);
	void slerp(const Quaternion& b, float t);
	void slerp(const Quaternion& q2, float t, Quaternion &q3) const;

	void lerp(const Quaternion& b, float t);
	void lerp(const Quaternion& q2, float t, Quaternion &q3) const;

public:
	void setAxisAngle(const Vector3& axis, const float angle);
	void setAxisAngle(float x, float y, float z, float angle);
	void getAxisAngle(Vector3 &v, float &angle) const;

	Vector3 rotate(const Vector3& v) const;

	void operator*=(const Vector3& v);
	void operator *= (const Quaternion &q);
	void operator += (const Quaternion &q);

	friend Quaternion operator + (const Quaternion &q1, const Quaternion& q2);
	friend Quaternion operator * (const Quaternion &q1, const Quaternion& q2);

	friend Quaternion operator * (const Quaternion &q, const Vector3& v);

	friend Quaternion operator * (float f, const Quaternion &q);
	friend Quaternion operator * (const Quaternion &q, float f);

	Quaternion& operator -();


	friend bool operator==(const Quaternion& q1, const Quaternion& q2);
	friend bool operator!=(const Quaternion& q1, const Quaternion& q2);

	void operator *= (float f);

	void computeMinimumRotation(const Vector3& rotateFrom, const Vector3& rotateTo);

	void normalize();
	float squaredLength() const;
	float length() const;
	void toMatrix(Matrix44 &) const;

	void toEulerAngles(Vector3 &euler) const;

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
Quaternion SimpleRotation(const Vector3 &a, const Vector3 &b);
Vector3 transformQuat(const Vector3& a, const Quaternion& q); //to euler

class BoundingBox
{
public:
	Vector3 center;
	Vector3 halfsize;
	BoundingBox() {}
	BoundingBox(Vector3 center, Vector3 halfsize) { this->center = center; this->halfsize = halfsize; };
	float getArea() { return halfsize.x * halfsize.y * halfsize.z * 2.0f; }
};

class Ray
{
public:
	Vector3 origin;
	Vector3 direction;
};

//applies a transform to a AABB from object to world
BoundingBox mergeBoundingBoxes(const BoundingBox& a, const BoundingBox& b);
BoundingBox transformBoundingBox(const Matrix44 m, const BoundingBox& box);

float signedDistanceToPlane(const Vector4& plane, const Vector3& point);
int planeBoxOverlap( const Vector4& plane, const Vector3& center, const Vector3& halfsize );
float ComputeSignedAngle( Vector2 a, Vector2 b); //returns the angle between both vectors in radians
inline float ease(float f) { return f*f*f*(f*(f*6.0f - 15.0f) + 10.0f); }
bool RayPlaneCollision( const Vector3& plane_pos, const Vector3& plane_normal, const Vector3& ray_origin, const Vector3& ray_dir, Vector3& result );
bool RayBoundingBoxCollision(const BoundingBox& box, const Vector3& ray_origin, const Vector3& ray_dir, Vector3& coll);
bool BoundingBoxSphereOverlap(const BoundingBox& box, const Vector3& center, float radius );
Vector3 reflect(const Vector3& I, const Vector3& N);

//value between 0 and 1
inline float random(float range = 1.0f, int offset = 0) { return ((rand() % 1000) / (1000.0f)) * range + offset; }


typedef Vector2 vec2;
typedef Vector3 vec3;
typedef Vector4 vec4;
typedef Matrix44 mat4;
typedef Quaternion quat;

#endif