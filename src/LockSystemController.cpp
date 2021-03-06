//
//
//  Generated by StarUML(tm) C++ Add-In
//
//  @ Project : Untitled
//  @ File Name : LockSystemController.cpp
//  @ Date : 7/19/2015
//  @ Author : 
//
//


#include <Arduino.h>
#include "LockSystemController.h"
#include "LockAccelerometerObserver.h"
#include "MotorController.h"
#include "Debug.h"
#include "Cartesian.h"
//#include "StringConsts.h"

#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
const PROGMEM char * const LockSystemController::SPACE = " ";
const PROGMEM char * const LockSystemController::S_CW = "CW";
const PROGMEM char * const LockSystemController::S_CCW = "CCW";
#endif

LockSystemController::LockSystemController(
		MotorController * mc,
		LockAccelerometerObserver * obs,
		bool angleIncrCW,
		int lockedPosition,
		int unlockedPosition, uint8_t  defaultPower,
		int checkRotationMinProgress,
		int testPower,
		int testTime) :
	motorCont(mc),
	laObs(obs),
	angleIncrCW(angleIncrCW),
	lockedPosition(lockedPosition), unlockedPosition(unlockedPosition),
	drivePower(defaultPower),
	mainState(Initial),
	autoState(init),
	initState(uncalibrated),
	bearings ({ 
			checkRotationMinProgress,
			testPower,
			testTime,
			0,
			0 })
{
	mc->init(this);
}

void LockSystemController::timeSlice()
{
	// TBD - nothing for now
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	static int counter = 0;
	if (++counter % 100 == 1) {
		stateInfo();
	}
#endif
	switch (mainState) {
		case Initial:
			tsInit();
			return;
			break;
		case Automatic:
			tsAuto();
			return;
			break;
		case Fault:
			tsFault();
			return;
			break;
		case Manual:
			tsManual();
			return;
			break;
	}
}

void LockSystemController::updateCalibData()
{
	if (initState == uncalibrated) {
		return;
	}

	Cartesian curCart;
	laObs->getCart(&curCart);

	if (calibrationData.maxVals.x < curCart.x) { calibrationData.maxVals.x = curCart.x; }
	if (calibrationData.maxVals.y < curCart.y) { calibrationData.maxVals.y = curCart.y; }
	if (calibrationData.maxVals.z < curCart.z) { calibrationData.maxVals.z = curCart.z; }

	if (calibrationData.minVals.x > curCart.x) { calibrationData.minVals.x = curCart.x; }
	if (calibrationData.minVals.y > curCart.y) { calibrationData.minVals.y = curCart.y; }
	if (calibrationData.minVals.z > curCart.z) { calibrationData.minVals.z = curCart.z; }

	calibrationData.absDeltas.x += abs(curCart.x - calibrationData.lastReading.x);
	calibrationData.absDeltas.y += abs(curCart.y - calibrationData.lastReading.y);
	calibrationData.absDeltas.z += abs(curCart.z - calibrationData.lastReading.z);

	calibrationData.lastReading = curCart;
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	static long lastPrint_ms = 0;
	if (millis() - lastPrint_ms > 500) {
		debugPrint(F("LSC:calib minVals "));
		debugPrint(calibrationData.minVals.x); debugPrint(SPACE);
		debugPrint(calibrationData.minVals.y); debugPrint(SPACE);
		debugPrintln(calibrationData.minVals.z);

		debugPrint(F("LSC:calib maxVals "));
		debugPrint(calibrationData.maxVals.x); debugPrint(SPACE);
		debugPrint(calibrationData.maxVals.y); debugPrint(SPACE);
		debugPrintln(calibrationData.maxVals.z);

		debugPrint(F("LSC:calib absDeltas "));
		debugPrint(calibrationData.absDeltas.x); debugPrint(SPACE);
		debugPrint(calibrationData.absDeltas.y); debugPrint(SPACE);
		debugPrintln(calibrationData.absDeltas.z);

		lastPrint_ms = millis();
	}
#endif
}

void LockSystemController::tsInit()
{
	switch (initState) {
		case uncalibrated:
			return;
			break;
		case calibratingStartFromUnlockedPos:
		case calibratingStartFromLockedPos:
			updateCalibData();
			break;
		case waitForCW:
			{
				int deltaCW = laObs->getLockAngleDeg() - bearings.checkRotationStartAngle;
				if (abs(deltaCW) >= bearings.checkRotationMinProgress)
				{
					motorCont->cmdStop();
					if (deltaCW > 0)
					{
						angleIncrCW = true;
					}
					else
					{
						angleIncrCW = false;
					}
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:waitForCW "));
					debugPrintln(angleIncrCW);
#endif
					newInitState(calibrated);
				}
				else if (millis() - bearings.testStartTime > bearings.testTime)
				{
					motorCont->cmdStop();
					newInitState(waitForCCW);
					startRotation(CCW);
				}
			}
			break;
		case waitForCCW:
			{
				int deltaCCW = laObs->getLockAngleDeg() - bearings.checkRotationStartAngle;
				if (abs(deltaCCW) >= bearings.checkRotationMinProgress)
				{
					motorCont->cmdStop();
					if (deltaCCW > 0)
					{
						angleIncrCW = false;
					}
					else
					{
						angleIncrCW = true;
					}
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:waitForCCW "));
					debugPrintln(angleIncrCW);
#endif
					newInitState(calibrated);
				}
				else if (millis() - bearings.testStartTime > bearings.testTime)
				{
					motorCont->cmdStop();
					newInitState(failed);
				}
			}
			break;
		case calibrated:
			debugPrintln(F("calibration success"));
			newMainState(Automatic);
			cmdLock();
			newAutoState(locked);
			break;
		case failed:
			debugPrintln(F("calibration failed"));
			break;
	}
}


void LockSystemController::tsAuto()
{
	// TBD - timeout on MotorController responses, etc
}

void LockSystemController::tsManual()
{
	// TBD - figure out when manual intervention has ended, etc.
}

void LockSystemController::tsFault()
{
	// TBD - determine recovery / retry times etc
}

void LockSystemController::stateInfo()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:si "));
#endif
	switch (mainState) {
		case Initial: debugPrint(F("Initial/")); break;
		case Automatic: debugPrint(F("Automatic/")); break;
		case Manual: debugPrint(F("Manual/")); break;
		case Fault: debugPrint(F("Fault/")); break;
	}

	switch (initState) {
		case uncalibrated: debugPrint(F("uncalibrated/")); break;
		case calibratingStartFromUnlockedPos: debugPrint(F("calibratingStartFromUnlockedPos/")); break;
		case calibratingStartFromLockedPos: debugPrint(F("calibratingStartFromLockedPos/")); break;
		case waitForCW: debugPrint(F("waitForCW/")); break;
		case waitForCCW: debugPrint(F("waitForCCW/")); break;
		case calibrated: debugPrint(F("calibrated/")); break;
		case failed: debugPrint(F("failed/")); break;
	}

	switch (autoState) {
		case locked: debugPrintln(F("locked")); break;
		case unlocked: debugPrintln(F("unlocked")); break;
		case locking: debugPrintln(F("locking")); break;
		case unlocking: debugPrintln(F("unlocking")); break;
		case stuck: debugPrintln(F("stuck")); break;
		case init: debugPrintln(F("init")); break;
	}
}

AxisPairs LockSystemController::findAxes()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:findAxes "));
#endif
	int deltaX = calibrationData.maxVals.x - calibrationData.minVals.x;
	int deltaY = calibrationData.maxVals.y - calibrationData.minVals.y;

	if (deltaX < deltaY)
	{
		laObs->setDefaultAxes(yz);
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
		debugPrintln(F("yz"));
#endif
		return yz;
	}
	else if (deltaY < deltaX)
	{
		laObs->setDefaultAxes(xz);
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
		debugPrintln(F("xz"));
#endif
		return xz;
	}
	else
	{
		//TBD: calibrationFailed();
	}
}

void LockSystemController::setLockUnlockPositions()
{
	// now we just need the angles calculated correctly per these settings
	// get them from the observer
	if (findAxes() == xz)
	{
		lockedPosition = calibrationData.lockedPosition_xz;
		unlockedPosition = calibrationData.unlockedPosition_xz;
	}
	else
	{
		lockedPosition = calibrationData.lockedPosition_yz;
		unlockedPosition = calibrationData.unlockedPosition_yz;
	}
}

void LockSystemController::cmdCalibAtLockedPos()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrintln(F("LSC:cmdCalibAtLockedPos"));
#endif
	switch (mainState) {
		case Initial:
			// regardlesss of calibration start or end, we are at locked position, record it
			calibrationData.lockedPosition_xz = laObs->getLockAngleDeg(xz);
			calibrationData.lockedPosition_yz = laObs->getLockAngleDeg(yz);
			laObs->getCart(&calibrationData.lockCart);
			switch (initState) {
				case uncalibrated:
				case calibratingStartFromLockedPos:
					// we are just starting the calibration cycle
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:calib start lck xz yz "));
					debugPrint(calibrationData.lockedPosition_xz); debugPrint(SPACE);
					debugPrintln(calibrationData.lockedPosition_yz);
#endif
					calibrationData.lastReading = calibrationData.lockCart;
					newInitState(calibratingStartFromLockedPos);
					break;
				case calibratingStartFromUnlockedPos:
					// we've recorded the other side already, do
					// all the calibration calculations and store
					// the positions
					setLockUnlockPositions();
					// TBD settings.lockedAngle.setData(lockedPosition);
					// TBD settings.unlockedAngle.setData(unlockedPosition);

#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:calib lck end xz yz "));
					debugPrint(calibrationData.lockedPosition_xz); debugPrint(SPACE);
					debugPrintln(calibrationData.lockedPosition_yz);
#endif

					// TBD settings.lockedAngle.setData(lockedPosition);
					// TBD settings.unlockedAngle.setData(unlockedPosition);
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:calibration complete lock "));
					debugPrint(lockedPosition);
					debugPrint(F(" unlock "));
					debugPrintln(unlockedPosition);
					debugPrintln(F("LSC:testing orientation"));
#endif
					startRotation(CW);
					newInitState(waitForCW);
					break;
			}
			break;
	}
}

void LockSystemController::cmdCalibAtUnlockedPos()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrintln(F("LSC:cmdCalibAtUnlockedPos"));
#endif
	switch (mainState) {
		case Initial:
			// regardlesss of calibration start or end, we are at unlocked position, record it
			calibrationData.unlockedPosition_xz = laObs->getLockAngleDeg(xz);
			calibrationData.unlockedPosition_yz = laObs->getLockAngleDeg(yz);
			laObs->getCart(&calibrationData.unlockCart);
			switch (initState) {
				case uncalibrated:
				case calibratingStartFromUnlockedPos:
					// we are just starting the calibration cycle
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:calib start ulk xz yz "));
					debugPrint(calibrationData.unlockedPosition_xz); debugPrint(SPACE);
					debugPrintln(calibrationData.unlockedPosition_yz);
#endif
					calibrationData.lastReading = calibrationData.unlockCart;
					newInitState(calibratingStartFromUnlockedPos);
					break;
				case calibratingStartFromLockedPos:
					// we've recorded the other side already, do
					// all the calibration calculations and store
					// the positions
					setLockUnlockPositions();

					// TBD settings.lockedAngle.setData(lockedPosition);
					// TBD settings.unlockedAngle.setData(unlockedPosition);
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
					debugPrint(F("LSC:calibration complete lock "));
					debugPrint(lockedPosition);
					debugPrint(F(" unlock "));
					debugPrintln(unlockedPosition);
					debugPrintln(F("LSC:testing orientation"));
#endif
					startRotation(CW);
					newInitState(waitForCW);
					break;
			}
			break;
	}
}


/*
   void LockSystemController::setCheckConsts()
   {
   bearings.checkRotationMinProgress = 30;
   bearings.testPower = 200;
   bearings.testTime = 3000;
   }
   */

void LockSystemController::startRotation(Direction d)
{
	//setCheckConsts();
	bearings.checkRotationStartAngle = laObs->getLockAngleDeg();
	bearings.testStartTime = millis();

	MotorController::Direction dir = (d == CW) ? MotorController::CW : MotorController::CCW;
	motorCont->cmdDriveMotorForDuration(dir, bearings.testPower, bearings.testTime);
}

void LockSystemController::motorComplete() {
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrintln(F("LSC:motorComplete"));
#endif
	switch (mainState) {
		case Automatic:
			{
				switch (autoState) {
					case locking:
						{
							newAutoState(locked);
						}
						break;
					case unlocking:
						{
							newAutoState(unlocked);
						}
						break;
				}
			}
			break;
	}
}

void LockSystemController::motorStuck() {
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrintln(F("LSC:motorStuck"));
#endif
	switch (mainState) {
		case Automatic:
			{
				switch (autoState) {
					case locking:
						{
							newAutoState(stuck);
							newMainState(Fault);
						}
						break;
					case unlocking:
						{
							newAutoState(stuck);
							newMainState(Fault);
						}
						break;
				}
			}
			break;
	}
}

void LockSystemController::cmdLock() {
	int curAngle = laObs->getLockAngleDeg();
	MotorController::Direction dir;
	if (lockedPosition > curAngle)
	{
		dir = (angleIncrCW) ? MotorController::CW : MotorController::CCW;
	}
	else
	{
		dir = (angleIncrCW) ? MotorController::CCW : MotorController::CW;
	}
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:cmdLock dir pwr pos ")); debugPrint(dir == MotorController::CW ? S_CW : S_CCW); debugPrint(SPACE);
	debugPrint(drivePower); debugPrint(SPACE);
	debugPrintln(lockedPosition);
#endif
	motorCont->cmdDriveMotorToPosition(dir, drivePower, lockedPosition);
}

void LockSystemController::cmdUnlock() {
	int curAngle = laObs->getLockAngleDeg();
	MotorController::Direction dir;
	if (unlockedPosition > curAngle)
	{
		dir = (angleIncrCW) ? MotorController::CW : MotorController::CCW;
	}
	else
	{
		dir = (angleIncrCW) ? MotorController::CCW : MotorController::CW;
	}
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:cmdUnlock dir pwr pos ")); debugPrint(dir == MotorController::CW ? S_CW : S_CCW); debugPrint(SPACE);
	debugPrint(drivePower); debugPrint(SPACE);
	debugPrintln(unlockedPosition);
#endif
	motorCont->cmdDriveMotorToPosition(dir, drivePower, unlockedPosition);
}

void LockSystemController::sleep()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrintln(F("LSC:sleep"));
#endif
	motorCont->sleep();
}

void LockSystemController::wakeUp()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrintln(F("LSC:wakeUp"));
#endif
	motorCont->wakeUp();
}

int LockSystemController::isLocked()
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:isLocked locked: "));
	debugPrintln(mainState == Automatic && autoState == locked);
#endif
	return (mainState == Automatic && autoState == locked);
}

void LockSystemController::newMainState(MainState state)
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:newMainState before "));
	stateInfo();
#endif
	mainState = state;
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:newMainState after "));
	stateInfo();
#endif
}

void LockSystemController::newInitState(InitState state)
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:newInitState before "));
	stateInfo();
#endif
	initState = state;
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:newInitState after "));
	stateInfo();
#endif
}

void LockSystemController::newAutoState(AutoState state)
{
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:newAutoState before "));
	stateInfo();
#endif
	autoState = state;
#ifdef LOCK_SYSTEM_CONTROLLER_DEBUG
	debugPrint(F("LSC:newAutoState after "));
	stateInfo();
#endif
}
