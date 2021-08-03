#pragma once

#include "BaseTest.h"
#include <maths/rigidmotion/RigidMotionEngine.h>
#include <maths/rigidmotion/RigidMotionR2Factory.h>
#include <maths/rigidmotion/RigidMotionR3Factory.h>

#include <armadillo>

using namespace arma;
using namespace rigidmotion;

namespace HeliosTests{

/**
 * @author: Alberto M. Esmoris Pena
 * @version 1.0
 * @brief Rigid motion test
 */
class RigidMotionTest : public BaseTest{
public:
    // ***  ATTRIBUTES  *** //
    // ******************** //
    /**
     * @brief Decimal precision for validation purposes
     */
    double eps = 0.00001;
    /**
     * @brief The rigid motion engine to be used by tests
     */
    RigidMotionEngine rme;
    /**
     * @brief The R2 rigid motion factory to be used by tests
     */
    RigidMotionR2Factory rm2f;
    /**
     * @brief The R3 rigid motion factory to be used by tests
     */
    //RigidMotionR3Factory rm3f; // TODO Rethink : To be implemented
    /**
     * @brief Matrix of points in R2 to be used by tests
     */
    mat R2X;
    /**
     * @brief Matrix of points in R3 to be used by tests
     */
    mat R3X;
    // ***  CONSTRUCTOR  *** //
    // ********************* //
    /**
     * @brief Rigid motion test constructor
     */
    RigidMotionTest() : BaseTest("Rigid motion test"){}

    // ***  R U N  *** //
    // *************** //
    /**
     * @see BaseTest::run
     */
    bool run() override;

    // ***  SUB-TESTS  *** //
    // ******************* //
    /**
     * @brief Test rigid motion implementation on its own
     * @return True if passed, false otherwise
     */
    bool testPureRigidMotion();
    /**
     * @brief Test rigid motion implementation adapted to Helios components
     * @return True if passed, false otherwise
     */
    bool testHeliosRigidMotion();

    // ***  PURE TESTS  *** //
    // ******************** //
    /**
     * @brief Test identity rigid motion in \f$\mathbb{R}^{2}\f$
     * @return True if passed, false otherwise
     */
    bool testPureIdentityR2();
    /**
     * @brief Test translation rigid motion in \f$\mathbb{R}^{2}\f$
     * @return True if passed, false otherwise
     */
    bool testPureTranslationR2();
    /**
     * @brief Test reflection rigid motion in \f$\mathbb{R}^{2}\f$
     * @return True if passed, false otherwise
     */
    bool testPureReflectionR2();

    // ***  HELIOS TESTS  *** //
    // ********************** //
};

// ***  R U N  *** //
// *************** //
bool RigidMotionTest::run(){
    // Initialize data
    R2X = mat(2, 5);
    for(size_t i = 0 ; i < 10 ; ++i){
        R2X.at(i) = (((double)i)-2.5)*std::pow(-1.0, (double)i);
    }
    R3X = mat(3, 5);
    for(size_t i = 0 ; i < 15 ; ++i){
        R3X.at(i) = (((double)i)-2.5)*std::pow(-1.0, (double)i);
    }

    // Run tests
    if(!testPureRigidMotion()) return false;
    if(!testHeliosRigidMotion()) return false;
    return true;
}

// ***  SUB-TESTS  *** //
// ******************* //
bool RigidMotionTest::testPureRigidMotion(){
    // R2 tests
    if(!testPureIdentityR2()) return false;
    if(!testPureTranslationR2()) return false;
    if(!testPureReflectionR2()) return false;

    // R3 tests
    // TODO Rethink : To be implemented

    return true;
}
bool RigidMotionTest::testHeliosRigidMotion(){
    // TODO Rethink : To be implemented
    return true;
}

// ***  PURE TESTS  *** //
// ******************** //
bool RigidMotionTest::testPureIdentityR2(){
    RigidMotion f = rm2f.makeIdentity();
    mat Y = rme.apply(f, R2X);
    mat Z = abs(Y-R2X);
    return !any(vectorise(Z) > eps);
}
bool RigidMotionTest::testPureTranslationR2(){
    colvec shift(std::vector<double>({1.67, -2.27}));
    RigidMotion f = rm2f.makeTranslation(shift);
    mat Y = rme.apply(f, R2X);
    mat EY = R2X.each_col() + shift;
    mat Z = abs(Y-EY);
    return !any(vectorise(Z) > eps);
}
bool RigidMotionTest::testPureReflectionR2(){
    colvec axis(std::vector<double>({2.1, -3.9}));
    colvec X(std::vector<double>({-3, 1}));
    RigidMotion f = rm2f.makeReflection(axis);
    colvec Y = rme.apply(f, X);
    colvec EY(std::vector<double>({0.81651376, 3.05504587}));
    bool passed = !any(vectorise(abs(Y-EY)) > eps);
    if(!passed) return passed;

    mat Y2 = rme.apply(f, R2X);
    mat EY2(2, 5);
    EY2.at(0, 0) = 0.12385321;        EY2.at(1, 0) = 2.91284404;
    EY2.at(0, 1) = 0.69266055;        EY2.at(1, 1) = 0.14220183;
    EY2.at(0, 2) = 1.26146789;        EY2.at(1, 2) = -2.62844037;
    EY2.at(0, 3) = 1.83027523;        EY2.at(1, 3) = -5.39908257;
    EY2.at(0, 4) = 2.39908257;        EY2.at(1, 4) = -8.16972477;
    mat Z = abs(Y2-EY2);
    passed = !any(vectorise(Z) > eps);
    if(!passed) return passed;

    axis = std::vector<double>({2.1, 0.9});
    f = rm2f.makeReflection(axis);
    mat Y3 = rme.apply(f, R2X);
    mat EY3(2, 5);
    EY3.at(0, 0) = -0.63793103;         EY3.at(1, 0) = -2.84482759;
    EY3.at(0, 1) = -0.70689655;         EY3.at(1, 1) = -0.01724138;
    EY3.at(0, 2) = -0.77586207;         EY3.at(1, 2) = 2.81034483;
    EY3.at(0, 3) = -0.84482759;         EY3.at(1, 3) = 5.63793103;
    EY3.at(0, 4) = -0.9137931;          EY3.at(1, 4) = 8.46551724;
    Z = abs(Y3-EY3);
    passed = !any(vectorise(Z) > eps);
    if(!passed) return passed;

    return passed;
}



}