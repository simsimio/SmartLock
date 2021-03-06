//
//
//  Generated by StarUML(tm) C++ Add-In
//
//  @ Project : Untitled
//  @ File Name : LockSystemController.h
//  @ Date : 7/19/2015
//  @ Author : 
//
//


#if !defined(_LOCKSYSTEMCONTROLLER_H)
#define _LOCKSYSTEMCONTROLLER_H

#define LOCK_SYSTEM_CONTROLLER_DEBUG

#include "Runnable.h"
#include "Cartesian.h"
#include "AxisPairs.h"
#include <Arduino.h>
#include <stdint.h>
class MotorController;
class LockAccelerometerObserver;

// TBD - incorporate reading calibration status from EEPROM
// Shortcut - for now we'll start up in Auto->Locked state - need to add
// calibration code and EEPROM store/read

class LockSystemController : public Runnable {
public:
	enum MainState {Initial, Automatic, Manual, Fault};
	enum InitState {uncalibrated, calibratingStartFromUnlockedPos, calibratingStartFromLockedPos, waitForCW, waitForCCW, calibrated, failed};
	enum AutoState {locked, unlocked, locking, unlocking, stuck, init};
/* TBD
	enum FaultState {};
	enum ManState {};
*/
	enum Direction { CW, CCW };

	LockSystemController(
			MotorController * mc,
			LockAccelerometerObserver * obs,
			bool angleIncrCW = true,
			int lockedPosition = 180,
			int unlockedPosition = 0, uint8_t defaultPower = 200,
			int checkRotationMinProgress = 30,
			int testPower = 250,
			int testTime = 3500);
	void cmdCalibAtLockedPos();
	void cmdCalibAtUnlockedPos();
	void motorComplete();
	void motorStuck();
	void cmdLock();
	void cmdUnlock();
	void sleep();
	void wakeUp();
	void timeSlice();
	int isLocked();
	void stateInfo();
	bool angleIncrCW;
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	static const PROGMEM char * const SPACE;
	static const PROGMEM char * const S_CW;
	static const PROGMEM char * const S_CCW;
#endif

private:
	void tsInit();
	void tsAuto();
	void tsFault();
	void tsManual();
	int lockedPosition;
	int unlockedPosition;
	MotorController * motorCont;
	LockAccelerometerObserver * laObs;
	uint8_t drivePower;
	MainState mainState;
	InitState initState;
	AutoState autoState;
	AxisPairs findAxes();
	void newMainState(MainState state);
	void newInitState(InitState state);
	void newAutoState(AutoState state);
	void startRotation(Direction d);
	void updateCalibData();
	void setLockUnlockPositions();

	struct {
		Cartesian lockCart;
		Cartesian unlockCart;
		int lockedPosition_xz;
		int lockedPosition_yz;
		int unlockedPosition_xz;
		int unlockedPosition_yz;
		Cartesian absDeltas;
		Cartesian lastReading;
		Cartesian maxVals;
		Cartesian minVals;
	} calibrationData;

	struct {
		int checkRotationMinProgress;
		int testPower;
		int testTime;

		int checkRotationStartAngle;
		int testStartTime;
	} bearings;

};

#endif  //_LOCKSYSTEMCONTROLLER_H
