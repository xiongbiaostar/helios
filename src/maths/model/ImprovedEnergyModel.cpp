#include <ImprovedEnergyModel.h>
#include <maths/EnergyMaths.h>
#include <scanner/ScanningDevice.h>
#include <scanner/detector/AbstractDetector.h>

// ***  CONSTRUCTION / DESTRUCTION  *** //
// ************************************ //
ImprovedEnergyModel::ImprovedEnergyModel(ScanningDevice const &sd) :
    BaseEnergyModel(sd),
    radii(sd.FWF_settings.beamSampleQuality+1),
    radiiSquared(sd.FWF_settings.beamSampleQuality+1),
    w0Squared(
        (sd.beamQuality*sd.wavelength_m)*(sd.beamQuality*sd.wavelength_m) / (
            (M_PI*sd.beamDivergence_rad)*(M_PI*sd.beamDivergence_rad)
        )
    ),
    totPower(2*sd.averagePower_w / (M_PI*w0Squared)),
    omegaCacheSquared(
        (sd.wavelength_m/(M_PI*w0Squared))*(sd.wavelength_m/(M_PI*w0Squared))
    ),
    targetAreaCache(M_PI/sd.numRays)
{
// Cached angles
    int const BSQ = sd.FWF_settings.beamSampleQuality;
    radii[0] = 0.0;
    radiiSquared[0] = 0.0;
    for(int i = 0 ; i < BSQ ; ++i){
        radii[i+1] = sd.beamDivergence_rad*(
            sd.cached_subrayRadiusStep[i]+0.5
        ) / (2 * (BSQ-0.5));
        radiiSquared[i+1] = radii[i+1] * radii[i+1];
    }
}


// ***  METHODS  *** //
// ***************** //
double ImprovedEnergyModel::computeIntensity(
    double const incidenceAngle,
    double const targetRange,
    Material const &mat,
    double const radius,
    int const subrayRadiusStep
){
    ImprovedReceivedPowerArgs args = ImprovedEnergyModel::extractArgumentsFromScanningDevice(
        sd,
        incidenceAngle,
        targetRange,
        mat,
        radius,
        subrayRadiusStep
    );
    return computeReceivedPower(args);
}

double ImprovedEnergyModel::computeReceivedPower(
    ModelArg const & _args
#if DATA_ANALYTICS >=2
   ,std::vector<std::vector<double>> &calcIntensityRecords
#endif
){
    ImprovedReceivedPowerArgs const &args = static_cast<
        ImprovedReceivedPowerArgs const &
    >(_args);
    // Pre-computations
    double const rangeSquared = args.targetRange*args.targetRange;
    // Emitted power
    double const Pe = computeEmittedPower(ImprovedEmittedPowerArgs{
        args.averagePower_w,
        args.wavelength_m,
        args.rangeMin,
        args.targetRange,
        rangeSquared,
        args.deviceBeamDivergence_rad,
        args.beamWaistRadius,
        args.numSubrays,
        args.beamSampleQuality,
        args.beamQualityFactor,
        args.subrayRadiusStep
    });
    // Target area
    double const targetArea = computeTargetArea(ImprovedTargetAreaArgs{
            rangeSquared,
            args.deviceBeamDivergence_rad,
            args.beamSampleQuality,
            args.subrayRadiusStep,
            args.numSubrays
        }
#if DATA_ANALYTICS >= 2
       ,calcIntensityRecords
#endif
    );
    // Cross-section
    double const bdrf = EnergyMaths::computeBDRF(
        args.material,
        args.incidenceAngle_rad
    );
    double const sigma = computeCrossSection(BaseCrossSectionArgs{
        args.material,
        bdrf,
        targetArea
    });
    // Received power
    double const atmosphericFactor = EnergyMaths::calcAtmosphericFactor(
        args.targetRange, args.atmosphericExtinction
    );
    double const receivedPower = EnergyMaths::calcReceivedPowerImprovedFast(
        Pe,
        args.Dr2,
        16*targetArea*rangeSquared,
        args.efficiency,
        atmosphericFactor,
        sigma
    );
#if DATA_ANALYTICS >= 2
    std::vector<double> & calcIntensityRecord = calcIntensityRecords.back();
    calcIntensityRecord[3] = args.incidenceAngle_rad;
    calcIntensityRecord[4] = args.targetRange;
    calcIntensityRecord[5] = targetArea;
    calcIntensityRecord[7] = bdrf;
    calcIntensityRecord[8] = sigma;
    calcIntensityRecord[9] = receivedPower;
    calcIntensityRecord[10] = 0; // By default, assume the point isn't captured
    calcIntensityRecord[11] = Pe;
    calcIntensityRecord[12] = args.subrayRadiusStep;
#endif
    return receivedPower * 1e09;
}

double ImprovedEnergyModel::computeEmittedPower(
    ModelArg const &_args
){
    ImprovedEmittedPowerArgs const & args = static_cast<
        ImprovedEmittedPowerArgs const &
    >(_args);
    double prevRadiusSquared = radiiSquared[int(args.subrayRadiusStep)];
    double const radiusSquared = radiiSquared[int(args.subrayRadiusStep)+1];
    double const Omega0 = 1 - args.targetRange/args.rangeMin;
    double const OmegaSquared = args.targetRangeSquared * omegaCacheSquared;
    double const wSquared = w0Squared * (Omega0*Omega0 + OmegaSquared);
    return EnergyMaths::calcSubrayWiseEmittedPowerFast(
        totPower,
        w0Squared,
        wSquared,
        radiusSquared,
        prevRadiusSquared,
        args.numSubrays
    );
}

double ImprovedEnergyModel::computeTargetArea(
    ModelArg const &_args
#if DATA_ANALYTICS >=2
   ,std::vector<std::vector<double>> &calcIntensityRecords
#endif
){
    // Once for target area and once for emitted power
    ImprovedTargetAreaArgs const & args = static_cast<
        ImprovedTargetAreaArgs const &
    >(_args);
    double const prevRadiusSquared = radiiSquared[int(args.subrayRadiusStep)];
    double const radiusSquared = radiiSquared[int(args.subrayRadiusStep)+1];
    double const radius_m_squared = radiusSquared * args.targetRangeSquared;
    double const prevRadius_m_squared =
        prevRadiusSquared * args.targetRangeSquared;
#if DATA_ANALYTICS >= 2
    std::vector<double> calcIntensityRecord(
        13, std::numeric_limits<double>::quiet_NaN()
    );
    calcIntensityRecord[6] = std::sqrt(radius_m_squared);
    calcIntensityRecords.push_back(calcIntensityRecord);
#endif
    return (radius_m_squared - prevRadius_m_squared) * targetAreaCache;
}

// ***  EXTRACT-ARGUMENTS METHODS  *** //
// *********************************** //
ImprovedReceivedPowerArgs ImprovedEnergyModel::extractArgumentsFromScanningDevice(
    ScanningDevice const &sd,
    double const incidenceAngle,
    double const targetRange,
    Material const &mat,
    double const radius,
    int const subrayRadiusStep
){
    return ImprovedReceivedPowerArgs(
        sd.averagePower_w,
        sd.wavelength_m,
        sd.detector->cfg_device_rangeMin_m,
        targetRange,
        sd.atmosphericExtinction,
        incidenceAngle,
        sd.cached_Dr2,
        sd.cached_Bt2,
        sd.efficiency,
        (double) ( (subrayRadiusStep==0) ? 1 :
                   (int) (subrayRadiusStep*PI_2) // 2 pi x depth = samples/ring
        ),
        mat,
        sd.beamDivergence_rad,
        sd.beamWaistRadius,
        (double) sd.FWF_settings.beamSampleQuality,
        (double) sd.beamQuality,
        (double) subrayRadiusStep
    );
}
