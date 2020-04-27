/*   ColDet - C++ 3D Collision Detection Library
 *   Copyright (C) 2000   Amir Geva
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Any comments, questions and bug reports send to:
 *   photon@photoneffect.com
 *
 * Or visit the home page: http://photoneffect.com/coldet/
 */
#ifndef H_MYTRITRI
#define H_MYTRITRI

#include "box.h"

/** A slower triangle-triangle intersection test, that returns the
    point of intersection. */
Vector3D my_tri_tri_intersect(const Triangle& t1, const Triangle& t2);

/** Triangle description class.  It is used to determine if a point
    on the triangle's plane is inside the triangle. */
class TriangleDesc : public Triangle
{
public:
  TriangleDesc(const Triangle& t, const Plane& p)
    : Triangle(t)
  {
    const Vector3D& n=p.normal;
    Vector3D a(flabs(n.x),flabs(n.y),flabs(n.z));
    if (a.x>a.y)
    {
      if (a.x>a.z) { i1=1; i2=2; }
      else         { i1=0; i2=1; }
    }
    else
    {
      if (a.y>a.z) { i1=0; i2=2; }
      else         { i1=0; i2=1; }
    }
  }

//bug fixed as seen on http://thephotoneffect.yuku.com/topic/614/t/Precision-bug-and-proposed-fix.html
	bool sameSide(const Vector3D& p1, const Vector3D& p2, const Vector3D& a, const Vector3D& b)
	{
		Vector3D cp1 = CrossProduct(b-a, p1-a);
		Vector3D cp2 = CrossProduct(b-a, p2-a);
		return ((cp1 * cp2) >= 0);
	}
	
	bool pointInTri(const Vector3D& P)
	{
		return (sameSide(P,v1,v2,v3) && sameSide(P,v2,v1,v3) && sameSide(P,v3,v1,v2));
	}
/*
  bool pointInTri(const Vector3D& P)
  {
    Vector3D u(P[i1]-v1[i1],
               v2[i1]-v1[i1],
               v3[i1]-v1[i1]);
    Vector3D v(P[i2]-v1[i2],
               v2[i2]-v1[i2],
               v3[i2]-v1[i2]);
    float a,b;
    if (u.y==0.0f)
    {
      b=u.x/u.z;
      if (b>=0.0f && b<=1.0f) a=(v.x-b*v.z)/v.y;
      else return false;
    }
    else
    {
      b=(v.x*u.y-u.x*v.y)/(v.z*u.y-u.z*v.y);
      if (b>=0.0f && b<=1.0f) a=(u.x-b*u.z)/u.y;
      else return false;
    }
    return (a>=0 && (a+b)<=1);
  }
*/
  const Vector3D& operator[] (int index) 
  { 
    switch (index)
    {
    case 0: return v1;
    case 1: return v2;
    case 2: return v3;
    case 3: return v1;
    }
    return v2;
  }

  int i1,i2;
};



#endif // H_MYTRITRI

