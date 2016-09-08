#include <arrayfire.h>
#include "Environments/Environment2D.h"
#include <random>
#include "BacterialPopulations/Kollmann2005Population.h"
#include "Models/Model2D.h"
#include "Solvers/ForwardEulerSolver.h"
#include "Solvers/RungeKuttaSolver.h"

#define NO_GRAPHICS

int main(int argc, char** argv)
{
	char t_device_name[64] = {0};
	char t_device_platform[64] = {0};
	char t_device_toolkit[64] = {0};
	char t_device_compute[64] = {0};
	af::deviceInfo(t_device_name, t_device_platform, t_device_toolkit, t_device_compute);

    printf("Device name: %s\n", t_device_name);
    printf("Platform name: %s\n", t_device_platform);
    printf("Toolkit: %s\n", t_device_toolkit);
    printf("Compute version: %s\n", t_device_compute);

    // Setup Environment
    EnvironmentSettings ESettings;

    ESettings.resolution = 0.5;
    ESettings.dimensions = std::vector<double> {50, 50};

    BoundaryCondition boundaryCondition(BC_PERIODIC);
    boundaryCondition.xpos = 0;
    boundaryCondition.ypos = 0;
    ESettings.boundaryCondition = boundaryCondition;

    // Setup Ligands

    Ligand ligand1 = {"Ligand1", 0, 100, 0.0, 0.0, 100.0};
    ESettings.ligands.push_back(ligand1);
//    Ligand ligand2 = {"Ligand2", 1, 10, 0.0, 0.0, 10.0};
//    ESettings.ligands.push_back(ligand2);
    shared_ptr<Solver> EnvSolver(static_cast<Solver *>(new ForwardEulerSolver));
    double largest_D = 0;
    for(auto ligand: ESettings.ligands)
        largest_D = std::max(largest_D, (double) ligand.diffusionCoefficient);

    ESettings.dt =(GPU_REALTYPE) pow(ESettings.resolution, 2)/(largest_D * 4);

    std::cout << "Simulating Environment with stepsize " << ESettings.dt << std::endl;

    shared_ptr<Environment2D> simEnv(new Environment2D(ESettings, EnvSolver));

    // Update randomness
    af::setSeed(time(NULL));

    std::vector<shared_ptr<BacterialPopulation>> populations;

    shared_ptr<Solver> BactSolver(static_cast<Solver *>(new RungeKuttaSolver));

    // Setup population 1
    std::vector<LigandInteraction> ligandInteractions1;
    LigandInteraction interaction11 = {0, 5, 0, 0, 0};
    ligandInteractions1.push_back(interaction11);

//    LigandInteraction interaction12 = {1,0.2, 0, 0, 0};
//    ligandInteractions1.push_back(interaction12);

    Kollmann2005Parameters bactParams = {BactSolver, ligandInteractions1, ESettings.dt, 5};
    populations.push_back(shared_ptr<BacterialPopulation>(static_cast<BacterialPopulation *>(new Kollmann2005Population("Population 1", simEnv, bactParams, 10))));

//    std::vector<LigandInteraction> ligandInteractions2;
//    LigandInteraction interaction21 = {1,0.2, 0, 0, 0};
//    ligandInteractions2.push_back(interaction21);
//
//    Kollmann2005Parameters bactParams2 = {BactSolver, ligandInteractions2, ESettings.dt, 10};
//    populations.push_back(shared_ptr<BacterialPopulation>(static_cast<BacterialPopulation *>(new Kollmann2005Population("Population 2", simEnv, bactParams2, 10))));


    // Setup model
    Model2D mymodel(simEnv, populations);
    mymodel.setupStorage("test.h5", 1000);
    mymodel.save();
    //mymodel.closeStorage();
    //for (auto pop: populations) {
    //    pop->printInternals();
    //}

    // Enable visualization
#ifndef NO_GRAPHICS
    Window diffusionwindow(1024, 512,"Simple Diffusion simulation");
    diffusionwindow.setColorMap(AF_COLORMAP_HEAT);
    Window populationwindow(1024,512, "Populations");
    mymodel.setupVisualizationWindows(diffusionwindow, populationwindow);
#endif
    int measurementTime = 100;
    double simulationTime = 400;
    time_t start= time(NULL);
    for(int i =0; i < simulationTime/ESettings.dt; i++) {
        mymodel.simulateTimestep();
        mymodel.save();
        if(i && !(i%measurementTime)) {
            double seconds = difftime(time(NULL), start);
            time(&start);
            std::cout << measurementTime/seconds << " iterations per second" << std::endl;
            std::cout << "Finished "<< i << " iterations of total " << simulationTime/ESettings.dt << std::endl;
        }

#ifndef NO_GRAPHICS
        mymodel.visualize();
#endif
    }
    mymodel.closeStorage();

    return 0;
}
