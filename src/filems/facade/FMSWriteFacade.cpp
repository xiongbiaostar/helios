#include <FMSWriteFacade.h>

#include <boost/filesystem.hpp>

#include <sstream>

using namespace helios::filems;

namespace fs=boost::filesystem;

using std::stringstream;


// ***  FACADE WRITE METHODS  *** //
// ****************************** //
void FMSWriteFacade::configure(
    string const &parent,
    string const &prefix,
    bool const computeWaveform,
    bool const lastLegInStrip
){
    // Configure measurement output path
    getMeasurementWriter()->configure(parent, prefix, lastLegInStrip);

    // Configure trajectory output path
    getTrajectoryWriter()->configure(parent, prefix);

    // Configure full waveform output path
    getFullWaveformWriter()->configure(parent, prefix, computeWaveform);

}

// ***  FACADE MEASUREMENT WRITE METHODS  *** //
// ****************************************** //
void FMSWriteFacade::validateMeasurementWriter(
    string const &callerName,
    string const &errorMsg
) const{
    // Check measurement writer does exist
    if(mw==nullptr){
        stringstream ss;
        ss  << callerName << " " << errorMsg << " because it does not exist";
        throw HeliosException(ss.str());
    }
}

void FMSWriteFacade::writeMeasurement(Measurement &m){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::writeMeasurement",
        "could not write measurement"
    );
    // Write the measurement
    mw->writeMeasurement(m);
}

void FMSWriteFacade::finishMeasurementWriter(){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::finishMeasurementWriter",
        "could not finish MeasurementWriter"
    );
    // Check there is a measurement writer to finish
    mw->finish();
}

fs::path FMSWriteFacade::getMeasurementWriterOutputPath(){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::getMeasurementWriterOutputPath",
        "could not get MeasurementWriter output file path"
    );
    // Get the output file path
    return mw->getOutputFilePath();
}
void FMSWriteFacade::setMeasurementWriterOutputPath(
    std::string path,
    const bool lastLegInStrip
){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::setMeasurementWriterOutputPath",
        "could not set MeasurementWriter output file path"
    );
    // Set the output file path
    mw->setOutputFilePath(path, lastLegInStrip);
}

bool FMSWriteFacade::isMeasurementWriterLasOutput() const{
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::isMeasurementWriterLasOutput",
        "could not get MeasurementWriter LAS output flag"
    );
    // Get the LAS output flag
    return mw->isLasOutput();
}

void FMSWriteFacade::setMeasurementWriterLasOutput(bool const lasOutput){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::isMeasurementWriterLasOutput",
        "could not set MeasurementWriter LAS output flag"
    );
    // Set the LAS output flag
    return mw->setLasOutput(lasOutput);
}

bool FMSWriteFacade::isMeasurementWriterLas10() const{
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::isMeasurementWriterLas10",
        "could not get MeasurementWriter LAS 10 flag"
    );
    // Get the LAS 10 flag
    return mw->isLas10();
}

void FMSWriteFacade::setMeasurementWriterLas10(bool const las10){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::setMeasurementWriterLas10",
        "could not set MeasurementWriter LAS 10 flag"
    );
    // Set the LAS 10 flag
    mw->setLas10(las10);
}

bool FMSWriteFacade::isMeasurementWriterZipOutput() const{
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::isMeasurementWriterZipOutput",
        "could not get MeasurementWriter zip output flag"
    );
    // Get the zip output flag
    return mw->isZipOutput();
}

void FMSWriteFacade::setMeasurementWriterZipOutput(bool const zipOutput){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::setMeasurementWriterZipOutput",
        "could not set MeasurementWriter zip output flag"
    );
    // Set the zip output flag
    mw->setZipOutput(zipOutput);
}

double FMSWriteFacade::getMeasurementWriterLasScale() const{
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::getMeasurementWriterLasScale",
        "could not get MeasurementWriter LAS scale"
    );
    // Obtain LAS scale
    return mw->getLasScale();
}

void FMSWriteFacade::setMeasurementWriterLasScale(double const lasScale){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::setMeasurementWriterLasScale",
        "could not set MeasurementWriter LAS scale"
    );
    // Set LAS scale
    return mw->setLasScale(lasScale);
}


// ***  FACADE TRAJECTORY WRITE METHODS  *** //
// ***************************************** //
void FMSWriteFacade::validateTrajectoryWriter(
    string const &callerName,
    string const &errorMsg
) const{
    // Check trajectory writer does exist
    if(tw==nullptr){
        stringstream ss;
        ss  << callerName << " " << errorMsg << " because it does not exist";
        throw HeliosException(ss.str());
    }
}
void FMSWriteFacade::writeTrajectory(Trajectory &t){
    // Check it is possible to do the operation
    validateTrajectoryWriter(
        "FMSWriteFacade::writeTrajectory",
        "could not write trajectory"
    );
    // Write the trajectory
    tw->writeTrajectory(t);
}

fs::path FMSWriteFacade::getTrajectoryWriterOutputPath(){
    // Check it is possible to do the operation
    validateTrajectoryWriter(
        "FMSWriteFacade::getTrajectoryWriterOutputPath",
        "could not get TrajectoryWriter output file path"
    );
    // Get the output file path
    return tw->getOutputFilePath();
}

void FMSWriteFacade::setTrajectoryWriterOutputPath(string const &path){
    // Check it is possible to do the operation
    validateMeasurementWriter(
        "FMSWriteFacade::setTrajectoryWriterOutputPath",
        "could not set TrajectoryWriter output file path"
    );
    // Set the output file path
    tw->setOutputFilePath(path);
}

// ***  FACADE FULL WAVEFORM WRITE METHODS  *** //
// ******************************************** //
void FMSWriteFacade::validateFullWaveformWriter(
    string const &callerName,
    string const &errorMsg
) const{
    // Check full waveform writer does exist
    if(fww==nullptr){
        stringstream ss;
        ss  << callerName << " " << errorMsg << " because it does not exist";
        throw HeliosException(ss.str());
    }
}
void FMSWriteFacade::writeFullWaveform(
    vector<double> const &fullwave,
    int const fullwaveIndex,
    double const minTime,
    double const maxTime,
    glm::dvec3 const &beamOrigin,
    glm::dvec3 const &beamDir,
    double const gpsTime
){
    // Check it is possible to do the operation
    validateFullWaveformWriter(
        "FMSWriteFacade::writeFullWaveform",
        "could not write full waveform"
    );
    // Write the full waveform data
    fww->writeFullWaveform(
        fullwave,
        fullwaveIndex,
        minTime,
        maxTime,
        beamOrigin,
        beamDir,
        gpsTime
    );
}
