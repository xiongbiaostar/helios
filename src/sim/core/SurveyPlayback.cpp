#include "logging.hpp"
#include <string>
#include <iomanip>

#include <memory>
#include <chrono>
using namespace std::chrono;

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "AbstractDetector.h"
#include "SurveyPlayback.h"
#include <glm/gtx/vector_angle.hpp>
#include "HelicopterPlatform.h"
#include <platform/InterpolatedMovingPlatform.h>
#include <ScanningStrip.h>
#include <filems/facade/FMSFacade.h>
#include <filems/write/comps/SimpleSyncFileWriter.h>

using helios::filems::SimpleSyncFileWriter;

using namespace std;

SurveyPlayback::SurveyPlayback(
    shared_ptr<Survey> survey,
    shared_ptr<FMSFacade> fms,
    int const parallelizationStrategy,
    std::shared_ptr<PulseThreadPoolInterface> pulseThreadPoolInterface,
    int const chunkSize,
    std::string fixedGpsTimeStart,
    bool exportToFile
):
    Simulation(
        parallelizationStrategy,
        pulseThreadPoolInterface,
        chunkSize,
        fixedGpsTimeStart
    ),
    fms(fms)
{
    this->mSurvey = survey;
    this->mSurvey->hatch(*this);
	this->exitAtEnd = true;
	this->exportToFile = exportToFile;
	this->setScanner(mSurvey->scanner);

	// ############### BEGIN If the leg has no survey defined, create a default one ################
	if (mSurvey->legs.size() == 0) {
		shared_ptr<Leg> leg(new Leg());

		// Set leg scanner settings:
		leg->mScannerSettings = make_shared<ScannerSettings>();

		// Set leg position to the center of the scene:
		shared_ptr<PlatformSettings> ps = make_shared<PlatformSettings>();
		ps->setPosition(
		    mSurvey->scanner->platform->scene->getAABB()->getCentroid()
        );
		leg->mPlatformSettings = ps;

		// Add leg to survey:
		mSurvey->addLeg(0, leg);
	}
	// ############### END If the leg has no survey defined, create a default one ################

    // If we start a new scan, move platform to destination of first leg:
    if(!mSurvey->legs.empty()) {
        getScanner()->platform->setPosition(
            mSurvey->legs[0]->mPlatformSettings->getPosition()
        );
    }

    // If platform is interpolated from data, sync GPS time if requested
    shared_ptr<InterpolatedMovingPlatform> imp =
        dynamic_pointer_cast<InterpolatedMovingPlatform>(
            getScanner()->platform
        );
    if(imp != nullptr && imp->isSyncGPSTime()){
        this->currentGpsTime_ms = imp->getStartTime() * 1000.0;
    }

	// Orientate platform and start first leg
	startLeg(0, true);

	// For progress tracking
	numEffectiveLegs = mSurvey->legs.size();
    if(getScanner()->platform->canMove()) {
		mSurvey->calculateLength();
		numEffectiveLegs--;
	}
}


void SurveyPlayback::estimateTime(int legCurrentProgress, bool onGround, double legElapsedLength) {
    if (legCurrentProgress > legProgress) {	// Do stuff only if leg progress incremented at least 1%
		legProgress = (double) legCurrentProgress;

		long currentTime = duration_cast<milliseconds>(
		    system_clock::now().time_since_epoch()).count();
		legElapsedTime_ms = currentTime - legStartTime_ns;
		legRemainingTime_ms = (long)((100 - legProgress) / legProgress
		    * legElapsedTime_ms);

		if (!getScanner()->platform->canMove()) {
			progress = ((mCurrentLegIndex * 100) + legProgress) /
			    (double) numEffectiveLegs;
		}
		else {
			progress = (elapsedLength + legElapsedLength) * 100
			    / (double) mSurvey->getLength();
		}
		elapsedTime_ms = currentTime - timeStart_ms;
		remainingTime_ms = (long)((100 - progress) /
		    progress * elapsedTime_ms);

        if(legProgress == 99){
            std::stringstream ss;
            ss << "Final yaw = " <<
                mSurvey->scanner->platform->getHeadingRad();
            logging::TRACE(ss.str());

        }
        ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Survey " << progress << "%\tElapsed "
            << milliToString(elapsedTime_ms) <<
            " Remaining " << milliToString(remainingTime_ms) << endl;
        oss << "Leg" << (mCurrentLegIndex+1) << "/" << (numEffectiveLegs)
            << " " << legProgress << "%\tElapsed "
            << milliToString(legElapsedTime_ms) << " Remaining "
            << milliToString(legRemainingTime_ms);
        logging::INFO(oss.str());
	}
}

void SurveyPlayback::trackProgress() {
	if(!getScanner()->platform->canMove()){
		double legElapsedAngle = std::fabs(
		    getScanner()->scannerHead->getRotateStart() -
		    getScanner()->scannerHead->getRotateCurrent()
        );
		int legProgress = (int)(legElapsedAngle * 100 / getScanner()->scannerHead->getRotateRange());
		estimateTime(legProgress, true, 0);
	}
	else if (mCurrentLegIndex < mSurvey->legs.size() - 1) {
		double legElapsedLength = glm::distance(
		    getCurrentLeg()->mPlatformSettings->getPosition(),
		    mSurvey->scanner->platform->getPosition()
        );
		int legProgress = (int)
		    (legElapsedLength * 100 / getCurrentLeg()->getLength());
		estimateTime(legProgress, false, legElapsedLength);
	}
}

void SurveyPlayback::doSimStep() {
	if (!mLegStarted) {
		mLegStarted = true;
		if(exportToFile) clearPointcloudFile();

		legProgress = 0;
		legStartTime_ns = duration_cast<milliseconds>(
		        system_clock::now().time_since_epoch()
        ).count();
	}

	trackProgress();

	//if(legProgress <= 20) // Profiling only (uncomment for profiling)
    Simulation::doSimStep();
	//else onLegComplete(); // Profiling only (uncomment for profiling)
}

shared_ptr<Leg> SurveyPlayback::getCurrentLeg() {
	if (mCurrentLegIndex < mSurvey->legs.size()) {
		return mSurvey->legs.at(mCurrentLegIndex);
	}
	// NOTE: This should never happen:
	logging::ERR("ERROR getting current leg: Index out of bounds");
	return nullptr;
}

shared_ptr<Leg> SurveyPlayback::getPreviousLeg(){
    if(mCurrentLegIndex == 0) return nullptr;
    return mSurvey->legs.at(mCurrentLegIndex - 1);
}

int SurveyPlayback::getCurrentLegIndex() {
	return mCurrentLegIndex;
}

string SurveyPlayback::getLegOutputPrefix(std::string format){
    std::shared_ptr<Leg> leg = getCurrentLeg();
    std::shared_ptr<ScanningStrip> strip = leg->getStrip();

    // Mark leg as processed
    leg->wasProcessed = true;

    stringstream ss;
    if(strip != nullptr){ // Handle prefix when leg belongs to a split
        ss << "strip"
           << boost::str(boost::format(format) % strip->getStripId());
    }
    else{ // Handle prefix when leg does not belong to a split
        ss << "leg"
           << boost::str(boost::format(format) % leg->getSerialId());
    }
    return ss.str();
}

void SurveyPlayback::onLegComplete() {
    // Do scanning pulse process handling of on leg complete, if any
    mScanner->onLegComplete();

    // Notify detector about leg completion
    mScanner->detector->onLegComplete();

	// Start next leg
    elapsedLength += mSurvey->legs.at(mCurrentLegIndex)->getLength();
	startNextLeg(false);
}

void SurveyPlayback::startLeg(unsigned int legIndex, bool manual) {
	if (legIndex < 0 || legIndex >= mSurvey->legs.size()) {
		return;
	}

	ostringstream oss;oss << "Starting leg " << legIndex << endl;
	logging::INFO(oss.str());
	mLegStarted = false;
	mCurrentLegIndex = legIndex;
	shared_ptr<Leg> leg = getCurrentLeg();

	// Apply scanner settings:
	if (leg->mScannerSettings != nullptr) {
		mSurvey->scanner->applySettings(leg->mScannerSettings);
	}
	shared_ptr<Platform> platform(getScanner()->platform);
    mSurvey->scanner->lastTrajectoryTime = 0L;

	// Apply platform settings:
	if (leg->mPlatformSettings != nullptr) {
		platform->applySettings(leg->mPlatformSettings, manual);

		// ################ BEGIN Set platform destination ##################
		unsigned int nextLegIndex = legIndex + 1;

		if (nextLegIndex < mSurvey->legs.size()) {
			// Set destination to position of next leg:
			shared_ptr<Leg> nextLeg = mSurvey->legs.at(nextLegIndex);
            if(
                leg->mTrajectorySettings != nullptr &&
                leg->mTrajectorySettings->teleportToStart
            ){
                platform->setPosition(
                    nextLeg->mPlatformSettings->getPosition()
                );
                platform->setOrigin(
                    nextLeg->mPlatformSettings->getPosition()
                );
                platform->setDestination(
                    nextLeg->mPlatformSettings->getPosition()
                );
            }
            else{
                platform->setOrigin(leg->mPlatformSettings->getPosition());
                if(
                    nextLeg->mTrajectorySettings != nullptr &&
                    nextLeg->mTrajectorySettings->teleportToStart
                ){
                    // If next leg teleports to start, current leg is stop leg
                    // Thus, set stop origin and destination to the same point
                    platform->setDestination(
                        leg->mPlatformSettings->getPosition()
                    );
                }
                else{
                    platform->setDestination(
                        nextLeg->mPlatformSettings->getPosition()
                    );
                }
            }
            if(nextLegIndex + 1 < mSurvey->legs.size()){
                platform->setAfterDestination(
                    mSurvey->legs.at(nextLegIndex + 1)
                        ->mPlatformSettings->getPosition()
                );
            }
            else{
                platform->setAfterDestination(
                    nextLeg->mPlatformSettings->getPosition()
                );
            }
            stringstream ss;
            ss  << "Leg" << legIndex << " waypoints:\n"
                << "\tOrigin: (" << platform->originWaypoint.x << ", "
                    << platform->originWaypoint.y << ", "
                    << platform->originWaypoint.z << ")\n"
                << "\tTarget: (" << platform->targetWaypoint.x << ", "
                    << platform->targetWaypoint.y << ", "
                    << platform->targetWaypoint.z << ")\n"
                << "\tNext: (" << platform->nextWaypoint.x << ", "
                    << platform->nextWaypoint.y << ", "
                    << platform->nextWaypoint.z << ")" << std::endl;
            logging::INFO(ss.str());
		}

        if(platform->canStopAndTurn() && leg->mPlatformSettings->stopAndTurn)
            stopAndTurn(legIndex, leg);
		else if(manual) platform->initLegManual();
        else platform->initLeg();
        try{ // Transform trajectory time (if any) to simulation time
            if(
                leg->mTrajectorySettings != nullptr &&
                leg->mTrajectorySettings->hasStartTime()
            ){
                std::shared_ptr<InterpolatedMovingPlatform> imp =
                    dynamic_pointer_cast<InterpolatedMovingPlatform>(platform);
                imp->toTrajectoryTime(leg->mTrajectorySettings->tStart);
            }
        }catch(...) {}

		// ################ END Set platform destination ##################
	}

	// Restart deflector if previous leg was not active
	shared_ptr<Leg> previousLeg = getPreviousLeg();
    if(
	    previousLeg != nullptr && !previousLeg->mScannerSettings->active &&
	    leg->mScannerSettings->active
    ){
        mSurvey->scanner->beamDeflector->restartDeflector();
	}


	if(exportToFile) prepareOutput();
    platform->writeNextTrajectory = true;
}

void SurveyPlayback::startNextLeg(bool manual) {
	if (mCurrentLegIndex < mSurvey->legs.size() - 1) {
		// If there are still legs left, start the next one:
		startLeg(mCurrentLegIndex + 1, manual);
	}
	else {
		// If this was the final leg, stop the simulation:
		if (exitAtEnd) {
			shutdown();
			stop();
		}
		else {
			pause(true);
		}
	}
}

void SurveyPlayback::shutdown() {
	Simulation::shutdown();
	mSurvey->scanner->detector->shutdown();
}

string SurveyPlayback::milliToString(long millis) {
	long seconds = (millis / 1000) % 60;
	long minutes = (millis / (60000)) % 60;
	long hours = (millis / (3600000)) % 24;
	long days = millis / 86400000;
	return boost::str(
	    boost::format("%02d %02d:%02d:%02d") % days % hours % minutes % seconds
    );
}

void SurveyPlayback::stopAndTurn(
    unsigned int legIndex,
    std::shared_ptr<Leg> leg
){
    if(legIndex == mSurvey->legs.size()-1) return;
    glm::dvec3 currentPos = leg->mPlatformSettings->getPosition();
    glm::dvec3 nextPos =
        mSurvey->legs[legIndex+1]->mPlatformSettings->getPosition();
    glm::dvec3 xyDir = nextPos-currentPos;
    xyDir.z = 0;
    xyDir = glm::normalize(xyDir);
    glm::dvec3 xyRef = glm::dvec3(0,1,0);
    double sign = (xyDir.x >= 0) ? 1 : -1;
    // sign*acos(-dotProduct/magnitudeProduct)
    double angle = sign*std::acos(
        -glm::dot(xyRef,xyDir) / glm::l2Norm(xyRef) / glm::l2Norm(xyDir)
    );

    mSurvey->scanner->platform->setHeadingRad(angle);
    HelicopterPlatform *hp = dynamic_cast<HelicopterPlatform*>(
        mSurvey->scanner->platform.get());
    if(hp!=nullptr){
        std::shared_ptr<PlatformSettings> lps = leg->mPlatformSettings;
        if(lps->yawAtDepartureSpecified) hp->yaw = lps->yawAtDeparture;
        hp->pitch = 0.0;
        hp->speed_xy.x = 0.0001;
        hp->speed_xy.y = 0.0001;
    }
    std::stringstream ss;
    ss << "stop and turn yaw = " << angle;
    logging::TRACE(ss.str());
}

void SurveyPlayback::prepareOutput(){
    // Check leg is active, otherwise there is no output to prepare
    std::shared_ptr<Leg> leg = getCurrentLeg();
    if(!leg->getScannerSettings().active) return;

    // Check if all the legs in the rip were processed
    std::shared_ptr<ScanningStrip> strip = leg->getStrip();
    bool lastLegInStrip = true;
    if(strip != nullptr){
        lastLegInStrip = getCurrentLeg()->getStrip()->isLastLegInStrip();
    }

    // Configure output paths
    fms->write.configure(
        getLegOutputPrefix(),
        getScanner()->isWriteWaveform(),
        lastLegInStrip
    );

    // Handle historical tracking of output paths
    getScanner()->trackOutputPath(
        fms->write.getMeasurementWriterOutputPath().string()
    );
}

void SurveyPlayback::clearPointcloudFile(){
    // Dont clear strip file, it would overwrite previous point cloud content
    if(getCurrentLeg()->isContainedInAStrip()) return;
    // Dont clear pcloud file, if leg is not active there is nothing to clear
    // Otherwise, WATCHOUT because last active leg might be overwriten
    if(!getCurrentLeg()->mScannerSettings->active) return;
    fms->write.clearPointcloudFile();
}

