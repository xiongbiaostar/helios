#pragma once

#include <filems/write/comps/LasSyncFileWriter.h>
#include <filems/write/strategies/LasMeasurementWriteStrategy.h>
#include <Measurement.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>


namespace helios { namespace filems {

using std::make_shared;

/**
 * @author Alberto M. Esmoris Pena
 * @version 1.0
 * @brief SyncFileWriter implementation to write measurements in LAS format
 */
class LasSyncFileMeasurementWriter :
    public LasSyncFileWriter<Measurement const &, glm::dvec3 const &>
{
public:
    // ***  CONSTRUCTION / DESTRUCTION  *** //
    // ************************************ //
    LasSyncFileMeasurementWriter() :
        LasSyncFileWriter<Measurement const&, glm::dvec3 const&>()
    {};
    /**
     * @brief Synchronous LAS file measurement writer constructor
     * @see LasSyncFileWriter::LasSyncFileWriter
     */
    explicit LasSyncFileMeasurementWriter(
        const std::string &path,
        bool const compress = false,
        double const scaleFactor = 0.0001,
        glm::dvec3 const offset = glm::dvec3(0, 0, 0),
        double const minIntensity = 0.0,
        double const deltaIntensity = 1000000.0,
        bool const createWriter = true
    ) :
        LasSyncFileWriter<Measurement const&, glm::dvec3 const&>(
            path,
            compress,
            scaleFactor,
            offset,
            minIntensity,
            deltaIntensity,
            createWriter
        )
    {
        // Write strategy
        this->writeStrategy = make_shared<LasMeasurementWriteStrategy>(
            *lw,
            lp,
            scaleFactorInverse,
            this->offset,
            this->minIntensity,
            maxIntensity,
            intensityCoefficient,
            ewAttrStart,
            fwiAttrStart,
            hoiAttrStart,
            ampAttrStart
        );
    }

    ~LasSyncFileMeasurementWriter() override
    {LasSyncFileMeasurementWriter::finish();}

};

}}
