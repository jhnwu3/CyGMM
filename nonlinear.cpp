#include "nonlinear.hpp"
/* Converts an Eigen::VectorXd into a State_N aka a vector<double> v2 as we are using two different C++ libraries for computations */
State_N convertInit(const VectorXd &v1){
    vector<double> v2;
    v2.resize(v1.size());
    VectorXd::Map(&v2[0], v1.size()) = v1;
    return v2;
}

/* 
Summary:
    Nonlinear position adaptation, randomly picks rate constants to generate a random value from a beta distribution.
Input:
    posK - position vector in PSO
    seed - currently unused, but was to be used to set seed for rng generator
    epsi - value to reposition particle back into hypercube
    nan - threshold to determine if overstepping into boundary.
    hone - width of beta distribution of random values to be generated from for new position

*/
VectorXd adaptVelocity(const VectorXd& posK, int seed, double epsi, double nan, int hone) {

    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    if(seed > 0){
        generator.seed(seed);
    }    
    VectorXd rPoint;
    rPoint = posK;
    /* create random int vector */
    vector<int> rand;
    for (int i = 0; i < posK.size(); i++) {
        rand.push_back(i);
    }
    shuffle(rand.begin(), rand.end(), generator); // shuffle indices as well as possible. 

    /*randomize which indiecs of position to iterate / randomly generate positions */
    int ncomp = rand.at(0);
    VectorXd wcomp(ncomp);
    shuffle(rand.begin(), rand.end(), generator);
    for (int i = 0; i < ncomp; i++) {
        wcomp(i) = rand.at(i);
    }
    
    /* Now randomly generate new positions using beta distribution */
    for (int smart = 0; smart < ncomp; smart++) {
        int px = wcomp(smart);
        double pos = rPoint(px);
        if (pos > 1.0 - nan) {
            pos -= epsi;
        }else if (pos < nan) {
            pos += epsi;
        }
        double alpha = hone * pos; // Component specific
        double beta = hone - alpha; // pos specific

        std::gamma_distribution<double> aDist(alpha, 1); // beta distribution consisting of gamma distributions
        std::gamma_distribution<double> bDist(beta, 1);

        double x = aDist(generator);
        double y = bDist(generator);
        rPoint(px) = (x / (x + y)); 
    }
    return rPoint;
}

/*Defunct evolve function - too slow to be put in practice for now */
Protein_Components evolveSystem(const VectorXd &pos, const MatrixXd& X_0, int nMoments, double t, double dt, double t0){
    Controlled_RK_Stepper_N controlledStepper;
    /*solve ODEs*/
    Protein_Components XtPSO(t, nMoments, X_0.rows(), X_0.cols());
    Moments_Mat_Obs XtObsPSO1(XtPSO);
    Nonlinear_ODE stepSys(pos);
    for(int i = 0; i < X_0.rows(); i++){
        State_N c0 = convertInit(X_0.row(i));
        XtPSO.index = i;
        integrate_adaptive(controlledStepper, stepSys, c0, t0, t, dt, XtObsPSO1);
    }
    XtPSO.mVec/=X_0.rows();

    return XtPSO;
}

/* Defunct nonlinear model moved into main for speed reasons. - To be debugged */
int main(int argc, char** argv){
    
    int nParts = 1500, nSteps = 30, nParts2 = 30, nSteps2 = 30, nRates = 5, nRuns = 1, nMoments = 9;
    MatrixXd X_0 = readX(getXPath(argc, argv));
    vector<MatrixXd> Y = readY(getYPath(argc, argv));
    auto t1 = std::chrono::high_resolution_clock::now();
    /*---------------------- Setup ------------------------ */
    VectorXd times = readCsvTimeParam(getTimeStepsPath(argc, argv));
    /* Variables (global) */
    double t0 = 0, dt = 1.0; // time variables
    int nTimeSteps = times.size();
    int Npars = nRates;
    double squeeze = 0.500, sdbeta = 0.10; 
    double boundary = 0.001;
    /* SETUP */
    int useDiag = 0;
    int sf1 = 1;
    int sf2 = 1;
    double epsi = 0.02;
    double nan = 0.005;
    /* PSO params */
    double sfp = 3.0, sfg = 1.0, sfe = 6.0; // initial particle historical weight, global weight social, inertial
    double sfi = sfe, sfc = sfp, sfs = sfg; // below are the variables being used to reiterate weights
    double alpha = 0.2;
    int N = X_0.rows();
    int hone = 28; 
    int startRow = 0;
     
    //nMoments = 2*N_SPECIES; // mean + var only!
    VectorXd wmatup(4);
    wmatup << 0.15, 0.35, 0.60, 0.9;
    double uniLowBound = 0.0, uniHiBound = 1.0;
    random_device RanDev;
    mt19937 gen(RanDev());
    uniform_real_distribution<double> unifDist(uniLowBound, uniHiBound);
    
    vector<MatrixXd> weights;

    for(int i = 0; i < nTimeSteps; i++){
        weights.push_back(MatrixXd::Identity(nMoments, nMoments));
    }
    
    cout << "Using two part PSO " << "Sample Size:" << N << " with:" << nMoments << " moments." << endl;
    cout << "Using Times:" << times.transpose() << endl;
    cout << "Bounds for Uniform Distribution (" << uniLowBound << "," << uniHiBound << ")"<< endl;
    cout << "Blind PSO --> nParts:" << nParts << " Nsteps:" << nSteps << endl;
    cout << "Targeted PSO --> nParts:" <<  nParts2 << " Nsteps:" << nSteps2 << endl;
    cout << "sdbeta:" << sdbeta << endl;
    // cout << "wt:" << endl << wt << endl;

    MatrixXd GBMAT(0, 0); // iterations of global best vectors
    MatrixXd PBMAT(nParts, Npars + 1); // particle best matrix + 1 for cost component
    MatrixXd POSMAT(nParts, Npars); // Position matrix as it goees through it in parallel

    /* Solve for Y_t (mu). */
    cout << "Loading in Truk!" << endl;
    VectorXd tru = VectorXd::Zero(Npars);
    tru << 0.1, 0.1, 0.95, 0.17, 0.05;

    cout << "Calculating Yt!" << endl;
    vector<MatrixXd> Yt3Mats;
    vector<VectorXd> Yt3Vecs;
    vector<VectorXd> Xt3Vecs;
    Controlled_RK_Stepper_N controlledStepper;
    if (simulateY(argc, argv)){
        MatrixXd Y_0 = Y[0];
        double trukCost = 0;
        for(int t = 0; t < nTimeSteps; t++){
            Nonlinear_ODE trueSys(tru);
            Protein_Components Yt(times(t), nMoments, N, X_0.cols());
            Protein_Components Xt(times(t), nMoments, N, X_0.cols());
            Moments_Mat_Obs YtObs(Yt);
            Moments_Mat_Obs XtObs(Xt);
            for (int i = 0; i < N; ++i) {
                //State_N c0 = gen_multi_norm_iSub(); // Y_0 is simulated using norm dist.
                State_N y0 = convertInit(Y_0.row(i));
                State_N x0 = convertInit(X_0.row(i));
                Yt.index = i;
                Xt.index = i;
                integrate_adaptive(controlledStepper, trueSys, y0, t0, times(t), dt, YtObs);
                integrate_adaptive(controlledStepper, trueSys, x0, t0, times(t), dt, XtObs);
            }
            Yt.mVec /= N;
            Xt.mVec /= N;
            trukCost += costFunction(Yt.mVec,Xt.mVec, weights[t]);
            Xt3Vecs.push_back(Xt.mVec);
            Yt3Mats.push_back(Yt.mat);
            Yt3Vecs.push_back(Yt.mVec);
        }
        cout << "truk cost:"<< trukCost << endl;
    }else{
        Yt3Mats = Y;
        for(int ydx = 0; ydx < Y.size(); ++ ydx){
            Yt3Vecs.push_back(momentVector(Y[ydx], nMoments));
        }

    }

    /* Compute initial wolfe weights */
    for(int t = 0; t < nTimeSteps; ++t){
        weights[t] = wolfWtMat(Yt3Mats[t], nMoments, false);
        cout << "wt:" << endl;
        cout << weights[t] << endl << endl;
    }
    if(useLinear(argc, argv)){ // bad coding practice to just simply return, but it works for now.
        MatrixXd linearGBMAT = linearModel(nParts, nSteps, nParts2, nSteps2, X_0, nRates, nMoments, times, simulateY(argc, argv), -1, argc, argv, -1);
        return EXIT_SUCCESS;
    }
    MatrixXd GBVECS = MatrixXd::Zero(nRuns, Npars + 1);
    for(int run = 0; run < nRuns; ++run){
        // make sure to reset GBMAT, POSMAT, AND PBMAT EVERY RUN!
        double sfi = sfe, sfc = sfp, sfs = sfg; // below are the variables being used to reiterate weights
        MatrixXd GBMAT = MatrixXd::Zero(0,0); // iterations of global best vectors
        MatrixXd PBMAT = MatrixXd::Zero(nParts, Npars + 1); // particle best matrix + 1 for cost component
        MatrixXd POSMAT = MatrixXd::Zero(nParts, Npars); // Position matrix as it goees through it in parallel
        
        /* Instantiate seedk aka global costs */
        double holdTheta2 = 0.1;
        VectorXd seed = VectorXd::Zero(Npars); 
        //seed.k = testVec;
        for (int i = 0; i < Npars; i++) { 
            seed(i) = unifDist(gen);
        }
        // seed.k(4) = tru.k(4);
        seed(1) = holdTheta2;
   
        double costSeedK = 0;
        for(int t = 0; t < nTimeSteps; t++){
            Protein_Components Xt(times(t), nMoments, N, X_0.cols());
            Moments_Mat_Obs XtObs(Xt);
            Nonlinear_ODE sys(seed);
            for (int i = 0; i < N; ++i) {
                //State_N c0 = gen_multi_norm_iSub();
                State_N c0 = convertInit(X_0.row(i));
                Xt.index = i;
                integrate_adaptive(controlledStepper, sys, c0, t0, times(t), dt, XtObs);
            }
            Xt.mVec /= N;  
            cout << "XtmVec:" << Xt.mVec.transpose() << endl;
            costSeedK += costFunction(Yt3Vecs[t], Xt.mVec, weights[t]);
        }

        cout << "seedk:"<< seed.transpose() << "| cost:" << costSeedK << endl;
        
        double gCost = costSeedK; //initialize costs and GBMAT
        // global values
        VectorXd GBVEC = seed;
        
        GBMAT.conservativeResize(GBMAT.rows() + 1, Npars + 1);
        for (int i = 0; i < Npars; i++) {
            GBMAT(GBMAT.rows() - 1, i) = seed(i);
        }
        GBMAT(GBMAT.rows() - 1, Npars) = gCost;
        double probabilityToTeleport = 3.0/4.0; 
        /* Blind PSO begins */
        cout << "PSO begins!" << endl;
        for(int step = 0; step < nSteps; ++step){
        #pragma omp parallel for 
            for(int particle = 0; particle < nParts; particle++){
                random_device pRanDev;
                mt19937 pGenerator(pRanDev());
                uniform_real_distribution<double> pUnifDist(uniLowBound, uniHiBound);
                /* instantiate all particle rate constants with unifDist */
                if(step == 0){
                    /* temporarily assign specified k constants */
                    for(int i = 0; i < Npars; i++){
                        POSMAT(particle, i) = pUnifDist(pGenerator);
                    }
                    // POSMAT(particle, 4) = 0.05;
                    POSMAT(particle, 1) = holdTheta2;
                    // POSMAT.row(particle) = seed.k;
                
                    VectorXd pos = VectorXd::Zero(Npars);
                    for(int i = 0; i < Npars; i++){
                        pos(i) = POSMAT(particle, i);
                    }
                    
                    double cost = 0;
                    for(int t = 0; t < nTimeSteps; t++){
                        Nonlinear_ODE initSys(pos);
                        Protein_Components XtPSO(times(t), nMoments, N, X_0.cols());
                        Moments_Mat_Obs XtObsPSO(XtPSO);
                        for(int i = 0; i < N; i++){
                            //State_N c0 = gen_multi_norm_iSub();
                            State_N c0 = convertInit(X_0.row(i));
                            XtPSO.index = i;
                            integrate_adaptive(controlledStepper, initSys, c0, t0, times(t), dt, XtObsPSO);
                        }
                        XtPSO.mVec/=N;
                        cost += costFunction(Yt3Vecs[t], XtPSO.mVec, weights[t]);
                    }
                    
                    
                    /* instantiate PBMAT */
                    for(int i = 0; i < Npars; i++){
                        PBMAT(particle, i) = POSMAT(particle, i);
                    }
                    PBMAT(particle, Npars) = cost; // add cost to final column
                }else{ 
                    /* using new rate constants, instantiate particle best values */
                    /* step into PSO */
                    double w1 = sfi * pUnifDist(pGenerator)/ sf2, w2 = sfc * pUnifDist(pGenerator) / sf2, w3 = sfs * pUnifDist(pGenerator)/ sf2;
                    double sumw = w1 + w2 + w3; //w1 = inertial, w2 = pbest, w3 = gbest
                    w1 = w1 / sumw; w2 = w2 / sumw; w3 = w3 / sumw;
         
                    VectorXd pos = VectorXd::Zero(Npars);
                    pos = POSMAT.row(particle);
                    VectorXd rpoint = adaptVelocity(pos, particle, epsi, nan, hone);
                    VectorXd PBVEC(Npars);
                    for(int i = 0; i < Npars; i++){
                        PBVEC(i) = PBMAT(particle, i);
                    }
                    
                    pos = w1 * rpoint + w2 * PBVEC + w3 * GBVEC; // update position of particle
                    
                    // PSO restart feature
                    // if(pUnifDist(pGenerator) < probabilityToTeleport){ // hard coded grid re-search for an adaptive component
                    //     pos(0) = pUnifDist(pGenerator);
                    //     pos(1) = pUnifDist(pGenerator);
                    //     pos(4) = pUnifDist(pGenerator);
                    // }
          
                    pos(1) = holdTheta2;
                    POSMAT.row(particle) = pos;
                    double cost = 0;
                    for(int t = 0; t < nTimeSteps; t++){
                        /*solve ODEs and recompute cost */
                        Protein_Components XtPSO(times(t), nMoments, N, X_0.cols());
                        Moments_Mat_Obs XtObsPSO1(XtPSO);
                        Nonlinear_ODE stepSys(pos);
                        for(int i = 0; i < N; i++){
                            State_N c0 = convertInit(X_0.row(i));
                            XtPSO.index = i;
                            integrate_adaptive(controlledStepper, stepSys, c0, t0, times(t), dt, XtObsPSO1);
                        }
                        XtPSO.mVec/=N;
                        cost += costFunction(Yt3Vecs[t], XtPSO.mVec, weights[t]);
                    }
                
                    /* update gBest and pBest */
                #pragma omp critical
                {
                    if(cost < PBMAT(particle, Npars)){ // particle best cost
                        for(int i = 0; i < Npars; i++){
                            PBMAT(particle, i) = pos(i);
                        }
                        PBMAT(particle, Npars) = cost;
                        if(cost < gCost){
                            gCost = cost;
                            GBVEC = pos;
                        }   
                    }
                }
                }
            }
            GBMAT.conservativeResize(GBMAT.rows() + 1, Npars + 1); // Add to GBMAT after resizing
            for (int i = 0; i < Npars; i++) {GBMAT(GBMAT.rows() - 1, i) = GBVEC(i);}
            GBMAT(GBMAT.rows() - 1, Npars) = gCost;
            sfi = sfi - (sfe - sfg) / nSteps;   // reduce the inertial weight after each step 
            sfs = sfs + (sfe - sfg) / nSteps;
            cout << "current:" << GBVEC.transpose()<<" "<< gCost << endl;
        }

        cout << "GBMAT from blind PSO:" << endl << endl;
        cout << GBMAT << endl << endl;
        cout << "truk: " << tru.transpose() << endl;
        auto tB = std::chrono::high_resolution_clock::now();
        auto bDuration = std::chrono::duration_cast<std::chrono::seconds>(tB - t1).count();
        cout << "blind PSO FINISHED RUNNING IN " << bDuration << " s TIME!" << endl;
    
        for(int i = 0; i < Npars; i++){
            GBVECS(run, i) = GBVEC(i);
        }
        GBVECS(run, Npars) = gCost;
    }
    double trukCost = 0;
    for(int t = 0; t < nTimeSteps; t++){
        trukCost += costFunction(Yt3Vecs[t], Xt3Vecs[t], weights[t]);
    }

    cout << "truk: " << tru.transpose() << " with trukCost with new weights:" << trukCost << endl;
    
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();
    cout << "CODE FINISHED RUNNING IN " << duration << " s TIME!" << endl;
    cout << "Program Finished, PSO Estimates Per Iteration:" << endl; 
    cout << GBMAT; // just to close the program at the end.
    return EXIT_SUCCESS;
}


