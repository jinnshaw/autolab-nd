/***************************************************************************
 * Project: RAPI                                                           *
 * Author:  Jens Wawerla (jwawerla@sfu.ca)                                 *
 * $Id: nd.cpp,v 1.8 2009-04-08 22:40:41 jwawerla Exp $
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 * $Log: nd.cpp,v $
 * Revision 1.8  2009-04-08 22:40:41  jwawerla
 * Hopefully ND interface issue solved
 *
 * Revision 1.7  2009-04-03 15:10:02  jwawerla
 * *** empty log message ***
 *
 * Revision 1.6  2009-03-29 00:54:27  jwawerla
 * Replan ctrl seems to work now
 *
 * Revision 1.5  2009-03-21 02:31:50  jwawerla
 * *** empty log message ***
 *
 * Revision 1.4  2009-03-20 03:05:23  jwawerla
 * dynamic obstacles added to wavefront
 *
 * Revision 1.3  2009-03-17 03:11:30  jwawerla
 * Another day no glory
 *
 * Revision 1.2  2009-03-16 14:27:18  jwawerla
 * robots still get stuck
 *
 * Revision 1.1.1.1  2009-03-15 03:52:02  jwawerla
 * First commit
 *
 * Revision 1.2  2008/03/17 23:58:40  jwawerla
 * Added logwrite support
 *
 *
 ***************************************************************************/
#include "nd.h"
#include "nd2_alg.h"
#include "printerror.h"
#include "utilities.h"
#include <string.h>
#include <math.h>


//-----------------------------------------------------------------------------
CNd::CNd ( float frontDim, float backDim, float sideDim, std::string robotName )
{
  mRobotName = robotName;
  mFrontDim = frontDim;
  mBackDim = backDim;
  mSideDim = sideDim;


  mNumSensors = 0;
  mReadingIndex = 0;
  mFgReadingBufferInitialized = false;
  mSafetyDist = 0.1;
  mAvoidDist = 0.6;
  mDistEps = 0.6;
  mAngleEps = D2R ( 10.0 );
  mVMax = 0.45;
  mVMin = 0.02;
  mWMax = D2R ( 45.0 );
  mWMin = D2R ( 0.0 );
  mVCmd = 0.0;
  mWCmd = 0.0;
  mCurrentTime = 0.0f;
  mFgWaiting = false;
  mFgTurningInPlace = false;
  mFgStalled = false;
  mFgAtGoal = true;
  mRotateStuckTime = 10.0;  // [s]
  mTranslateStuckTime = 2.0;
  mTranslateStuckDist = 0.25;
  mTranslateStuckAngle = D2R ( 20.0 );
  mObstacles.longitud = 0;
  mVDotMax = 0.75;
  mWDotMax = 0.75;
  mCurrentTime = 0.0;
  mRotateMinError = 0.0;
  mRotateStartTime = 0.0;


  // Fill in the ND's parameter structure

  // Rectangular geometry
  // mNDparam.geometryRect = 1;
  mNDparam.geometryRect = 1;
  // Distance from the wheel to the front
  mNDparam.front = frontDim + mSafetyDist;
  // Distance from the wheel to the back
  mNDparam.back = backDim +  mSafetyDist;
  // Distance from the wheel to the left side (note robot is symetric, therefore
  // no need for right side
  mNDparam.left = sideDim + mSafetyDist;

  mNDparam.R = 0.3F;   // radius of robot, used only if geometryRect = 0

  mNDparam.holonomic = 0;  // Non holonomic vehicle

  mNDparam.vlmax = mVMax;
  mNDparam.vamax = mWMax;

  mNDparam.almax = mVDotMax;  // maximal translational acceleration
  mNDparam.aamax = mWDotMax;  // maximal rotational acceleration

  mNDparam.dsmax = mAvoidDist; // Security distance
  mNDparam.dsmin = mNDparam.dsmax / 4.0F;
  //NDparametros.enlarge = NDparametros.dsmin/2.0F;
  mNDparam.enlarge = mNDparam.dsmin * 0.2F;

  mNDparam.discontinuity = 1.5 * mNDparam.left;  // Discontinuity

  mNDparam.T = 0.1F;  // Sample rate of the SICK

  // Pass the structure to ND for initialization
  InicializarND ( &mNDparam );
  // set current driving direction to forward
  mCurrentDir = FORWARD;

  reset();
}
//-----------------------------------------------------------------------------
CNd::~CNd()
{
}
//-----------------------------------------------------------------------------
CVelocity2d CNd::getRecommendedVelocity()
{
  return CVelocity2d ( mVCmd, 0.0, mWCmd );
}
//-----------------------------------------------------------------------------
void CNd::setSafetyDistance ( float dist )
{
  mSafetyDist = dist;
  // Distance from the wheel to the front
  mNDparam.front = mFrontDim + mSafetyDist;
  // Distance from the wheel to the back
  mNDparam.back = mBackDim +  mSafetyDist;
  // Distance from the wheel to the left side (note robot is symetric, therefore
  // no need for right side
  mNDparam.left = mSideDim + mSafetyDist;

  // Pass the structure to ND for initialization
  InicializarND ( &mNDparam );
}
//-----------------------------------------------------------------------------
void CNd::setAvoidDistance ( float dist )
{
  mAvoidDist = dist;

  mNDparam.dsmax = mAvoidDist; // Security distance
  // Pass the structure to ND for initialization
  InicializarND ( &mNDparam );
}
//-----------------------------------------------------------------------------
void CNd::reset()
{
  mFgWaiting = false;
  mFgTurningInPlace = false;
  mFgStalled = false;
  mFgAtGoal = false;
}
//-----------------------------------------------------------------------------
void CNd::setEpsilonAngle ( float angle )
{
  mAngleEps = angle;
}
//-----------------------------------------------------------------------------
void CNd::setEpsilonDistance ( float dist )
{
  mDistEps = dist;
}
//-----------------------------------------------------------------------------
void CNd::addRangeFinder ( ARangeFinder* sensor )
{
  if ( mNumSensors < MAX_ND_SENSORS ) {
    mSensorList[mNumSensors] = sensor;
    mNumSensors++;
  }
}
//-----------------------------------------------------------------------------
bool CNd::atGoal()
{
  return mFgAtGoal;
}
//-----------------------------------------------------------------------------
bool CNd::isStalled()
{
  return mFgStalled;
}
//-----------------------------------------------------------------------------
bool CNd::hasActiveGoal()
{
  return mFgActiveGoal;
}
//-----------------------------------------------------------------------------
int CNd::getNumSectors()
{
  return SECTORES;
}
//-----------------------------------------------------------------------------
void CNd::setGoal ( CPose2d goal )
{
  mGoal = goal;
  mFgActiveGoal = true;
  mFgAtGoal = false;
  mFgStalled = false;
  mFgTurningInPlace = false;
  mTranslateStartTime = mCurrentTime;
  mLastRobotPose = mRobotPose;
}
//-----------------------------------------------------------------------------
void CNd::processSensors()
{
  float rx, ry;
  float globalSensorX, globalSensorY, globalSensorAngle;
  float cosR, sinR;
  float maxRange;

  // sin and cosin of robots headings
  sinR = sin ( mRobotPose.mYaw );
  cosR = cos ( mRobotPose.mYaw );

  mReadingIndex = 0;
  for ( int s = 0; s < mNumSensors; s++ ) {
    maxRange = mSensorList[s]->getMaxRange();
    for ( unsigned int i = 0; i < mSensorList[s]->getNumSamples(); i++ ) {

      if ( mSensorList[s]->mRangeData[i].range < maxRange ) {

        // calculate global sensor coordinates
        rx = mSensorList[s]->mRelativeBeamPose[i].mX;
        ry = mSensorList[s]->mRelativeBeamPose[i].mY;

        globalSensorX = mRobotPose.mX + rx * cosR - ry * sinR;
        globalSensorY = mRobotPose.mY + rx * sinR + ry * cosR;
        globalSensorAngle = NORMALIZE_TO_2PI ( mRobotPose.mYaw +
                                               mSensorList[s]->
                                               mRelativeBeamPose[i].mYaw );

        // convert to cartesian coords, in global coordinate system
        mObstacles.punto[mReadingIndex].x = globalSensorX +
                                            mSensorList[s]->mRangeData[i].range *
                                            cos ( globalSensorAngle );
        mObstacles.punto[mReadingIndex].y = globalSensorY +
                                            mSensorList[s]->mRangeData[i].range *
                                            sin ( globalSensorAngle );
        mReadingIndex ++;
      }

      if ( mReadingIndex >= MAX_POINTS_SCENARIO ) {
        mReadingIndex = 0;
        mFgReadingBufferInitialized = true;
      }
    }
    if ( mFgReadingBufferInitialized )
      mObstacles.longitud = MAX_POINTS_SCENARIO;
    else
      mObstacles.longitud = mReadingIndex;
  }
}
//-----------------------------------------------------------------------------
void CNd::update ( float timestamp, CPose2d robotPose,
                   CVelocity2d robotVelocity )
{
  TVelocities *cmdVel;
  TCoordenadas goal;
  TInfoMovimiento motionData;
  float gDx, gDa;

  // increment time
  mCurrentTime = timestamp;

  // are we waiting for a stall to clear?
  if ( mFgWaiting )
    return;

  // do we have a goal?
  if ( !mFgActiveGoal )
    return;

  // set robot pose in GLOBAL CS
  motionData.SR1.posicion.x = robotPose.mX;
  motionData.SR1.posicion.y = robotPose.mY;
  motionData.SR1.orientacion = robotPose.mYaw;
  // set velocity
  motionData.velocidades.v = robotVelocity.mVX;
  motionData.velocidades.w = robotVelocity.mYawDot;
  motionData.velocidades.v_theta = 0.0f;


  mRobotPose = robotPose;

  processSensors();

  if ( mObstacles.longitud == 0 ) {
    PRT_ERR1 ( "%s: No sensor data available, did you register a range finder ?",
               mRobotName.c_str() );
    return;
  }

  // TODO: put a smarter check earlier
  //assert( mObstacles.longitud <= MAX_POINTS_SCENARIO );

  // are we at the goal?
  gDx = hypot ( mGoal.mX - mRobotPose.mX,
                mGoal.mY - mRobotPose.mY );

  //gDa = angleDiff ( mGoal.mYaw, mRobotPose.mYaw );
  gDa = NORMALIZE_ANGLE(mGoal.mYaw - mRobotPose.mYaw);

  // Are we at the goal yet ??
  if ( ( gDx < mDistEps ) && ( fabs ( gDa ) < mAngleEps ) ) {
    mFgActiveGoal = false;
    mVCmd = 0.0f;
    mWCmd = 0.0f;
    PRT_MSG1 ( 6, "%s: At goal", mRobotName.c_str() );
    mFgAtGoal = true;
    return;
  }
  else {
    // are we close enough in distance?
    if ( ( gDx < mDistEps ) || ( mFgTurningInPlace ) ) {
      PRT_MSG1 ( 9, "%s: Turning in place", mRobotName.c_str() );
      mWCmd = mWMax * MAX(fabs(gDa) / PI, 0.1) * SIGN(gDa);
      mVCmd = 0.0;
/*
      // To make the robot turn (safely) to the goal orientation, we'll
      // give it a fake goal that is in the right direction, and just
      // ignore the translational velocity.
      goal.x = mRobotPose.mX + 10.0 * cos ( mGoal.mYaw );
      goal.y = mRobotPose.mY + 10.0 * sin ( mGoal.mYaw );

      // In case we went backward to get here, reverse direction so that we
      // can attain the goal heading
      setDirection ( FORWARD );

      cmdVel = IterarND ( goal,            // goal
                          mDistEps,        // goal tolerance
                          &motionData,     // current velocity of the robot
                          &mObstacles,     // list of the obstacle points in global cs
                          &mInfo );        // ND puts internal data in here

      if ( !cmdVel ) {
        // Emergency stop
        mVCmd = 0.0;
        mWCmd = 0.0;
        mFgStalled = true;
        mFgActiveGoal = false;
        PRT_MSG1 ( 6, "%s: Emergency stop", mRobotName.c_str() );
        return;
      }

      else {
        mFgStalled = false;
      }
      // we are turning in place, so ignore translational speed command
      mVCmd = 0.0f;
      mWCmd = cmdVel->w;
*/
      if ( not mFgTurningInPlace ) {
        // first time; cache the time and current heading error
        mRotateStartTime = mCurrentTime;
        mRotateMinError = fabs ( gDa );
        mFgTurningInPlace = true;
      }
      else {
        // Are we making progress?
        if ( fabs ( gDa ) < mRotateMinError ) {
          // yes; reset the time
          mRotateStartTime = mCurrentTime;
          mRotateMinError = fabs ( gDa );
        }
        else {
          // no; have we run out of time?
          if ( ( mCurrentTime - mRotateStartTime ) > mRotateStuckTime ) {
            PRT_MSG1 ( 6, "%s: Ran out of time trying to attain goal heading",
                       mRobotName.c_str() );
            mVCmd = 0.0f;
            mWCmd = 0.0f;
            mFgStalled = true;
            mFgActiveGoal = false;
            return;
          }
        }
      }
    }

    // we're far away; execute the normal ND loop
    else {
      // Have we moved far enough?
      float oDx = hypot ( mRobotPose.mX - mLastRobotPose.mX,
                          mRobotPose.mY - mLastRobotPose.mY );
      float oDa = NORMALIZE_ANGLE( mRobotPose.mYaw - mLastRobotPose.mYaw );

      if ( ( oDx > mTranslateStuckDist ) ||
           ( fabs ( oDa ) > mTranslateStuckAngle ) ) {
        mLastRobotPose = mRobotPose;
        mTranslateStartTime = mCurrentTime;
      }
      else {
        // Has it been long enough?
        if ( ( mCurrentTime - mTranslateStartTime ) > mTranslateStuckTime ) {
          PRT_MSG1 ( 6, "%s: ran out of time trying to get to goal",
                     mRobotName.c_str() );
          mVCmd = 0;
          mWCmd = 0;
          mFgStalled = true;
          mFgActiveGoal = false;
          return;
        }
      }

      // The current odometric goal
      goal.x = mGoal.mX;
      goal.y = mGoal.mY;

      // Were we asked to go backward?
      if ( mCurrentDir == BACKWARD ) {
        // Trick the ND by telling it that the robot is pointing the
        // opposite direction
        motionData.SR1.orientacion = NORMALIZE_TO_2PI ( motionData.SR1.orientacion + M_PI );
        // Also reverse the robot's geometry (it may be asymmetric
        // front-to-back)
        setDirection ( BACKWARD );
      }
      else
        setDirection ( FORWARD );

      cmdVel = IterarND ( goal,            // goal
                          mDistEps,        // goal tolerance
                          &motionData,     // current velocity of the robot
                          &mObstacles,     // list of the obstacle points in global cs
                          &mInfo );        //ND leaves internal data in here

      if ( !cmdVel ) {
        // Emergency stop
        mVCmd = 0.0f;
        mWCmd = 0.0f;
        mFgStalled = true;
        mFgActiveGoal = false;
        PRT_MSG1 ( 6, "%s: Emergency stop", mRobotName.c_str() );
        return;
      }
      else {

        mFgStalled = false;
        mVCmd = cmdVel->v;
        mWCmd = cmdVel->w;
      }
    } // far away loop

    if ( !mVCmd && !mWCmd ) {
      // ND is done, yet we didn't detect that we reached the goal.  How odd.
      mVCmd = 0.0f;
      mWCmd = 0.0f;
      mFgStalled = true;
      mFgActiveGoal = false;
      PRT_MSG1 ( 9, "%s: ND failed to reach goal ?!", mRobotName.c_str() );
      return;
    }
    else {
      mVCmd = threshold ( mVCmd, mVMin, mVMax );

      if ( !mVCmd )
        mWCmd = threshold ( mWCmd, mWMin, mWMax );
      // Were we asked to go backward?
      if ( mCurrentDir < 0 ) {
        // reverse the commanded x velocity
        mVCmd = -mVCmd;
      }
    } // if (!mVCmd && !mWCmd)
  }
}

//-----------------------------------------------------------------------------
float CNd::angleDiff ( float a, float b )
{
  float d1, d2;
  a = NORMALIZE_TO_2PI ( a );
  b = NORMALIZE_TO_2PI ( b );
  d1 = a - b;
  d2 = 2 * M_PI - fabs ( d1 );

  if ( d1 > 0 )
    d2 *= -1.0;

  if ( fabs ( d1 ) < fabs ( d2 ) ) {
    return ( d1 );
  }
  else {
    return ( d2 );
  }

}
//-----------------------------------------------------------------------------
void CNd::setDirection ( tDirection dir )
{
  if ( dir == mCurrentDir )
    return;

  if ( dir == FORWARD ) {
    // Distance to the front
    mNDparam.front = mFrontDim + mSafetyDist;

    // Distance to the back
    mNDparam.back = mBackDim + mSafetyDist;
    InicializarND ( &mNDparam );
  }
  else {
    // Distance to the front
    mNDparam.front = mBackDim + mSafetyDist;
    // Distance to the back
    mNDparam.back = mFrontDim + mSafetyDist;
    InicializarND ( &mNDparam );
  }

  mCurrentDir = dir;
}
//-----------------------------------------------------------------------------
float CNd::threshold ( float v, float vMin, float vMax )
{
  if ( isAboutZero ( v ) )
    return ( v );
  else
    if ( v > 0.0 ) {
      v = MIN ( v, vMax );
      v = MAX ( v, vMin );
      return ( v );
    }
    else {
      v = MAX ( v, -vMax );
      v = MIN ( v, -vMin );
      return ( v );
    }
}
//-----------------------------------------------------------------------------


