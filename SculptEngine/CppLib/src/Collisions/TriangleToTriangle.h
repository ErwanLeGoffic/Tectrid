/*
*         
*  Triangle-Triangle Overlap Test Routines        
*  July, 2002                                                          
*  Updated December 2003                                                
*                                                                       
*  This file contains C implementation of algorithms for                
*  performing two and three-dimensional triangle-triangle intersection test 
*  The algorithms and underlying theory are described in                    
*                                                                           
* "Fast and Robust Triangle-Triangle Overlap Test 
*  Using Orientation Predicates"  P. Guigue - O. Devillers
*                                                 
*  Journal of Graphics Tools, 8(1), 2003                                    
*                                                                           
*  Several geometric predicates are defined.  Their parameters are all      
*  points.  Each point is an array of two or three double precision         
*  floating point numbers. The geometric predicates implemented in          
*  this file are:                                                            
*                                                                           
*    int tri_tri_overlap_test_3d(p1,q1,r1,p2,q2,r2)                         
*    int tri_tri_overlap_test_2d(p1,q1,r1,p2,q2,r2)                         
*                                                                           
*    int tri_tri_intersection_test_3d(p1,q1,r1,p2,q2,r2,
*                                     coplanar,source,target)               
*                                                                           
*       is a version that computes the segment of intersection when            
*       the triangles overlap (and are not coplanar)                        
*                                                                           
*    each function returns 1 if the triangles (including their              
*    boundary) intersect, otherwise 0                                       
*                                                                           
*                                                                           
*  Other information are available from the Web page                        
*  http://www.acm.org/jgt/papers/GuigueDevillers03/                         
*                                                                           
*/

#define real float

/* function prototype */

// Three-dimensional Triangle-Triangle Overlap Test
int tri_tri_overlap_test_3d(real p1[3], real q1[3], real r1[3], 
          real p2[3], real q2[3], real r2[3]);


// Three-dimensional Triangle-Triangle Overlap Test
// additionaly computes the segment of intersection of the two triangles if it exists. 
// coplanar returns whether the triangles are coplanar, 
// source and target are the endpoints of the line segment of intersection 
int tri_tri_intersection_test_3d(real p1[3], real q1[3], real r1[3], 
								 real p2[3], real q2[3], real r2[3],
								 int * coplanar, 
								 real source[3],real target[3]);


int coplanar_tri_tri3d(real  p1[3], real  q1[3], real  r1[3],
           real  p2[3], real  q2[3], real  r2[3],
           real  N1[3], real  N2[3]);


// Two dimensional Triangle-Triangle Overlap Test
int tri_tri_overlap_test_2d(real p1[2], real q1[2], real r1[2], 
          real p2[2], real q2[2], real r2[2]);
