
/**
 * Stage visualization for ND
 * Author: Richard Vaughan 2009
 * $Id$
 */

#include "ndvis.h"

static void DrawCircle ( float x, float y, float z, float radius, float steps )
{
  glBegin ( GL_LINE_LOOP );
  for ( float a = 0;  a < 2.0*M_PI; a+= 2.0*M_PI/steps )
    glVertex3f ( x + radius * sin ( a ),
                 y + radius * cos ( a ),
                 z );
  glEnd();
}
//-----------------------------------------------------------------------------
static void DrawBox ( CRectangle& rect )
{
  glBegin ( GL_LINES );
  glVertex2f ( rect.getLeft(),
               rect.getBottom() );
  glVertex2f ( rect.getLeft(),
               rect.getTop() );

  glVertex2f ( rect.getLeft(),
               rect.getTop() );
  glVertex2f ( rect.getRight(),
               rect.getTop() );

  glVertex2f ( rect.getRight(),
               rect.getTop() );
  glVertex2f ( rect.getRight(),
               rect.getBottom() );

  glVertex2f ( rect.getRight(),
               rect.getBottom() );
  glVertex2f ( rect.getLeft(),
               rect.getBottom() );
  glEnd();
}
//-----------------------------------------------------------------------------
void NdVis::Visualize ( Stg::Model* mod, Stg::Camera* cam )
{
  Stg::Geom geom = mod->GetGeom();

  // push the current robot-centered coordinate frame on the stack
  glPushMatrix();

  // go into global coordinates
  Stg::Gl::pose_inverse_shift ( mod->GetGlobalPose() );

  // goal point
  if ( nd->hasActiveGoal() ) {
    CPose2d goal = nd->getGoal();
    glPointSize ( 15 );
    mod->PushColor ( 1,0,0,0.8 ); // red
    glBegin ( GL_POINTS );
    glVertex2f ( goal.mX, goal.mY );
    glEnd();
    char buf[64];
    snprintf ( buf, 64, "%s", nd->mInfo.situacion );
    Stg::Gl::draw_string ( goal.mX + 0.2, goal.mY + 0.2, 0, buf );
    mod->PopColor();
  }

  // obstacle cloud
  for ( int i = 0; i < nd->mObstacles.longitud; i++ ) {
    glPointSize ( 5 );
    mod->PushColor ( 1,0,0,0.8 ); // red
    glBegin ( GL_POINTS );
    glVertex3f ( nd->mObstacles.punto[i].x,
                 nd->mObstacles.punto[i].y, 0.5 );
    glEnd();
    mod->PopColor();
  }

  glPopMatrix(); // back to robot coords

  // current desired heading angle, in local frame
  mod->PushColor ( 0,0,1,0.8 ); // blue
  float dx =  1.0 * cos ( nd->mInfo.angle );
  float dy =  1.0 * sin ( nd->mInfo.angle );
  glBegin ( GL_LINES );
  glVertex2f ( 0, 0 );
  glVertex2f ( dx, dy );
  glEnd();
  mod->PopColor();

  // safety distance
  mod->PushColor ( 0,1,1,1 ); // green
  DrawCircle ( 0,0,0, nd->mSafetyDist, 20 );
  mod->PopColor();

  // sector distances
  glPointSize ( 5 );
  mod->PushColor ( 1,0,1,0.4 ); // red
  for ( unsigned int i = 0; i < SECTORES; i++ ) {
    //if ( nd->mInfo.d[i].r < 0 )
    if ( nd->mInfo.d[i].r < 0 )
      continue;

    glBegin ( GL_LINES );

    //float dx = nd->mInfo.dr[i] *  cos ( nd->mInfo.d[i].a );
    //float dy = nd->mInfo.dr[i] *  sin ( nd->mInfo.d[i].a );
    float dx = nd->mInfo.d[i].r *  cos ( nd->mInfo.d[i].a );
    float dy = nd->mInfo.d[i].r *  sin ( nd->mInfo.d[i].a );

    //float dx =  robot.E[i] * cos ( sector2angle ( i ) );
    //float dy =  robot.E[i] * sin ( sector2angle ( i ) );
    glVertex2f ( 0,0 );
    glVertex2f ( dx, dy );

    glEnd();
  }
  mod->PopColor();

  // ND Angle [-pi/2, pi/2]
  mod->PushColor ( 0,0,1,0.8 ); // blue
  glLineWidth ( 5.0 );
  glBegin ( GL_LINES );
  dx = 1.0 *  cos ( nd->mInfo.angle );
  dy = 1.0 *  sin ( nd->mInfo.angle );
  glVertex2f ( 0,0 );
  glVertex2f ( dx, dy );
  glEnd();
  mod->PopColor();
  glLineWidth ( 1.0 );

  if ( nd->mFrontAvoidBox.fgObstacle )
    mod->PushColor ( 1, 0, 0, 0.8 ); // red
  else
    mod->PushColor ( 0, 1, 0, 0.8 ); // green
  DrawBox ( nd->mFrontAvoidBox.rect );
  mod->PopColor();

  if ( nd->mRightAvoidBox.fgObstacle )
    mod->PushColor ( 1, 0, 0, 0.8 ); // red
  else
    mod->PushColor ( 0, 1, 0, 0.8 ); // green
  DrawBox ( nd->mRightAvoidBox.rect );
  mod->PopColor();

  if ( nd->mLeftAvoidBox.fgObstacle )
    mod->PushColor ( 1, 0, 0, 0.8 ); // red
  else
    mod->PushColor ( 0, 1, 0, 0.8 ); // green
  DrawBox ( nd->mLeftAvoidBox.rect );
  mod->PopColor();

  if ( nd->mBackAvoidBox.fgObstacle )
    mod->PushColor ( 1, 0, 0, 0.8 ); // red
  else
    mod->PushColor ( 0, 1, 0, 0.8 ); // green
  DrawBox ( nd->mBackAvoidBox.rect );
  mod->PopColor();

  // regions
  mod->PushColor ( 0,1,0,0.8 ); // green

  //printf ( "length: %d\n", nd->mInfo.regiones.longitud );

  for ( int i=0; i <nd->mInfo.regiones.longitud; i++ ) {
    TRegion* reg = &nd->mInfo.regiones.vector[i];

    /*
        printf ( "principio %d final %d principio_ascendente %d final_ascendente %d descartada %d direction_tipo %d direcction_sector %d direction_angle %.2f\n",           reg->principio,
                 reg->final, reg->principio_ascendente, reg->final_ascendente, reg->descartada, reg->direction_tipo, reg->direction_sector, reg->direction_angle );
    */
    int i = reg->principio;
    while ( i != reg->final ) {

      float dx = 1.0 *  cos ( sector2angle ( i ) );
      float dy = 1.0 *  sin ( sector2angle ( i ) );

      glBegin ( GL_LINES );
      glVertex3f ( 0,0,0.01 );
      glVertex3f ( dx,dy,0.01 );
      glEnd();
      i ++;
      if ( i >= SECTORES )
        i = 0;
    }
  }
}
