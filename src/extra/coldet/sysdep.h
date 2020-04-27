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
#ifndef H_SYSDEP
#define H_SYSDEP


///////////////////////////////////////////////////
// g++ compiler on most systems
///////////////////////////////////////////////////
#ifdef GCC

typedef unsigned long DWORD;
DWORD GetTickCount();
#define __CD__BEGIN
#define __CD__END

///////////////////////////////////////////////////
// Windows compilers
///////////////////////////////////////////////////
#elif defined(WIN32)

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #define __CD__BEGIN
  #define __CD__END
  #ifndef EXPORT
    #ifdef COLDET_EXPORTS
      #define EXPORT /*__declspec(dllexport)*/
    #else
      #define EXPORT /*__declspec(dllimport)*/
    #endif
  #endif

///////////////////////////////////////////////////
// MacOS 9.0.4/MacOS X.  CodeWarrior Pro 6
// Thanks to Marco Tenuti for this addition
///////////////////////////////////////////////////
#elif defined(__APPLE__)
   typedef unsigned long DWORD;
   #define __CD__BEGIN
   #define __CD__END
   #define GetTickCount() ::getTime()
extern long getTime();
#else

#error No system specified (WIN32 GCC macintosh)

#endif

#ifndef EXPORT
  #define EXPORT
#endif

#endif // H_SYSDEP
