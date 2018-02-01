#!/bin/bash

bazel build --config=android_arm64 //tensorflow/examples/train_mnist/... \
    --verbose_failures=true \
    --cxxopt="-ferror-limit=1" \
    --jobs=8

REMOTE_DIR='/data/local/tmp'

# Compiled binary code
adb push ../../../bazel-bin/tensorflow/examples/train_mnist/train_mnist $REMOTE_DIR

# Send MNIST dataset to the phone. Notice that you should first create a
# MNIST_data directory using normal priviledge first
adb push ./MNIST_data/t10k-images-idx3-ubyte $REMOTE_DIR/MNIST_data/
adb push ./MNIST_data/train-images-idx3-ubyte $REMOTE_DIR/MNIST_data/
adb push ./MNIST_data/t10k-labels-idx1-ubyte $REMOTE_DIR/MNIST_data/
adb push ./MNIST_data/train-labels-idx1-ubyte $REMOTE_DIR/MNIST_data/

# Send the trained model: mlp/cnn network
adb push mnist_100_cnn.pb $REMOTE_DIR
adb push mnist_100_mlp.pb $REMOTE_DIR
adb push mnist_100_dnn.pb $REMOTE_DIR
