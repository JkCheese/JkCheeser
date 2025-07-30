#ifndef EVALTUNER_H
#define EVALTUNER_H

#include "evalparams.h"
#include "board.h"
#include "magic.h"

typedef struct {
    char fen[128];
    double wdl; // [0.0, 1.0]
} TrainingEntry;

typedef struct {    
    int passed_pawn_bonus;
    
    int knight_outpost_bonus;

    int rook_semi_open_file_bonus;
    int rook_open_file_bonus;
    int blind_swine_rooks_bonus;

    int tropism[6][8];
    
    int king_zone_hits_by_type[6];
    int king_zone_attacker_count;
} FeatureCounts;

typedef struct {
    int index;       // Linear index of the parameter used
    double weight;   // How much it contributed to the final eval
} FeatureContribution;

extern const int tropism_feature_idx[6][2];

#define MAX_FEATURES_PER_POSITION 16384

typedef struct {
    double score;
    FeatureContribution features[MAX_FEATURES_PER_POSITION];
    int num_features;
} EvalResult;

int load_dataset(const char* path, TrainingEntry* entries, int max_entries);
EvalResult evaluate_with_features(const Position* pos, const EvalParamsDouble* params, const MagicData* magic);
double find_best_k(const MagicData* magic, const EvalParamsDouble* params, const TrainingEntry* data, int n);
double sigmoid(double x, double k);
double sigmoid_derivative(double x, double k);
double compute_loss(const MagicData* magic, const EvalParamsDouble* params, const TrainingEntry* data, int n, double k);
void run_minibatch_training(const MagicData* magic, EvalParamsDouble* params, const TrainingEntry* data, int batch_size, double learning_rate, int iterations, double sigmoid_k, const char* output_file);
void convert_params_to_integer(const EvalParamsDouble* in, EvalParams* out);
void save_evalparams_text(const char* path, const EvalParams* p);
void run_tuner_main(const MagicData* magic, const char* dataset_path, const char* output_prefix);

#endif