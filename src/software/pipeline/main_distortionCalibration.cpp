// This file is part of the AliceVision project.
// Copyright (c) 2022 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This application tries to estimate the distortion of a set of images.
// It is assumed that for each image we have a result of the checkerboard detector.

// The constraint for this calibration is that we may not know :
// - the checkerboard size
// - the squares sizes
// - the checkerboard relative poses

// We may only have only one image per distortion to estimate.

// The idea is is to calibrate distortion parameters without estimating the pose or the intrinsics.
// This algorithms groups the corners by lines and minimize a distance between corners and lines using distortion.

#include <aliceVision/cmdline/cmdline.hpp>
#include <aliceVision/system/Logger.hpp>
#include <aliceVision/system/main.hpp>

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

#include <aliceVision/calibration/checkerDetector.hpp>
#include <aliceVision/calibration/checkerDetector_io.hpp>
#include <aliceVision/calibration/distortionEstimation.hpp>

#include <aliceVision/camera/Undistortion3DEA4.hpp>

#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/math/constants/constants.hpp>

#include <fstream>

// These constants define the current software version.
// They must be updated when the command line is changed.
#define ALICEVISION_SOFTWARE_VERSION_MAJOR 2
#define ALICEVISION_SOFTWARE_VERSION_MINOR 1

namespace po = boost::program_options;
using namespace aliceVision;


// Utility lambda to create lines by iterating over a board's cells in a given order
std::vector<calibration::LineWithPoints> createLines(
                const std::vector<calibration::CheckerDetector::CheckerBoardCorner> & corners,
                const calibration::CheckerDetector::CheckerBoard& board,
                bool exploreByRow,
                bool replaceRowWithSum,
                bool replaceColWithSum,
                bool flipRow,
                bool flipCol)
{
    std::vector<calibration::LineWithPoints> lines;

    const std::size_t minPointsPerLine = 10;

    int dim1 = exploreByRow ? board.rows() : board.cols();
    int dim2 = exploreByRow ? board.cols() : board.rows();

    for (int i = 0; i < dim1; ++i)
    {
        // Random init
        calibration::LineWithPoints line;
        line.dist = i;
        line.step = i;
        line.angle = M_PI_4;

        for (int j = 0; j < dim2; ++j)
        {
            int i_cell = replaceRowWithSum ? i + j : (exploreByRow ? i : j);
            i_cell = flipRow ? board.rows() - 1 - i_cell : i_cell;

            int j_cell = replaceColWithSum ? i + j : (exploreByRow ? j : i);
            j_cell = flipCol ? board.cols() - 1 - j_cell : j_cell;

            if (i_cell < 0 || i_cell >= board.rows() || j_cell < 0 || j_cell >= board.cols())
                continue;

            const IndexT idx = board(i_cell, j_cell);
            if (idx == UndefinedIndexT)
                continue;

            const calibration::CheckerDetector::CheckerBoardCorner& p = corners[idx];
            line.points.push_back(p.center);
        }

        // Check that we don't have a too small line which won't be easy to estimate
        if (line.points.size() < minPointsPerLine)
            continue;

        lines.push_back(line);
    }

    return lines;
}

bool retrieveLines(std::vector<calibration::LineWithPoints>& lineWithPoints, std::vector<calibration::SizeConstraint>& constraints, const calibration::CheckerDetector& detect)
{
    const std::vector<calibration::CheckerDetector::CheckerBoardCorner>& corners = detect.getCorners();
    const std::vector<calibration::CheckerDetector::CheckerBoard>& boards = detect.getBoards();
    
    for (auto& b : boards)
    {
        std::vector<calibration::LineWithPoints> items;

        std::vector<int> verticals, horizontals;
        
        items = createLines(corners, b, true, false, false, false, false);
        for (int i = 0; i < items.size(); i++)
        {
            items[i].groupId = 0;
            items[i].angle = 0;
            items[i].angleOffset = M_PI_2;
            horizontals.push_back(lineWithPoints.size());
            lineWithPoints.push_back(items[i]);
        }

        items = createLines(corners, b, false, false, false, false, false);        
        for (int i = 0; i < items.size(); i++)
        {
            items[i].groupId = 0;
            items[i].angle = 0;
            items[i].angleOffset = 0;
            verticals.push_back(lineWithPoints.size());
            lineWithPoints.push_back(items[i]);
        }

        /*items = createLines(corners, b, true, true, false, false, false);
        lineWithPoints.insert(lineWithPoints.end(), items.begin(), items.end());
        items = createLines(corners, b, true, true, false, true, false);
        lineWithPoints.insert(lineWithPoints.end(), items.begin(), items.end());
        items = createLines(corners, b, false, false, true, false, false);
        lineWithPoints.insert(lineWithPoints.end(), items.begin(), items.end());
        items = createLines(corners, b, false, false, true, true, false);
        lineWithPoints.insert(lineWithPoints.end(), items.begin(), items.end());*/


        std::vector<std::pair<int, int>> pairs1;
        for (int i = 0; i < horizontals.size() - 1; i++)
        {
            int id1 = horizontals[i];
            int id2 = horizontals[i + 1];
            pairs1.push_back(std::make_pair(id1, id2));
        }

        std::vector<std::pair<int, int>> pairs2;
        for (int i = 0; i < verticals.size() - 1; i++)
        {
            int id1 = verticals[i];
            int id2 = verticals[i + 1];
            pairs2.push_back(std::make_pair(id1, id2));
        }
        

        for (int idPair1 = 0; idPair1 < pairs1.size(); idPair1++)
        {
            for (int idPair2 = 0; idPair2 < pairs2.size(); idPair2++)
            {
                calibration::SizeConstraint c;
                c.firstPair = pairs1[idPair1];
                c.secondPair = pairs2[idPair2];

                double s11 = lineWithPoints[c.firstPair.first].step;
                double s12 = lineWithPoints[c.firstPair.second].step;
                double s21 = lineWithPoints[c.secondPair.first].step;
                double s22 = lineWithPoints[c.secondPair.second].step;

                if ((s12 - s11) != (s22  - s21))
                {
                    //continue;
                }

                constraints.push_back(c);
            }
        }
    }

    // Check that enough lines have been generated
    return lineWithPoints.size() > 1;
}

struct MinimizationStep
{
    std::vector<bool> locks;
    std::vector<calibration::SizeConstraint> sizeConstraints;
    bool lockAngles;
};

bool estimateDistortionMultiStep(std::shared_ptr<camera::Undistortion> undistortion,
                                 calibration::Statistics& statistics,
                                 std::vector<calibration::LineWithPoints>& lines,
                                 const std::vector<double> initialParams,
                                 const std::vector<MinimizationStep> steps)
{
    undistortion->setParameters(initialParams);

    for (std::size_t i = 0; i < steps.size(); ++i)
    {
        if (!calibration::estimate(undistortion, 
                                    statistics, 
                                    lines, 
                                    steps[i].sizeConstraints, 
                                    true, 
                                    steps[i].lockAngles, 
                                    steps[i].locks))
        {
            ALICEVISION_LOG_ERROR("Failed to calibrate at step " << i);
            return false;
        }
    }

    return true;
}

int aliceVision_main(int argc, char* argv[])
{
    std::string sfmInputDataFilepath;
    std::string checkerBoardsPath;
    std::string sfmOutputDataFilepath;

    std::string undistortionModelName = "3deanamorphic4";

    // clang-format off
    po::options_description requiredParams("Required parameters");
    requiredParams.add_options()
        ("input,i", po::value<std::string>(&sfmInputDataFilepath)->required(),
         "SfMData file input.")
        ("checkerboards", po::value<std::string>(&checkerBoardsPath)->required(),
         "Checkerboards json files directory.")
        ("output,o", po::value<std::string>(&sfmOutputDataFilepath)->required(),
         "SfMData file output.");
    
    po::options_description optionalParams("Optional parameters");
    optionalParams.add_options()
        ("undistortionModelName", po::value<std::string>(&undistortionModelName)->default_value(undistortionModelName),
         "Distortion model used for estimating undistortion.");
    // clang-format on

    CmdLine cmdline("This program calibrates camera distortion.\n"
                    "AliceVision distortionCalibration");
    cmdline.add(requiredParams);
    cmdline.add(optionalParams);
    if (!cmdline.execute(argc, argv))
    {
        return EXIT_FAILURE;
    }

    // Load sfmData from disk
    sfmData::SfMData sfmData;
    if (!sfmDataIO::load(sfmData, sfmInputDataFilepath, sfmDataIO::ESfMData(sfmDataIO::ALL)))
    {
        ALICEVISION_LOG_ERROR("The input SfMData file '" << sfmInputDataFilepath << "' cannot be read.");
        return EXIT_FAILURE;
    }

    // Load the checkerboards
    std::map<IndexT, calibration::CheckerDetector> boardsAllImages;
    for (auto& pv : sfmData.getViews())
    {
        IndexT viewId = pv.first;

        // Read the json file
        std::stringstream ss;
        ss << checkerBoardsPath << "/"
           << "checkers_" << viewId << ".json";
        std::ifstream inputfile(ss.str());
        if (!inputfile.is_open())
            continue;

        std::stringstream buffer;
        buffer << inputfile.rdbuf();
        boost::json::value jv = boost::json::parse(buffer.str());

        // Store the checkerboard
        calibration::CheckerDetector detector(boost::json::value_to<calibration::CheckerDetector>(jv));
        boardsAllImages[viewId] = detector;
    }

    // Retrieve camera model
    camera::EUNDISTORTION undistortionModel = camera::EUNDISTORTION_stringToEnum(undistortionModelName);

    // Calibrate each intrinsic independently
    for (auto& [intrinsicId, intrinsicPtr] : sfmData.getIntrinsics())
    {
        //Make sure we have only one aspect ratio per intrinsics
        std::set<double> pa;
        for (auto& [viewId, viewPtr] : sfmData.getViews())
        {
            if (viewPtr->getIntrinsicId() == intrinsicId)
            {
                pa.insert(viewPtr->getImage().getDoubleMetadata({"PixelAspectRatio"}));
            }
        }
        
        if (pa.size() != 1)
        {
            ALICEVISION_LOG_ERROR("An intrinsic has multiple views with different Pixel Aspect Ratios");
            return EXIT_FAILURE;
        }

        double pixelAspectRatio = *pa.begin();

        // Convert to pinhole
        std::shared_ptr<camera::Pinhole> cameraIn = std::dynamic_pointer_cast<camera::Pinhole>(intrinsicPtr);

        // Create new camera corresponding to given model
        std::shared_ptr<camera::Pinhole> cameraOut =
            std::dynamic_pointer_cast<camera::Pinhole>(
                camera::createIntrinsic(
                    camera::EINTRINSIC::PINHOLE_CAMERA,
                    camera::EDISTORTION::DISTORTION_NONE,
                    undistortionModel,
                    cameraIn->w(), 
                    cameraIn->h()
                )
            );

        if (!cameraIn || !cameraOut)
        {
            ALICEVISION_LOG_ERROR("Only work for pinhole cameras");
            return EXIT_FAILURE;
        }

        ALICEVISION_LOG_INFO("Processing Intrinsic " << intrinsicId);

        // Copy internal data from input camera to output camera
        cameraOut->setSensorWidth(cameraIn->sensorWidth());
        cameraOut->setSensorHeight(cameraIn->sensorHeight());
        cameraOut->setSerialNumber(cameraIn->serialNumber());
        cameraOut->setScale(cameraIn->getScale());
        cameraOut->setOffset(cameraIn->getOffset());

        // Change distortion initialization mode to calibrated
        cameraOut->setDistortionInitializationMode(camera::EInitMode::CALIBRATED);

        // Retrieve undistortion object
        std::shared_ptr<camera::Undistortion> undistortion = cameraOut->getUndistortion();
        if (!undistortion)
        {
            ALICEVISION_LOG_ERROR("Only work for cameras that support undistortion");
            return EXIT_FAILURE;
        }

        undistortion->setPixelAspectRatio(pixelAspectRatio);

        // Transform checkerboards to line With points
        std::vector<calibration::LineWithPoints> allLinesWithPoints;
        std::vector<calibration::SizeConstraint> allConstraints;
        for (auto& pv : sfmData.getViews())
        {
            if (pv.second->getIntrinsicId() != intrinsicId)
            {
                continue;
            }

            std::vector<calibration::LineWithPoints> linesWithPoints;
            std::vector<calibration::SizeConstraint> constraints;
            if (!retrieveLines(linesWithPoints, constraints, boardsAllImages[pv.first]))
            {
                continue;
            }
            
            //Shift the constraintIndices to match global array
            int shift = allLinesWithPoints.size();
            for (auto & c : constraints)
            {
                c.firstPair.first += shift;
                c.firstPair.second += shift;
                c.secondPair.first += shift;
                c.secondPair.second += shift;
            }

            allLinesWithPoints.insert(allLinesWithPoints.end(), linesWithPoints.begin(), linesWithPoints.end());
            allConstraints.insert(allConstraints.end(), constraints.begin(), constraints.end());
        }

        calibration::Statistics statistics;
        std::vector<double> initialParams;
        std::vector<MinimizationStep> steps;

        if (undistortionModel == camera::EUNDISTORTION::UNDISTORTION_3DEANAMORPHIC4)
        { 
            initialParams = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0};
            steps = {
                    //First, lock everything but lines 
                    {
                        {true, true, true, true, true, true, true, true, true, true, true, true, true},
                        std::vector<calibration::SizeConstraint>(),
                        true
                    },
                    {
                        {true, true, true, true, true, true, true, true, true, true, false, false, true},
                        allConstraints,
                        false
                    },
                    {
                        {false, false, false, false, true, true, true, true, true, true, false, false, true},
                        allConstraints,
                        false
                    },
                    {
                        {false, false, false, false, false, false, false, false, false, false, false, false, true},
                        allConstraints,
                        false
                    },
                };
        }
        else
        {
            ALICEVISION_LOG_ERROR("Unsupported camera model for undistortion.");
            return EXIT_FAILURE;
        }

        if (!estimateDistortionMultiStep(undistortion, statistics, 
                                        allLinesWithPoints, 
                                        initialParams, steps))
        {
            ALICEVISION_LOG_ERROR("Error estimating distortion");
            return EXIT_FAILURE;
        }

        // Override input intrinsic with output camera
        intrinsicPtr = cameraOut;

        ALICEVISION_LOG_INFO("Result quality of calibration: ");
        ALICEVISION_LOG_INFO("Mean of error (stddev): " << statistics.mean << "(" << statistics.stddev << ")");
        ALICEVISION_LOG_INFO("Median of error: " << statistics.median);
    }

    // Save sfmData to disk
    if (!sfmDataIO::save(sfmData, sfmOutputDataFilepath, sfmDataIO::ESfMData(sfmDataIO::ALL)))
    {
        ALICEVISION_LOG_ERROR("The output SfMData file '" << sfmOutputDataFilepath << "' cannot be written.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
