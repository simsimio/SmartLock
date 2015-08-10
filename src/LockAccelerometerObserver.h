//
//
//  Generated by StarUML(tm) C++ Add-In
//
//  @ Project : Untitled
//  @ File Name : LockAccelerometerObserver.h
//  @ Date : 7/14/2015
//  @ Author : 
//
//


#if !defined(_LOCKACCELEROMETEROBSERVER_H)
#define _LOCKACCELEROMETEROBSERVER_H

#include "AccelerometerObserver.h"

class LockAccelerometerObserver : public AccelerometerObserver {
public:
	LockAccelerometerObserver(AccelerometerSubject * subject, float alpha = 0.2, bool xyreverse = false);
	virtual int getLockAngleDeg();
	// TBD virtual void getDoorPosition();
	// TBD virtual void getLockOmega();
	// TBD virtual void getLockAlpha();
	virtual void Update();
protected:
	int revs;
	int angle;
	int lastAngle;
	bool xyreverse;

};

#endif  //_LOCKACCELEROMETEROBSERVER_H
