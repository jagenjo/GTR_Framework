/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This class wraps the behaviour of a camera. A Camera helps to set the point of view from where we will render the scene.
	The most important attributes are  eye and center which say where is the camera and where is it pointing.
	This class also stores the matrices used to do the transformation and projection of the scene.
*/

#ifndef CAMERA_H
#define CAMERA_H

#include "framework.h"

class Camera
{
public:
	static Camera* current;

	enum { PERSPECTIVE, ORTHOGRAPHIC }; //types of cameras available

	char type; //camera type

	//vectors to define the orientation of the camera
	Vector3 eye; //where is the camera
	Vector3 center; //where is it pointing
	Vector3 up; //the up pointing up

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

	Camera();

	//set as current
	void enable();
	void renderInMenu();

	//translate and rotate the camera
	void move(Vector3 delta);
	void moveGlobal(Vector3 delta);
	void rotate(float angle, const Vector3& axis);
	void orbit(float yaw, float pitch);
	void changeDistance(float dt);

	//transform a local camera vector to world coordinates
	Vector3 getLocalVector(const Vector3& v);

	//set the info
	void setPerspective(float fov, float aspect, float near_plane, float far_plane);
	void setOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);
	void lookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
	void lookAt(const Matrix44& m);

	//used to extract frustum planes
	void extractFrustum();

	//compute the matrices
	void updateViewMatrix();
	void updateProjectionMatrix();

	//to work between world and screen coordinates
	Vector3 project(Vector3 pos3d, float window_width, float window_height); //to project 3D points to screen coordinates
	Vector3 unproject( Vector3 coord2d, float window_width, float window_height ); //to project screen coordinates to world coordinates
	float getProjectedScale(Vector3 pos3D, float radius); //used to know how big one unit will look at this distance
	Vector3 getRayDirection(int mouse_x, int mouse_y, float window_width, float window_height);

	//culling
	bool testPointInFrustum( Vector3 v );
	char testSphereInFrustum( const Vector3& v, float radius);
	char testBoxInFrustum( const Vector3& center, const Vector3& halfsize );
};


#endif