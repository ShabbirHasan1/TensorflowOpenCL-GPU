
/*
By Cheng Wei on 2018/Jan/24
==============================================================================*/
// A simple program trainging a MNIST TF model using TF C++ API

#include <fstream>
#include <utility>
#include <vector>
#include <iostream>
#include <math.h>
#include <numeric>

#include "tensorflow/cc/ops/const_op.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

// MNIST reader helper
#include "mnist/mnist_reader.hpp"

// These are all common classes it's handy to reference with no namespace.
using tensorflow::Flag;
using tensorflow::Tensor;
using tensorflow::Status;
using tensorflow::string;
using tensorflow::int32;
using namespace tensorflow;
using namespace std;

// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
Status LoadGraph(const string& graph_file_name,
                 unique_ptr<Session>* session) {
  GraphDef graph_def;
  Status load_graph_status =
      ReadBinaryProto(Env::Default(), graph_file_name, &graph_def);
  if (!load_graph_status.ok()) {
    return errors::NotFound("Failed to load compute graph at '",
                                        graph_file_name, "'");
  }
  session->reset(NewSession(SessionOptions()));
  Status session_create_status = (*session)->Create(graph_def);
  if (!session_create_status.ok()) {
    return session_create_status;
  }
  return Status::OK();
}

int main(int argc, char* argv[]) {

  string root_dir         = "/data/local/tmp/";
  string graph_name       = "mnist_100_mlp.pb";
  string mnist_dir        = root_dir + "MNIST_data/";
  string T_input          = "input";
  string T_label          = "output";
  string test_Ops         = "test";
  string train_Ops        = "train";
  int32  input_width      = 28;
  int32  input_height     = 28;
  int32  batch_size       = 50;
  int32  max_steps        = 100000;
  vector<float>
         drop_prob        = { 0.5 } ;

  vector<Flag> flag_list = {
      Flag("root_dir",      &root_dir,
        "interpret graph file names relative to this directory"),
      Flag("graph_name",    &graph_name,    "graph to be executed"),
      Flag("mnist_dir",     &mnist_dir,     "MNIST dataset directory"),
      Flag("T_input",       &T_input,       "name of input Tensor"),
      Flag("T_label",       &T_label,       "name of output Tensor"),
      Flag("test_Ops",      &test_Ops,      "name of cost Ops"),
      Flag("train_Ops",     &train_Ops,     "name of training Ops"),
      Flag("batch_size",    &batch_size,    "training batch size"),
      Flag("max_steps",     &max_steps,     "maximum number of taining steps"),
      Flag("drop_prob",     &drop_prob[0],  "Drop out layer (if any) probability"),
  };

  string usage = Flags::Usage(argv[0], flag_list);
  const bool parse_result = Flags::Parse(&argc, argv, flag_list);
  if (!parse_result) {
    LOG(ERROR) << usage;
    return -1;
  }

  // We need to call this to set up global state for TensorFlow.
  port::InitMain(argv[0], &argc, &argv);
  if (argc > 1) {
    LOG(ERROR) << "Unknown argument " << argv[1] << "\n" << usage;
    return -1;
  }

  LOG(INFO) << "[Root directory] = " << root_dir ;

  // Prepare MNIST dataset
    LOG(INFO) << "[MNIST Dataset Directory] = " << mnist_dir ;

    // Load MNIST data
    mnist::MNIST_dataset<vector, vector<uint8_t>, uint8_t> dataset =
        mnist::read_dataset<vector, vector, uint8_t, uint8_t>(mnist_dir);

    LOG(INFO) << "[MNIST Dataset] Num of Training Images = " <<
      dataset.training_images.size() ;
    LOG(INFO) << "[MNIST Dataset] Num of Training Labels = " <<
      dataset.training_labels.size() ;
    LOG(INFO) << "[MNIST Dataset] Num of Test     Images = " <<
      dataset.test_images.size() ;
    LOG(INFO) << "[MNIST Dataset] Num of Test     Labels = " <<
      dataset.test_labels.size() ;

    input_width  = sqrt( dataset.training_images[0].size() );
    input_height = sqrt( dataset.training_images[0].size() );

    LOG(INFO) << "[MNIST Dataset] Input Image Size       = " <<
      input_width << "x" << input_height ;

  // Load TF model.
  unique_ptr<Session> session;
  string graph_path = io::JoinPath(root_dir, graph_name);
  Status load_graph_status = LoadGraph(graph_path, &session);
  if (!load_graph_status.ok()) {
    LOG(ERROR) << load_graph_status;
    return -1;
  }
  LOG(INFO) << "[TF Model File Loaded From Directory] = " << graph_path ;

  // Initialize our variables
  TF_CHECK_OK( session->Run( {}, {}, {"init"}, nullptr ) );
  LOG(INFO) << "[TF initialize all variables]" ;

  // Load MNIST training data & labels into TF Tensors

    // batch_img_tensor with dimenstion { batch_size, input_width * input_height }
    // = { batch_size, 28*28 }
    Tensor batch_img_tensor( DT_FLOAT, TensorShape( { batch_size, input_width * input_height } ) );

    // batch_label_tensor with dimenstion { batch_size, 1 } = { batch_size, 1 }
    Tensor batch_label_tensor( DT_INT64, TensorShape( { batch_size } ) );

    // dropout_prob_tensor with dimenstion { 1 }
    Tensor dropout_prob_tensor( DT_FLOAT, TensorShape( { 1 } ) );

    for( auto batch_idx = 0 ; batch_idx < dataset.training_images.size() / batch_size;
      batch_idx ++ )
    { // Training Batch Loop

      // If the number of training steps > max_steps then stop training
      if ( ( batch_idx + 1 ) * batch_size > max_steps ){ break; }

      // image vector with dimension { 1, batch_size x input_width x input_height }
      vector<float> batch_img_vec;
      // label vector with dimension { 1, batch_size }
      vector<long int> batch_label_vec;

      for ( auto batch_itr = 0 ; batch_itr < batch_size ; batch_itr ++ )
      { // Within a single input image

        // vec_img with dim { 1, input_width * input_height }
        vector<uint8_t> vec_img = dataset.training_images[ batch_idx * batch_size + batch_itr ];
        for( int pixel = 0 ; pixel < vec_img.size() ; pixel ++ ){
          // Cast image data from uint_8 -> float
          batch_img_vec.push_back( (float)( vec_img[pixel] ) );
        }

        // vec_label with dim { 1, 1 }
        uint8_t vec_label = dataset.training_labels[ batch_idx * batch_size + batch_itr ];
        batch_label_vec.push_back( (long int)(vec_label) );

      } // Within a single input image

      // Copy a batch of image data to TF Tensor
      copy_n( batch_img_vec.begin(), batch_img_vec.size(),
        batch_img_tensor.flat<float>().data() );
      // Copy a batch of label data to TF Tensor
      copy_n( batch_label_vec.begin(), batch_label_vec.size(),
        batch_label_tensor.flat<long long>().data() );

      // Tensor info
      LOG(INFO) << "Batch " << batch_idx << " of data loaded into Tensor\n" <<
      " [Image] " << batch_img_tensor.DebugString() << "\n"
      " [Label] " << batch_label_tensor.DebugString() << "\n";

      // Copy drop out probability to TF Tensor
      copy_n( drop_prob.begin(), drop_prob.size(), dropout_prob_tensor.flat<float>().data() );

      // Traing the model for each batch
      if ( graph_name == "mnist_100_mlp.pb" ){
        TF_CHECK_OK( session->Run( { { T_input, batch_img_tensor },
          { T_label, batch_label_tensor } }, {}, { train_Ops }, nullptr) );
      }else if( graph_name == "mnist_100_cnn.pb" ){
        TF_CHECK_OK( session->Run( { { T_input, batch_img_tensor },
          { T_label, batch_label_tensor }, { "Dropout/Placeholder", dropout_prob_tensor } }, {},
          { train_Ops }, nullptr) );
      }else if( graph_name == "mnist_100_dnn.pb" ){
        TF_CHECK_OK( session->Run( { { T_input, batch_img_tensor },
          { T_label, batch_label_tensor }, { "dropout/Placeholder", dropout_prob_tensor } }, {},
          { train_Ops }, nullptr) );
      }else{
        LOG(ERROR) << "graph_name not recognized";
      }
      LOG(INFO) << "Batch " << batch_idx << " trained\n" ;

  } // End of Training Batch Loop

  vector<float> avg_accu;

  // Load MNIST testing data & labels into TF Tensors
  for( auto batch_idx = 0 ; batch_idx < dataset.test_images.size() / batch_size;
    batch_idx ++ )
  { // Testing Batch Loop

    // test_img_tensor with dimenstion { batch_size, input_width * input_height }
    // = { batch_size, 28*28 }
    Tensor test_img_tensor(DT_FLOAT, TensorShape( { batch_size, input_width * input_height } ) );

    // test_label_tensor with dimenstion { batch_size, 1 } = { batch_size, 1 }
    Tensor test_label_tensor(DT_INT64, TensorShape( { batch_size } ) );

    // image vector with dimension { 1, batch_size x input_width x input_height }
    vector<float> test_img_vec;
    // label vector with dimension { 1, batch_size }
    vector<long int> test_label_vec;

    for ( auto batch_itr = 0 ; batch_itr < batch_size ; batch_itr ++ )
    { // Within a single input image

      // vec_img with dim { 1, input_width * input_height }
      vector<uint8_t> vec_img = dataset.test_images[ batch_idx * batch_size + batch_itr ];
      for( auto pixel = 0 ; pixel < vec_img.size() ; pixel ++ ){
        // Cast image data from uint_8 -> float
        test_img_vec.push_back( (float)( vec_img[pixel] ) );
      }

      // vec_label with dim { 1, 1 }
      uint8_t vec_label = dataset.test_labels[ batch_idx * batch_size + batch_itr ];
      test_label_vec.push_back( (long int)(vec_label) );

    } // Within a single input image

    // Copy a batch testing image data to TF Tensor
    copy_n( test_img_vec.begin(), test_img_vec.size(),
      test_img_tensor.flat<float>().data() );
    // Copy a batch testing label data to TF Tensor
    copy_n( test_label_vec.begin(), test_label_vec.size(),
      test_label_tensor.flat<long long>().data() );

    // Tensor info
    LOG(INFO) << "[Test] Batch " << batch_idx << " loaded into Tensor\n" <<
    " [Image] " << test_img_tensor.DebugString() << "\n"
    " [Label] " << test_label_tensor.DebugString() << "\n";

    // Output Tensor ( a vector list of Tensors )
    vector<Tensor> outputs;
    // Copy drop out probability to TF Tensor
    copy_n( drop_prob.begin(), drop_prob.size(), dropout_prob_tensor.flat<float>().data() );

    // Test the accuracy of TF model
    if ( graph_name == "mnist_100_mlp.pb" ){
      TF_CHECK_OK( session->Run( { { T_input, test_img_tensor },
        { T_label, test_label_tensor } }, { test_Ops }, {}, &outputs ) );
    }else if( graph_name == "mnist_100_cnn.pb" ){
      TF_CHECK_OK( session->Run( { { T_input, test_img_tensor },
        { T_label, test_label_tensor }, { "Dropout/Placeholder", dropout_prob_tensor } }, { test_Ops },
        { }, &outputs) );
    }else if( graph_name == "mnist_100_dnn.pb" ){
      TF_CHECK_OK( session->Run( { { T_input, test_img_tensor },
        { T_label, test_label_tensor }, { "dropout/Placeholder", dropout_prob_tensor } },
        { test_Ops }, { }, &outputs) );
    }else{
      LOG(ERROR) << "graph_name not recognized";
    }

    // Print Accuracy
    LOG(INFO) << "Accuracy " << outputs[0].scalar<float>()(0) * 100 << "\%\n";
    avg_accu.push_back( outputs[0].scalar<float>()(0) );

  } // Testing Batch Loop

  LOG(INFO) << "Overall testing accuracy " << 100 * accumulate(
    avg_accu.begin(), avg_accu.end(), 0.0f) / (int)(dataset.test_images.size() / batch_size);
} // End of main
