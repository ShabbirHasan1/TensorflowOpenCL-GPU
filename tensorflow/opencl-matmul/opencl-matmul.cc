#include <sys/time.h>
#include <time.h>
#include <random>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/graph/default_device.h"

using namespace tensorflow;
using namespace std;

int main(int argc, char* argv[]) {

    if( argc != 3 ){
      cerr << "expected 2 arguments [Size of Matrix] [Num of Runs]" << endl;
      exit(1);
    }

    // Random generator
    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::normal_distribution<> dis{0,5};

    // Timers
    struct timeval start, end;

    string graph_definition = "matmul.pb";
    Session* session;
    GraphDef graph_def;
    SessionOptions opts;
    vector<Tensor> outputs; // Store outputs
    TF_CHECK_OK(ReadBinaryProto(Env::Default(), graph_definition, &graph_def));

    // Set graph options
    graph::SetDefaultDevice("/cpu:0", &graph_def);

    // create a new session
    TF_CHECK_OK(NewSession(opts, &session));

    // Load graph into session
    TF_CHECK_OK(session->Create(graph_def));

    // Matrix size
    int N = atoi( argv[1] );
    Tensor Tx (DT_FLOAT, TensorShape({N ,N}));
    Tensor Ty (DT_FLOAT, TensorShape({N ,N}));

    // Number of runs
    int num_runs = atoi( argv[2] );

    // Matrix initializaiotn
    auto Tx_map = Tx.tensor<float, 2>();
    for( int i = 0 ; i < N ; i ++ ){
      for( auto j = 0 ; j < N ; j ++ ){
        Tx_map(i, j) = dis(gen);
      }
    }
    auto Ty_map = Ty.tensor<float, 2>();
    for( int i = 0 ; i < N ; i ++ ){
      for( auto j = 0 ; j < N ; j ++ ){
        Ty_map(i, j) = dis(gen);
      }
    }

    LOG(INFO) << ">>> [TF] Starting " << num_runs << " TF MatMul runs...";
    gettimeofday(&start, NULL);

    for (int r=0; r<num_runs; r++) {
      // Compute matrix multiplaction result using TF
      TF_CHECK_OK(session->Run({{"x", Tx}, {"y", Ty}}, {"matmul"}, {}, &outputs)); // Get cost
    }
    auto tf_res = outputs[0].matrix<float>();
    // cout << "TF result: \n" << tf_res << endl;

    gettimeofday(&end, NULL);
    double interval = ( end.tv_sec * 1.0e6 + end.tv_usec ) -
      ( start.tv_sec * 1.0e6 + start.tv_usec );
    double runtime = interval / num_runs;
    LOG(INFO) << ">>> Done: took " << runtime << " us per run";

    // Compute matrix multiplaction result using Eigen
    auto Tx_mat = Eigen::Map<Eigen::Matrix<
                              float,           /* scalar element type */
                              Eigen::Dynamic,  /* num_rows is a run-time value */
                              Eigen::Dynamic,  /* num_cols is a run-time value */
                              Eigen::RowMajor  /* tensorflow::Tensor is always row-major */>>(
                                Tx.flat<float>().data(),  /* ptr to data */
                                Tx.dim_size(0),           /* num_rows */
                                Tx.dim_size(1)            /* num_cols */);
    auto Ty_mat = Eigen::Map<Eigen::Matrix<
                              float,           /* scalar element type */
                              Eigen::Dynamic,  /* num_rows is a run-time value */
                              Eigen::Dynamic,  /* num_cols is a run-time value */
                              Eigen::RowMajor  /* tensorflow::Tensor is always row-major */>>(
                                Ty.flat<float>().data(),  /* ptr to data */
                                Ty.dim_size(0),           /* num_rows */
                                Ty.dim_size(1)            /* num_cols */);

    auto eigen_res = Eigen::Map<Eigen::Matrix<
                            float,           /* scalar element type */
                            Eigen::Dynamic,  /* num_rows is a run-time value */
                            Eigen::Dynamic,  /* num_cols is a run-time value */
                            Eigen::RowMajor  /* tensorflow::Tensor is always row-major */>>(
                              Tx.flat<float>().data(),  /* ptr to data */
                              Tx.dim_size(0),           /* num_rows */
                              Ty.dim_size(1)            /* num_cols */);

    LOG(INFO) << ">>> [Eigen] Starting " << num_runs << " Eigen MatMul runs...";
    gettimeofday(&start, NULL);

    eigen_res = ( Tx_mat * Ty_mat );
    // cout << "Eigen result: \n" << eigen_res << endl ;

    gettimeofday(&end, NULL);
    interval = ( end.tv_sec * 1.0e6 + end.tv_usec ) -
      ( start.tv_sec * 1.0e6 + start.tv_usec );
    runtime = interval;
    LOG(INFO) << ">>> Done: took " << runtime << " us per run";

    cout << "Checking results ...\n";
    double accu_err = 0;
    for( auto row = 0 ; row < Tx.dim_size(0) ; row ++ )
    {
      for( auto col = 0 ; col < Ty.dim_size(1) ; col ++ ){
        accu_err += abs( tf_res(row, col) - eigen_res(row, col) );
      }
    }
    cout << "err per unit: " << accu_err/(N*N) << endl;

    return 0;
}
