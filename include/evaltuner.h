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
    int index;       // Linear index of the parameter used
    double weight;   // How much it contributed to the final eval
} FeatureContribution;

#define MAX_FEATURES_PER_POSITION 2048

typedef struct {
    double score;
    FeatureContribution features[MAX_FEATURES_PER_POSITION];
    int num_features;
} EvalResult;

int load_dataset(const char* path, TrainingEntry* entries, int max_entries);
EvalResult evaluate_with_features(const Position* pos, const EvalParamsDouble* params, const MagicData* magic);
double find_best_k(const EvalParamsDouble* params, const TrainingEntry* data, int n, const MagicData* magic);
double sigmoid(double x, double k);
double sigmoid_derivative(double x, double k);
double compute_loss(const EvalParamsDouble* params, const TrainingEntry* data, int n, double k, const MagicData* magic);
void run_minibatch_training(
    EvalParamsDouble* params,
    const TrainingEntry* data,
    int num_entries,
    const MagicData* magic,
    int batch_size,
    double learning_rate,
    int iterations,
    double sigmoid_k
);
void convert_params_to_integer(const EvalParamsDouble* in, EvalParams* out);
void save_evalparams_text(const char* path, const EvalParams* p);
void run_tuner_main(const char* dataset_path, const char* output_prefix, const MagicData* magic);

#endif