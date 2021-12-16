#include "Heap.h"
#include "LKH.h"

static const char Delimiters[] = " :=\n\t\r\f\v\xef\xbb\xbf";
static void CheckSpecificationPart(void);
static char *Copy(const char *S);
static void CreateNodes(void);
static int FixEdge(Node *Na, Node *Nb);
static void Read_DIMENSION(void);
static void Read_NAME(void);
static void Read_NODE_SECTION(void);
static void Read_SALESMEN_and_CAPACITY(void);
static void Read_TOUR_SECTION(FILE **File);
static void Convert2FullMatrix(void);

void ReadProblem() {
    int i, j, K;

    if (!(ProblemFile = fopen(ProblemFileName, "r")))
        eprintf("Cannot open PROBLEM_FILE: \"%s\"", ProblemFileName);
    if (TraceLevel >= 1)
        printff("Reading PROBLEM_FILE: \"%s\" ... ", ProblemFileName);
    FreeStructures();
    FirstNode = 0;
    WeightFormat = -1;
    WeightType = FLOOR_2D;
    CoordType = TWOD_COORDS;
    Name = Copy("Unnamed");
    Type = Copy("CVRPTW");
    EdgeWeightType = Copy("FLOOR_2D");
    EdgeWeightFormat = NULL;
    EdgeDataFormat = NodeCoordType = DisplayDataType = NULL;
    Distance = Distance_FLOOR_2D;
    _C = 0;
    c = c_FLOOR_2D;
    DistanceLimit = DBL_MAX;
    Read_DIMENSION();
    Read_NAME();
    Read_SALESMEN_and_CAPACITY();
    CheckSpecificationPart();
    if (!FirstNode)
        CreateNodes();
    Read_NODE_SECTION();

    Swaps = 0;

    /* Adjust parameters */
    if (Seed == 0)
        Seed = (unsigned)time(0);
    if (Precision == 0)
        Precision = 100;
    if (InitialStepSize == 0)
        InitialStepSize = 1;
    if (MaxSwaps < 0)
        MaxSwaps = Dimension;
    if (KickType > Dimension / 2)
        KickType = Dimension / 2;
    if (Runs == 0)
        Runs = 10;
    if (MaxCandidates > Dimension - 1)
        MaxCandidates = Dimension - 1;
    if (ExtraCandidates > Dimension - 1)
        ExtraCandidates = Dimension - 1;
    if (Scale < 1)
        Scale = 1;
    if (SubproblemSize >= Dimension)
        SubproblemSize = Dimension;
    else if (SubproblemSize == 0) {
        if (AscentCandidates > Dimension - 1)
            AscentCandidates = Dimension - 1;
        if (InitialPeriod < 0) {
            InitialPeriod = Dimension / 2;
            if (InitialPeriod < 100)
                InitialPeriod = 100;
        }
        if (Excess < 0)
            Excess = 1.0 / DimensionSaved * Salesmen;
        if (MaxTrials == -1)
            MaxTrials = Dimension;
        HeapMake(Dimension);
    }
    if (POPMUSIC_MaxNeighbors > Dimension - 1)
        POPMUSIC_MaxNeighbors = Dimension - 1;
    if (POPMUSIC_SampleSize > Dimension)
        POPMUSIC_SampleSize = Dimension;
    Depot = &NodeSet[MTSPDepot];
    if (ProblemType == CVRP) {
        Node *N;
        int MinSalesmen;
        if (Capacity <= 0)
            eprintf("CAPACITY not specified");
        TotalDemand = 0;
        N = FirstNode;
        do
            TotalDemand += N->Demand;
        while ((N = N->Suc) != FirstNode);
        MinSalesmen = TotalDemand / Capacity + (TotalDemand % Capacity != 0);
        if (Salesmen == 1) {
            Salesmen = MinSalesmen;
            if (Salesmen > Dimension)
                eprintf("CVRP: SALESMEN larger than DIMENSION");
        } else if (Salesmen < MinSalesmen)
            eprintf("CVRP: SALESMEN too small to meet demand");
        assert(Salesmen >= 1 && Salesmen <= Dimension);
        if (Salesmen == 1)
            assert(ProblemType == TSP);
    }

    TSPTW_Makespan = 0;
    if (Salesmen > 1) {
        if (Salesmen > Dim && MTSPMinSize > 0)
            eprintf("Too many salesmen/vehicles (>= DIMENSION)");
        MTSP2TSP();
    }

    if (SubproblemSize > 0 || SubproblemTourFile)
        eprintf("Partitioning not implemented for constrained problems");

    Depot->DepotId = 1;
    for (i = Dim + 1; i <= DimensionSaved; i++)
        NodeSet[i].DepotId = i - Dim + 1;
    if (Dimension != DimensionSaved) {
        NodeSet[Depot->Id + DimensionSaved].DepotId = 1;
        for (i = Dim + 1; i <= DimensionSaved; i++)
            NodeSet[i + DimensionSaved].DepotId = i - Dim + 1;
    }
    if (Scale < 1)
        Scale = 1;
    else {
        Node *Ni = FirstNode;
        do {
            Ni->Earliest *= Scale;
            Ni->Latest *= Scale;
            Ni->ServiceTime *= Scale;
        } while ((Ni = Ni->Suc) != FirstNode);
        ServiceTime *= Scale;
        RiskThreshold *= Scale;
        if (DistanceLimit != DBL_MAX)
            DistanceLimit *= Scale;
    }
    if (ServiceTime != 0) {
        for (i = 1; i <= Dim; i++)
            NodeSet[i].ServiceTime = ServiceTime;
        Depot->ServiceTime = 0;
    }
    if (CostMatrix == 0 && Dimension <= MaxMatrixDimension && Distance != 0 && Distance != Distance_1 && Distance != Distance_EXPLICIT &&
        Distance != Distance_LARGE && Distance != Distance_ATSP && Distance != Distance_MTSP && Distance != Distance_SPECIAL) {
        Node *Ni, *Nj;
        CostMatrix = (int *)calloc((size_t)Dim * (Dim - 1) / 2, sizeof(int));
        Ni = FirstNode->Suc;
        do {
            Ni->C = &CostMatrix[(size_t)(Ni->Id - 1) * (Ni->Id - 2) / 2] - 1;
            if (ProblemType != HPP || Ni->Id <= Dim)
                for (Nj = FirstNode; Nj != Ni; Nj = Nj->Suc)
                    Ni->C[Nj->Id] = Fixed(Ni, Nj) ? 0 : Distance(Ni, Nj);
            else
                for (Nj = FirstNode; Nj != Ni; Nj = Nj->Suc)
                    Ni->C[Nj->Id] = 0;
        } while ((Ni = Ni->Suc) != FirstNode);
        c = 0;
        WeightType = EXPLICIT;
    }
    if (ProblemType == TSPTW || ProblemType == CVRPTW || ProblemType == VRPBTW || ProblemType == PDPTW || ProblemType == RCTVRPTW) {
        M = INT_MAX / 2 / Precision;
        for (i = 1; i <= Dim; i++) {
            Node *Ni = &NodeSet[i];
            for (j = 1; j <= Dim; j++) {
                Node *Nj = &NodeSet[j];
                if (Ni != Nj && Ni->Earliest + Ni->ServiceTime + Ni->C[j] > Nj->Latest)
                    Ni->C[j] = M;
            }
        }
        if (ProblemType == TSPTW) {
            for (i = 1; i <= Dim; i++)
                for (j = 1; j <= Dim; j++)
                    if (j != i)
                        NodeSet[i].C[j] += NodeSet[i].ServiceTime;
        }
    }

#ifdef CAVA_CACHE
    _C = WeightType == EXPLICIT ? C_EXPLICIT : C_FUNCTION;
#else
    C = WeightType == EXPLICIT ? C_EXPLICIT : C_FUNCTION;
#endif

    D = WeightType == EXPLICIT ? D_EXPLICIT : D_FUNCTION;
    if (ProblemType != CVRP && ProblemType != CVRPTW && ProblemType != CTSP && ProblemType != STTSP && ProblemType != TSP &&
        ProblemType != ATSP) {
        M = INT_MAX / 2 / Precision;
        for (i = Dim + 1; i <= DimensionSaved; i++) {
            for (j = 1; j <= DimensionSaved; j++) {
                if (j == i)
                    continue;
                if (j == MTSPDepot || j > Dim)
                    NodeSet[i].C[j] = NodeSet[MTSPDepot].C[j] = M;
                NodeSet[i].C[j] = NodeSet[MTSPDepot].C[j];
                NodeSet[j].C[i] = NodeSet[j].C[MTSPDepot];
            }
        }
        if (ProblemType == CCVRP || ProblemType == OVRP)
            for (i = 1; i <= Dim; i++)
                NodeSet[i].C[MTSPDepot] = 0;
    }
    if (Precision > 1 && CostMatrix) {
        for (i = 2; i <= Dim; i++) {
            Node *N = &NodeSet[i];
            for (j = 1; j < i; j++)
                if (N->C[j] * Precision / Precision != N->C[j])
                    eprintf("PRECISION (= %d) is too large", Precision);
        }
    }
    /* if (SubsequentMoveType == 0) {
        SubsequentMoveType = MoveType;
        SubsequentMoveTypeSpecial = MoveTypeSpecial;
    } */
    K = MoveType >= SubsequentMoveType || !SubsequentPatching ? MoveType : SubsequentMoveType;
    if (PatchingC > K)
        PatchingC = K;
    if (PatchingA > 1 && PatchingA >= PatchingC)
        PatchingA = PatchingC > 2 ? PatchingC - 1 : 1;
    if (NonsequentialMoveType == -1 || NonsequentialMoveType > K + PatchingC + PatchingA - 1)
        NonsequentialMoveType = K + PatchingC + PatchingA - 1;
    /* if (PatchingC >= 1) {
        BestMove = BestSubsequentMove = BestKOptMove;
        if (!SubsequentPatching && SubsequentMoveType <= 5) {
            MoveFunction BestOptMove[] =
                { 0, 0, Best2OptMove, Best3OptMove,
                Best4OptMove, Best5OptMove
            };
            BestSubsequentMove = BestOptMove[SubsequentMoveType];
        }
    } else {
        MoveFunction BestOptMove[] = { 0, 0, Best2OptMove, Best3OptMove,
            Best4OptMove, Best5OptMove
        };
        BestMove = MoveType <= 5 ? BestOptMove[MoveType] : BestKOptMove;
        BestSubsequentMove = SubsequentMoveType <= 5 ?
            BestOptMove[SubsequentMoveType] : BestKOptMove;
    } */
    /* if (MoveTypeSpecial)
        BestMove = BestSpecialOptMove; */
    /* if (SubsequentMoveTypeSpecial)
        BestSubsequentMove = BestSpecialOptMove; */
    if (ProblemType == HCP || ProblemType == HPP)
        MaxCandidates = 0;
    if (TraceLevel >= 1) {
        printff("done\n");
        PrintParameters();
    } else
        printff("PROBLEM_FILE = %s\n", ProblemFileName ? ProblemFileName : "");
    fclose(ProblemFile);
    if (InitialTourFileName)
        ReadTour(InitialTourFileName, &InitialTourFile);
    if (InputTourFileName)
        ReadTour(InputTourFileName, &InputTourFile);
    if (SubproblemTourFileName && SubproblemSize > 0)
        ReadTour(SubproblemTourFileName, &SubproblemTourFile);
    if (MergeTourFiles >= 1) {
        free(MergeTourFile);
        MergeTourFile = (FILE **)malloc(MergeTourFiles * sizeof(FILE *));
        for (i = 0; i < MergeTourFiles; i++)
            ReadTour(MergeTourFileName[i], &MergeTourFile[i]);
    }

    if (Dim > 2000) {
        InitialPeriod = 1000;
        POPMUSIC_Solutions = 100;
    }

    free(LastLine);
    LastLine = 0;
}

static int TwoDWeightType() {
    if (Asymmetric)
        return 0;
    return WeightType == EUC_2D || WeightType == MAX_2D || WeightType == MAN_2D || WeightType == CEIL_2D || WeightType == FLOOR_2D ||
           WeightType == GEO || WeightType == GEOM || WeightType == GEO_MEEUS || WeightType == GEOM_MEEUS || WeightType == ATT ||
           WeightType == TOR_2D || (WeightType == SPECIAL && CoordType == TWOD_COORDS);
}

static int ThreeDWeightType() {
    if (Asymmetric)
        return 0;
    return WeightType == EUC_3D || WeightType == MAX_3D || WeightType == MAN_3D || WeightType == CEIL_3D || WeightType == FLOOR_3D ||
           WeightType == TOR_3D || WeightType == XRAY1 || WeightType == XRAY2 || (WeightType == SPECIAL && CoordType == THREED_COORDS);
}

static void CheckSpecificationPart() {
    if (Dimension < 3)
        eprintf("DIMENSION < 3 or not specified");
    if (WeightType == -1 && !Asymmetric && ProblemType != HCP && ProblemType != HPP && !EdgeWeightType && ProblemType != STTSP)
        eprintf("EDGE_WEIGHT_TYPE is missing");
    if (WeightType == EXPLICIT && WeightFormat == -1 && !EdgeWeightFormat)
        eprintf("EDGE_WEIGHT_FORMAT is missing");
    if (WeightType == EXPLICIT && WeightFormat == FUNCTION)
        eprintf("Conflicting EDGE_WEIGHT_TYPE and EDGE_WEIGHT_FORMAT");
    if (WeightType != EXPLICIT && (WeightType != SPECIAL || CoordType != NO_COORDS) && WeightType != -1 && WeightFormat != -1 &&
        WeightFormat != FUNCTION)
        eprintf("Conflicting EDGE_WEIGHT_TYPE and EDGE_WEIGHT_FORMAT");
    if ((ProblemType == ATSP || ProblemType == SOP) && WeightType != EXPLICIT && WeightType != -1)
        eprintf("Conflicting TYPE and EDGE_WEIGHT_TYPE");
    if (CandidateSetType == DELAUNAY && !TwoDWeightType() && MaxCandidates > 0)
        eprintf("Illegal EDGE_WEIGHT_TYPE for CANDIDATE_SET_TYPE = DELAUNAY");
    if (CandidateSetType == QUADRANT && !TwoDWeightType() && !ThreeDWeightType() && MaxCandidates + ExtraCandidates > 0)
        eprintf("Illegal EDGE_WEIGHT_TYPE for CANDIDATE_SET_TYPE = QUADRANT");
    if (ExtraCandidateSetType == QUADRANT && !TwoDWeightType() && !ThreeDWeightType() && ExtraCandidates > 0)
        eprintf(
            "Illegal EDGE_WEIGHT_TYPE for EXTRA_CANDIDATE_SET_TYPE = "
            "QUADRANT");
    if (InitialTourAlgorithm == QUICK_BORUVKA && !TwoDWeightType() && !ThreeDWeightType())
        eprintf(
            "Illegal EDGE_WEIGHT_TYPE for INITIAL_TOUR_ALGORITHM = "
            "QUICK-BORUVKA");
    if (InitialTourAlgorithm == SIERPINSKI && !TwoDWeightType())
        eprintf(
            "Illegal EDGE_WEIGHT_TYPE for INITIAL_TOUR_ALGORITHM = "
            "SIERPINSKI");
    if (DelaunayPartitioning && !TwoDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for DELAUNAY specification");
    if (KarpPartitioning && !TwoDWeightType() && !ThreeDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for KARP specification");
    if (KCenterPartitioning && !TwoDWeightType() && !ThreeDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for K-CENTER specification");
    if (KMeansPartitioning && !TwoDWeightType() && !ThreeDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for K-MEANS specification");
    if (MoorePartitioning && !TwoDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for MOORE specification");
    if (RohePartitioning && !TwoDWeightType() && !ThreeDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for ROHE specification");
    if (SierpinskiPartitioning && !TwoDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for SIERPINSKI specification");
    if (SubproblemBorders && !TwoDWeightType() && !ThreeDWeightType())
        eprintf("Illegal EDGE_WEIGHT_TYPE for BORDERS specification");
    if (InitialTourAlgorithm == MTSP_ALG && Asymmetric)
        eprintf(
            "INTIAL_TOUR_ALGORITHM = MTSP is not applicable for "
            "asymetric problems");
}

static char *Copy(const char *S) {
    char *Buffer;

    if (!S || strlen(S) == 0)
        return 0;
    Buffer = (char *)malloc(strlen(S) + 1);
    strcpy(Buffer, S);
    return Buffer;
}

static void CreateNodes() {
    Node *Prev = 0, *N = 0;
    int i;

    if (Dimension <= 0)
        eprintf("DIMENSION is not positive (or not specified)");
    if (Asymmetric) {
        Dim = DimensionSaved;
        DimensionSaved = Dimension + Salesmen - 1;
        Dimension = 2 * DimensionSaved;
    } else if (ProblemType == HPP) {
        Dimension++;
        if (Dimension > MaxMatrixDimension)
            eprintf("DIMENSION too large in HPP problem");
    }
    NodeSet = (Node *)calloc(Dimension + 1, sizeof(Node));
    assert(Dimension > 1);
    for (i = 1; i <= Dimension; i++, Prev = N) {
        N = &NodeSet[i];
        if (i == 1)
            FirstNode = N;
        else
            Link(Prev, N);
        N->Id = N->OriginalId = i;
        if (MergeTourFiles >= 1)
            N->MergeSuc = (Node **)calloc(MergeTourFiles, sizeof(Node *));
        N->Earliest = 0;
        N->Latest = INT_MAX;
    }
    Link(N, FirstNode);
}

static int FixEdge(Node *Na, Node *Nb) {
    if (!Na->FixedTo1 || Na->FixedTo1 == Nb)
        Na->FixedTo1 = Nb;
    else if (!Na->FixedTo2 || Na->FixedTo2 == Nb)
        Na->FixedTo2 = Nb;
    else
        return 0;
    if (!Nb->FixedTo1 || Nb->FixedTo1 == Na)
        Nb->FixedTo1 = Na;
    else if (!Nb->FixedTo2 || Nb->FixedTo1 == Na)
        Nb->FixedTo2 = Na;
    else
        return 0;
    return 1;
}

static void Read_NAME() { Name = Copy(ReadLine(ProblemFile)); }

static void Read_DIMENSION() {
    FILE *prob_file;
    if (!(prob_file = fopen(ProblemFileName, "r")))
        eprintf("Cannot open PROBLEM_FILE: \"%s\"", ProblemFileName);
    char *Line;
    int tempi;
    double tempd;
    while ((Line = ReadLine(prob_file)))
        if (sscanf(Line, "%d %lf %lf %d %lf %lf %lf", &tempi, &tempd, &tempd, &tempi, &tempd, &tempd, &tempd) == 7)
            ++Dimension;
    DimensionSaved = Dim = Dimension;
    fclose(prob_file);
}

static void Read_NODE_SECTION() {
    char *Line;
    int Id, Demand;
    double X, Y, Earliest, Latest, ServiceTime;
    while ((Line = ReadLine(ProblemFile))) {
        // printff("Read : %s\n", Line);
        if (sscanf(Line, "%d %lf %lf %d %lf %lf %lf", &Id, &X, &Y, &Demand, &Earliest, &Latest, &ServiceTime) == 7) {
            NodeSet[Id + 1].Id = Id + 1;
            NodeSet[Id + 1].X = X;
            NodeSet[Id + 1].Y = Y;
            NodeSet[Id + 1].Demand = Demand;
            NodeSet[Id + 1].Earliest = Earliest;
            NodeSet[Id + 1].Latest = Latest;
            NodeSet[Id + 1].ServiceTime = ServiceTime;
            // printf("Written: %d %.1f %.1f %d %.1f %.1f %.1f\n", NodeSet[Id + 1].Id, NodeSet[Id + 1].X, NodeSet[Id + 1].Y, NodeSet[Id +
            // 1].Demand,
            //       NodeSet[Id + 1].Earliest, NodeSet[Id + 1].Latest, NodeSet[Id + 1].ServiceTime);
        }
    }
    if (Asymmetric)
        Convert2FullMatrix();
}

static void Read_SALESMEN_and_CAPACITY() {
    char *Line;
    int veh;
    while ((Line = ReadLine(ProblemFile)) && (sscanf(Line, "%d %d", &veh, &Capacity) != 2))
        ;
    if (InitialSolFileName == NULL)
        Salesmen = veh;
}

static void Convert2FullMatrix() {
    int n = DimensionSaved, i, j;
    Node *Ni, *Nj;

    if (Scale < 1)
        Scale = 1;
    if (n > MaxMatrixDimension) {
        OldDistance = Distance;
        Distance = Distance_Asymmetric;
        for (i = 1; i <= n; i++) {
            Ni = &NodeSet[i];
            Nj = &NodeSet[i + n];
            Nj->X = Ni->X;
            Nj->Y = Ni->Y;
            Nj->Z = Ni->Z;
            FixEdge(Ni, Nj);
        }
        return;
    }
    CostMatrix = (int *)calloc((size_t)n * n, sizeof(int));
    for (i = 1; i <= n; i++) {
        Ni = &NodeSet[i];
        Ni->C = &CostMatrix[(size_t)(i - 1) * n] - 1;
    }
    for (i = 1; i <= Dim; i++) {
        Ni = &NodeSet[i];
        for (j = i + 1; j <= Dim; j++) {
            Nj = &NodeSet[j];
            Ni->C[j] = Nj->C[i] = Distance(Ni, Nj);
        }
    }
    for (i = 1; i <= n; i++)
        FixEdge(&NodeSet[i], &NodeSet[i + n]);
    c = 0;
    OriginalDistance = Distance;
    Distance = Distance_ATSP;
    WeightType = -1;
}

/*
 The ReadTour function reads a tour from a file.

 The format is as follows:

 OPTIMUM = <real>
 Known optimal tour length. A run will be terminated as soon as a tour
 length less than or equal to optimum is achieved.
 Default: MINUS_INFINITY.

 TOUR_SECTION :
 A tour is specified in this section. The tour is given by a list of integers
 giving the sequence in which the nodes are visited in the tour. The tour is
 terminated by a -1.

 EOF
 Terminates the input data. The entry is optional.

 Other keywords in TSPLIB format may be included in the file, but they are
 ignored.
 */

void ReadTour(char *FileName, FILE **File) {
    char *Line, *Keyword, *Token;
    unsigned int i;
    int Done = 0;

    if (!(*File = fopen(FileName, "r")))
        eprintf("Cannot open tour file: \"%s\"", FileName);
    while ((Line = ReadLine(*File))) {
        if (!(Keyword = strtok(Line, Delimiters)))
            continue;
        for (i = 0; i < strlen(Keyword); i++)
            Keyword[i] = (char)toupper(Keyword[i]);
        if (!strcmp(Keyword, "COMMENT") || !strcmp(Keyword, "DEMAND_SECTION") || !strcmp(Keyword, "DEPOT_SECTION") ||
            !strcmp(Keyword, "DISPLAY_DATA_SECTION") || !strcmp(Keyword, "DISPLAY_DATA_TYPE") || !strcmp(Keyword, "EDGE_DATA_FORMAT") ||
            !strcmp(Keyword, "EDGE_DATA_SECTION") || !strcmp(Keyword, "EDGE_WEIGHT_FORMAT") || !strcmp(Keyword, "EDGE_WEIGHT_SECTION") ||
            !strcmp(Keyword, "EDGE_WEIGHT_TYPE") || !strcmp(Keyword, "FIXED_EDGES_SECTION") || !strcmp(Keyword, "NAME") ||
            !strcmp(Keyword, "NODE_COORD_SECTION") || !strcmp(Keyword, "NODE_COORD_TYPE") || !strcmp(Keyword, "TYPE"))
            ;
        else if (strcmp(Keyword, "OPTIMUM") == 0) {
            if (!(Token = strtok(0, Delimiters)) || !sscanf(Token, GainInputFormat, &Optimum))
                eprintf("[%s] (OPTIMUM): Integer expected", FileName);
        } else if (strcmp(Keyword, "DIMENSION") == 0) {
            int Dim = 0;
            if (!(Token = strtok(0, Delimiters)) || !sscanf(Token, "%d", &Dim))
                eprintf("[%s] (DIMENSION): Integer expected", FileName);
            if (Dim != DimensionSaved && Dim != Dimension) {
                printff("Dim = %d, DimensionSaved = %d, Dimension = %d\n", Dim, DimensionSaved, Dimension);
                eprintf("[%s] (DIMENSION): does not match problem dimension", FileName);
            }
        } else if (!strcmp(Keyword, "TOUR_SECTION")) {
            Read_TOUR_SECTION(File);
            Done = 1;
        } else if (!strcmp(Keyword, "EOF"))
            break;
        else
            eprintf("[%s] Unknown Keyword: %s", FileName, Keyword);
    }
    if (!Done)
        eprintf("Missing TOUR_SECTION in tour file: \"%s\"", FileName);
    fclose(*File);
}

static void Read_TOUR_SECTION(FILE **File) {
    Node *First = 0, *Last = 0, *N, *Na;
    int i, k;

    if (TraceLevel >= 1) {
        printff("Reading ");
        if (File == &InitialTourFile)
            printff("INITIAL_TOUR_FILE: \"%s\" ... ", InitialTourFileName);
        else if (File == &InputTourFile)
            printff("INPUT_TOUR_FILE: \"%s\" ... ", InputTourFileName);
        else if (File == &SubproblemTourFile)
            printff("SUBPROBLEM_TOUR_FILE: \"%s\" ... ", SubproblemTourFileName);
        else
            for (i = 0; i < MergeTourFiles; i++)
                if (File == &MergeTourFile[i])
                    printff("MERGE_TOUR_FILE: \"%s\" ... ", MergeTourFileName[i]);
    }
    if (!FirstNode)
        CreateNodes();
    N = FirstNode;
    do
        N->V = 0;
    while ((N = N->Suc) != FirstNode);
    if (ProblemType == HPP)
        Dimension--;
    if (Asymmetric)
        Dimension = DimensionSaved;
    int b = 0;
    if (!fscanint(*File, &i))
        i = -1;
    else if (i == 0) {
        b = 1;
        i++;
    }
    for (k = 0; k <= Dimension && i != -1; k++) {
        if (i <= 0 || i > Dimension)
            eprintf("TOUR_SECTION: Node number out of range: %d", i);
        N = &NodeSet[i];
        if (N->V == 1 && k != Dimension)
            eprintf("TOUR_SECTION: Node number occurs twice: %d", N->Id);
        N->V = 1;
        if (k == 0)
            First = Last = N;
        else {
            if (Asymmetric) {
                Na = N + Dimension;
                Na->V = 1;
            } else
                Na = 0;
            if (File == &InitialTourFile) {
                if (!Na)
                    Last->InitialSuc = N;
                else {
                    Last->InitialSuc = Na;
                    Na->InitialSuc = N;
                }
            } else if (File == &InputTourFile) {
                if (!Na)
                    Last->InputSuc = N;
                else {
                    Last->InputSuc = Na;
                    Na->InputSuc = N;
                }
            } else if (File == &SubproblemTourFile) {
                if (!Na)
                    (Last->SubproblemSuc = N)->SubproblemPred = Last;
                else {
                    (Last->SubproblemSuc = Na)->SubproblemPred = Last;
                    (Na->SubproblemSuc = N)->SubproblemPred = Na;
                }
            } else {
                for (i = 0; i < MergeTourFiles; i++) {
                    if (File == &MergeTourFile[i]) {
                        if (!Na) {
                            Last->MergeSuc[i] = N;
                            if (i == 0)
                                N->MergePred = Last;
                        } else {
                            Last->MergeSuc[i] = Na;
                            Na->MergeSuc[i] = N;
                            if (i == 0) {
                                Na->MergePred = Last;
                                N->MergePred = Na;
                            }
                        }
                    }
                }
            }
            Last = N;
        }
        if (k < Dimension) {
            fscanint(*File, &i);
            if (b)
                if (i >= 0)
                    i++;
        }
        if (k == Dimension - 1)
            i = First->Id;
    }
    N = FirstNode;
    do {
        if (!N->V)
            eprintf("TOUR_SECTION: Node is missing: %d", N->Id);
    } while ((N = N->Suc) != FirstNode);
    if (File == &SubproblemTourFile) {
        do {
            if (N->FixedTo1 && N->SubproblemPred != N->FixedTo1 && N->SubproblemSuc != N->FixedTo1)
                eprintf(
                    "Fixed edge (%d, %d) "
                    "does not belong to subproblem tour",
                    N->Id, N->FixedTo1->Id);
            if (N->FixedTo2 && N->SubproblemPred != N->FixedTo2 && N->SubproblemSuc != N->FixedTo2)
                eprintf(
                    "Fixed edge (%d, %d) "
                    "does not belong to subproblem tour",
                    N->Id, N->FixedTo2->Id);
        } while ((N = N->Suc) != FirstNode);
    }
    if (ProblemType == HPP)
        Dimension++;
    if (Asymmetric)
        Dimension *= 2;
    if (TraceLevel >= 1)
        printff("done\n");
}
