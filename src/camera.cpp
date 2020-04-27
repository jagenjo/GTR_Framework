#include "camera.h"
#include "utils.h"

#include "includes.h"
#include <iostream>

Camera* Camera::current = NULL;

Camera::Camera()
{
	lookAt( Vector3(0, 0, 0), Vector3(0, 0, -1), Vector3(0, 1, 0) );
	setOrthographic(-100,100,-100, 100,-100,100);
}

void Camera::enable()
{
    checkGLErrors();
	current = this;
	updateViewMatrix();
	updateProjectionMatrix();
	extractFrustum();
    checkGLErrors();

	//legacy rendering...
#ifndef __APPLE__
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection_matrix.m);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view_matrix.m);
    checkGLErrors();
#endif
}

void Camera::updateViewMatrix()
{
	view_matrix.lookAt( eye, center, up );
	viewprojection_matrix = view_matrix * projection_matrix;
	extractFrustum();
}

// ******************************************

//Create a projection matrix
void Camera::updateProjectionMatrix()
{
	if (type == ORTHOGRAPHIC)
		projection_matrix.ortho(left,right,bottom,top,near_plane,far_plane);
	else
		projection_matrix.perspective(fov, aspect, near_plane, far_plane);

	viewprojection_matrix = view_matrix * projection_matrix;

	extractFrustum();
}

Vector3 Camera::getLocalVector(const Vector3& v)
{
	Matrix44 iV = view_matrix;
	if (iV.inverse() == false)
		std::cout << "Matrix Inverse error" << std::endl;
	Vector3 result = iV.rotateVector(v);
	return result;
}

void Camera::move(Vector3 delta)
{
	Vector3 localDelta = getLocalVector(delta);
	eye = eye - localDelta;
	center = center - localDelta;
	updateViewMatrix();
}

void Camera::moveGlobal(Vector3 delta)
{
	eye = eye - delta;
	center = center - delta;
	updateViewMatrix();
}

void Camera::rotate(float angle, const Vector3& axis)
{
	Matrix44 R;
	R.setRotation(angle,axis);
	Vector3 new_front = R * (center - eye);
	center = eye + new_front;
	updateViewMatrix();

}

void Camera::orbit(float yaw, float pitch)
{
	vec3 front = normalize( center - eye );
	float problem_angle = dot(front, up);

	vec3 right = getLocalVector(vec3(1.0, 0, 0));
	vec3 dist = eye - center;

	//yaw
	quat R = quat(up, -yaw);
	dist = transformQuat(dist, R);

	if (!(problem_angle > 0.99 && pitch > 0 || problem_angle < -0.99 && pitch < 0))
		R.setAxisAngle(right, pitch);

	dist = transformQuat(dist, R);

	eye = dist + center;

	updateViewMatrix();
}

void Camera::changeDistance(float dt)
{
	if (type == ORTHOGRAPHIC)
	{
		float f = dt < 0 ? 1.1 : 0.9;
		left *= f;
		right *= f;
		bottom *= f;
		top *= f;
		updateProjectionMatrix();
		return;
	}
	vec3 dist = eye - center;
	dist *= dt < 0 ? 1.1 : 0.9;
	eye = dist + center;

	updateViewMatrix();
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	type = ORTHOGRAPHIC;

	this->left = left;
	this->right = right;
	this->bottom = bottom;
	this->top = top;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	updateProjectionMatrix();
}

void Camera::setPerspective(float fov, float aspect, float near_plane, float far_plane)
{
	type = PERSPECTIVE;

	this->fov = fov;
	this->aspect = aspect;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	//update projection
	updateProjectionMatrix();
}

void Camera::lookAt(const Vector3& eye, const Vector3& center, const Vector3& up)
{
	this->eye = eye;
	this->center = center;
	this->up = up;

	updateViewMatrix();
}

void Camera::lookAt(const Matrix44& m)
{
	this->eye = m * Vector3();
	this->center = m * Vector3(0,0,-1);
	this->up = m.rotateVector(Vector3(0, 1, 0));
}

void Camera::extractFrustum()
{
	float   proj[16]; 
	float   modl[16];
	float   clip[16];
	float   t;

	Matrix44 v = view_matrix;

	memcpy( proj, projection_matrix.m, sizeof(Matrix44) );
	memcpy( modl, v.m, sizeof(Matrix44));

	/* Combine the two matrices (multiply projection by modelview) */
	clip[0] = modl[0] * proj[0] + modl[1] * proj[4] + modl[2] * proj[8] + modl[3] * proj[12];
	clip[1] = modl[0] * proj[1] + modl[1] * proj[5] + modl[2] * proj[9] + modl[3] * proj[13];
	clip[2] = modl[0] * proj[2] + modl[1] * proj[6] + modl[2] * proj[10] + modl[3] * proj[14];
	clip[3] = modl[0] * proj[3] + modl[1] * proj[7] + modl[2] * proj[11] + modl[3] * proj[15];

	clip[4] = modl[4] * proj[0] + modl[5] * proj[4] + modl[6] * proj[8] + modl[7] * proj[12];
	clip[5] = modl[4] * proj[1] + modl[5] * proj[5] + modl[6] * proj[9] + modl[7] * proj[13];
	clip[6] = modl[4] * proj[2] + modl[5] * proj[6] + modl[6] * proj[10] + modl[7] * proj[14];
	clip[7] = modl[4] * proj[3] + modl[5] * proj[7] + modl[6] * proj[11] + modl[7] * proj[15];

	clip[8] = modl[8] * proj[0] + modl[9] * proj[4] + modl[10] * proj[8] + modl[11] * proj[12];
	clip[9] = modl[8] * proj[1] + modl[9] * proj[5] + modl[10] * proj[9] + modl[11] * proj[13];
	clip[10] = modl[8] * proj[2] + modl[9] * proj[6] + modl[10] * proj[10] + modl[11] * proj[14];
	clip[11] = modl[8] * proj[3] + modl[9] * proj[7] + modl[10] * proj[11] + modl[11] * proj[15];

	clip[12] = modl[12] * proj[0] + modl[13] * proj[4] + modl[14] * proj[8] + modl[15] * proj[12];
	clip[13] = modl[12] * proj[1] + modl[13] * proj[5] + modl[14] * proj[9] + modl[15] * proj[13];
	clip[14] = modl[12] * proj[2] + modl[13] * proj[6] + modl[14] * proj[10] + modl[15] * proj[14];
	clip[15] = modl[12] * proj[3] + modl[13] * proj[7] + modl[14] * proj[11] + modl[15] * proj[15];

	/* Extract the numbers for the RIGHT plane */
	frustum[0][0] = clip[3] - clip[0];
	frustum[0][1] = clip[7] - clip[4];
	frustum[0][2] = clip[11] - clip[8];
	frustum[0][3] = clip[15] - clip[12];

	/* Normalize the result */
	t = sqrt(frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2]);
	frustum[0][0] /= t;
	frustum[0][1] /= t;
	frustum[0][2] /= t;
	frustum[0][3] /= t;

	/* Extract the numbers for the LEFT plane */
	frustum[1][0] = clip[3] + clip[0];
	frustum[1][1] = clip[7] + clip[4];
	frustum[1][2] = clip[11] + clip[8];
	frustum[1][3] = clip[15] + clip[12];

	/* Normalize the result */
	t = sqrt(frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2]);
	frustum[1][0] /= t;
	frustum[1][1] /= t;
	frustum[1][2] /= t;
	frustum[1][3] /= t;

	/* Extract the BOTTOM plane */
	frustum[2][0] = clip[3] + clip[1];
	frustum[2][1] = clip[7] + clip[5];
	frustum[2][2] = clip[11] + clip[9];
	frustum[2][3] = clip[15] + clip[13];

	/* Normalize the result */
	t = sqrt(frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2]);
	frustum[2][0] /= t;
	frustum[2][1] /= t;
	frustum[2][2] /= t;
	frustum[2][3] /= t;

	/* Extract the TOP plane */
	frustum[3][0] = clip[3] - clip[1];
	frustum[3][1] = clip[7] - clip[5];
	frustum[3][2] = clip[11] - clip[9];
	frustum[3][3] = clip[15] - clip[13];

	/* Normalize the result */
	t = sqrt(frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2]);
	frustum[3][0] /= t;
	frustum[3][1] /= t;
	frustum[3][2] /= t;
	frustum[3][3] /= t;

	/* Extract the FAR plane */
	frustum[4][0] = clip[3] - clip[2];
	frustum[4][1] = clip[7] - clip[6];
	frustum[4][2] = clip[11] - clip[10];
	frustum[4][3] = clip[15] - clip[14];

	/* Normalize the result */
	t = sqrt(frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2]);
	frustum[4][0] /= t;
	frustum[4][1] /= t;
	frustum[4][2] /= t;
	frustum[4][3] /= t;

	/* Extract the NEAR plane */
	frustum[5][0] = clip[3] + clip[2];
	frustum[5][1] = clip[7] + clip[6];
	frustum[5][2] = clip[11] + clip[10];
	frustum[5][3] = clip[15] + clip[14];

	/* Normalize the result */
	t = sqrt(frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2]);
	frustum[5][0] /= t;
	frustum[5][1] /= t;
	frustum[5][2] /= t;
	frustum[5][3] /= t;
}

bool Camera::testPointInFrustum( Vector3 v )
{
	for (int p = 0; p < 6; p++)
		if (frustum[p][0] * v.x + frustum[p][1] * v.y + frustum[p][2] * v.z + frustum[p][3] <= 0)
			return false;
	return true;
}

Vector3 Camera::project( Vector3 pos3d, float window_width, float window_height)
{
	Vector3 norm = viewprojection_matrix.project(pos3d); //returns from 0 to 1
	norm.x *= window_width;
	norm.y *= window_height;
	return norm;
}

Vector3 Camera::unproject(Vector3 coord2d, float window_width, float window_height)
{
	coord2d.x = (coord2d.x * 2.0f) / window_width - 1.0f;
	coord2d.y = (coord2d.y * 2.0f) / window_height - 1.0f;
	coord2d.z = 2.0f * coord2d.z - 1.0f;
	Matrix44 inv_vp = viewprojection_matrix;
	inv_vp.inverse();
	Vector4 r = inv_vp * Vector4(coord2d, 1.0f );
	return Vector3(r.x / r.w, r.y / r.w, r.z / r.w );
}

Vector3 Camera::getRayDirection( int mouse_x, int mouse_y, float window_width, float window_height )
{
	Vector3 mouse_pos( (float)mouse_x, window_height - mouse_y, 1.0f);
	Vector3 p = unproject(mouse_pos, window_width, window_height);
	return (p - eye).normalize();
}

float Camera::getProjectedScale(Vector3 pos3D, float radius) {
	float dist = eye.distance(pos3D);
	return ((float)sin(fov*DEG2RAD) / dist) * radius * 200.0f; //100 is to compensate width in pixels
}


char Camera::testSphereInFrustum( const Vector3& v, float radius)
{
	int p;

	for (p = 0; p < 6; p++)
	{
		float dist = frustum[p][0] * v.x + frustum[p][1] * v.y + frustum[p][2] * v.z + frustum[p][3];
		if (dist <= -radius)
			return CLIP_OUTSIDE;
	}
	return CLIP_INSIDE;
}


void Camera::renderInMenu()
{
	#ifndef SKIP_IMGUI
		bool changed = false;
		changed |= ImGui::Combo("Type", (int*)&type, "PERSPECTIVE\0ORTHOGRAPHIC",2);
		if (changed && type == ORTHOGRAPHIC)
			setOrthographic(-200, 200, -200 / aspect, 200 / aspect, 0.1, 10000);
		changed |= ImGui::SliderFloat3("Eye", &eye.x, -100, 100);
		changed |= ImGui::SliderFloat3("Center", &center.x, -100, 100);
		changed |= ImGui::SliderFloat3("Up", &up.x, -100, 100);
		changed |= ImGui::SliderFloat("FOV", &fov, 15, 180);
		changed |= ImGui::SliderFloat("Near", &near_plane, 0.01, far_plane);
		changed |= ImGui::SliderFloat("Far", &far_plane, near_plane, 10000);
		if (changed)
			this->lookAt(eye, center, up);
	#endif
}


char Camera::testBoxInFrustum(const Vector3& center, const Vector3& halfsize)
{
	int flag = 0, o = 0;

	flag = planeBoxOverlap( (Vector4&)frustum[0], center,halfsize );
	if (flag == CLIP_OUTSIDE)
		return CLIP_OUTSIDE;
	o += flag;
	flag = planeBoxOverlap((Vector4&)frustum[1], center, halfsize);
	if (flag == CLIP_OUTSIDE)
		return CLIP_OUTSIDE;
	o += flag;
	flag = planeBoxOverlap((Vector4&)frustum[2], center, halfsize);
	if (flag == CLIP_OUTSIDE)
		return CLIP_OUTSIDE;
	o += flag;
	flag = planeBoxOverlap((Vector4&)frustum[3], center, halfsize);
	if (flag == CLIP_OUTSIDE)
		return CLIP_OUTSIDE;
	o += flag;
	flag = planeBoxOverlap((Vector4&)frustum[4], center, halfsize);
	if (flag == CLIP_OUTSIDE)
		return CLIP_OUTSIDE;
	o += flag;
	flag = planeBoxOverlap((Vector4&)frustum[5], center, halfsize);
	if (flag == CLIP_OUTSIDE)
		return CLIP_OUTSIDE;
	o += flag;
	return o == 0 ? CLIP_INSIDE : CLIP_OVERLAP;
}

