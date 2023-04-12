/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This class wraps the behaviour of a camera. A Camera helps to set the point of view from where we will render the scene.
	The most important attributes are  eye and center which say where is the camera and where is it pointing.
	This class also stores the matrices used to do the transformation and projection of the scene.
*/

#ifndef CAMERA_H
#define CAMERA_H

#include "../core/math.h"

class Camera
{
public:
	static Camera* current;

	enum { PERSPECTIVE, ORTHOGRAPHIC }; //types of cameras available

	char type; //camera type

	//vectors to define the orientation of the camera
	Vector3f eye; //where is the camera
	Vector3f center; //where is it pointing
	Vector3f up; //the up pointing up

	//properties of the projection of the camera
	float fov;			//view angle in degrees (1/zoom)
	float aspect;		//aspect ratio (width/height)
	float near_plane;	//near plane
	float far_plane;	//far plane

	//for orthogonal projection
	float left,right,top,bottom;

	//planes
	float frustum[6][4];

	//matrices
	Matrix44 view_matrix;
	Matrix44 projection_matrix;
	Matrix44 viewprojection_matrix;
	Matrix44 inverse_viewprojection_matrix;

	Vector3f front;

	Camera();

	//set as current
	void enable();

	//translate and rotate the camera
	void move(Vector3f delta);
	void moveGlobal(Vector3f delta);
	void rotate(float angle, const Vector3f& axis);
	void orbit(float yaw, float pitch);
	void changeDistance(float dt);

	//transform a local camera vector to world coordinates
	Vector3f getLocalVector(const Vector3f& v);

	//set the info
	void setPerspective(float fov, float aspect, float near_plane, float far_plane);
	void setOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);
	void lookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up);
	void lookAt(const Matrix44& m);

	//used to extract frustum planes
	void extractFrustum();

	//compute the matrices
	void updateViewMatrix();
	void updateProjectionMatrix();

	//to work between world and screen coordinates
	Vector3f project(Vector3f pos3d, float window_width, float window_height, bool flip_y = false); //to project 3D points to screen coordinates
	Vector3f unproject( Vector3f coord2d, float window_width, float window_height ); //to project screen coordinates to world coordinates
	float getProjectedScale(Vector3f pos3D, float radius); //used to know how big one unit will look at this distance
	Vector3f getRayDirection(int mouse_x, int mouse_y, float window_width, float window_height);

	//culling
	bool testPointInFrustum( Vector3f v );
	char testSphereInFrustum( const Vector3f& v, float radius);
	char testBoxInFrustum( const Vector3f& center, const Vector3f& halfsize );
};


#endif