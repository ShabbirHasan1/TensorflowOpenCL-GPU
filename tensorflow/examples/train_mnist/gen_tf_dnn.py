# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

"""A deep MNIST classifier using convolutional layers.

See extensive documentation at
https://www.tensorflow.org/get_started/mnist/pros
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import sys
import tensorflow as tf
from tensorflow.examples.tutorials.mnist import input_data

def deepnn(X):
    """deepnn builds the graph for a deep net for classifying digits.

    Args:
    X: an input tensor with the dimensions (N_examples, 784), where 784 is the
    number of pixels in a standard MNIST image.

    Returns:
    A tuple (y, keepProb). y is a tensor of shape (N_examples, 10), with values
    equal to the logits of classifying the digit into one of 10 classes (the
    digits 0-9). keepProb is a scalar placeholder for the probability of
    dropout.
    """
    # Reshape to use within a convolutional neural net.
    # Last dimension is for "features" - there is only one here, since images are
    # grayscale -- it would be 3 for an RGB image, 4 for RGBA, etc.
    with tf.name_scope('reshape'):
        x_image = tf.reshape(X, [-1, 28, 28, 1])

    # First convolutional layer - maps one grayscale image to 32 feature maps.
    with tf.name_scope('conv1'):
        W_conv1 = weight_variable([5, 5, 1, 32])
        b_conv1 = bias_variable([32])
        h_conv1 = tf.nn.relu(conv2d(x_image, W_conv1) + b_conv1)

    # Pooling layer - downsamples by 2X.
    with tf.name_scope('pool1'):
        h_pool1 = max_pool_2x2(h_conv1)

    # Second convolutional layer -- maps 32 feature maps to 64.
    with tf.name_scope('conv2'):
        W_conv2 = weight_variable([5, 5, 32, 64])
        b_conv2 = bias_variable([64])
        h_conv2 = tf.nn.relu(conv2d(h_pool1, W_conv2) + b_conv2)

    # Second pooling layer.
    with tf.name_scope('pool2'):
        h_pool2 = max_pool_2x2(h_conv2)

    # Fully connected layer 1 -- after 2 round of downsampling, our 28x28 image
    # is down to 7x7x64 feature maps -- maps this to 1024 features.
    with tf.name_scope('fc1'):
        W_fc1 = weight_variable([7 * 7 * 64, 1024])
        b_fc1 = bias_variable([1024])
        h_pool2_flat = tf.reshape(h_pool2, [-1, 7 * 7 * 64])
        h_fc1 = tf.nn.relu(tf.matmul(h_pool2_flat, W_fc1) + b_fc1)

    # Dropout - controls the complexity of the model, prevents co-adaptation of
    # features.
    with tf.name_scope('Dropout'):
        keepProb = tf.placeholder(tf.float32)
        h_fc1_drop = tf.nn.dropout(h_fc1, keepProb)

    # Map the 1024 features to 10 classes, one for each digit
    with tf.name_scope('fc2'):
        W_fc2 = weight_variable([1024, 10])
        b_fc2 = bias_variable([10])
        Yconv = tf.matmul(h_fc1_drop, W_fc2) + b_fc2

    return Yconv, keepProb

def conv2d(X, W):
    """conv2d returns a 2d convolution layer with full stride."""
    return tf.nn.conv2d(X, W, strides=[1, 1, 1, 1], padding='SAME')


def max_pool_2x2(X):
    """max_pool_2x2 downsamples a feature map by 2X."""
    return tf.nn.max_pool(X, ksize=[1, 2, 2, 1],
                        strides=[1, 2, 2, 1], padding='SAME')


def weight_variable(shape):
    """weight_variable generates a weight variable of a given shape."""
    initial = tf.truncated_normal(shape, stddev=0.1)
    return tf.Variable(initial)


def bias_variable(shape):
    """bias_variable generates a bias variable of a given shape."""
    initial = tf.constant(0.1, shape=shape)
    return tf.Variable(initial)


def main(_):

    # Import data
    mnist = input_data.read_data_sets(FLAGS.mnistDataDir, one_hot=True)

    # Training parameters
    maxEpochs = FLAGS.maxEpochs
    batchSize = FLAGS.batchSize
    testStep = FLAGS.testStep

    # Network parameters
    n_input = 784 # MNIST data input (img shape: 28*28)
    n_classes = 10 # MNIST total classes (0-9 digits)

    # Create the model
    X = tf.placeholder(tf.float32, [None, n_input], name="input")
    Y = tf.placeholder(tf.float32, [None, n_classes], name="output")

    # Build the graph for the deep net
    Yconv, keepProb = deepnn(X)

    # Define loss
    loss = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits_v2(
        logits=Yconv, labels=Y))

    # Define optimizer
    with tf.name_scope('adam_optimizer'):
        train_op = tf.train.AdamOptimizer().minimize(loss, name="train")

    # Define accuracy
    prediction = tf.equal(tf.argmax(Yconv, 1), tf.argmax(Y, 1))
    accuracy = tf.reduce_mean(tf.cast(prediction, tf.float32), name="test")

    # Create a summary to monitor cross_entropy tensor
    tf.summary.scalar("loss", loss)
    # Create a summary to monitor accuracy tensor
    tf.summary.scalar("accuracy", accuracy)
    # Merge all summaries into a single op
    merged_summary_op = tf.summary.merge_all()

    # Initializing the variables
    init = tf.initialize_variables(tf.all_variables(), name='init')

    with tf.Session() as sess:
        # Session Init
        sess.run(init)

        # Logger Init
        summaryWriter = tf.summary.FileWriter(FLAGS.logDir, graph=sess.graph)

        # Training
        for step in range( maxEpochs ):
            # Get MNIST training data
            batchImage, batchLabel = mnist.train.next_batch(batchSize)

            # Test training model for every testStep
            if step % testStep == 0:
                # Run accuracy op & summary op to get accuracy & training progress
                acc, summary = sess.run( [ accuracy, merged_summary_op ], \
                    feed_dict={ X: mnist.test.images, Y: mnist.test.labels, keepProb: 1.0})

                # Write accuracy to log file
                summaryWriter.add_summary(summary, step)

                # Print accuracy
                print('step %d, training accuracy %f' % (step, acc))

            # Run training op
            train_op.run(feed_dict={X: batchImage, Y: batchLabel, keepProb: 0.5})

        # Write TF model
        tf.train.write_graph(sess.graph_def,
                            './',
                            'mnist_dnn.pb', as_text=False)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--mnistDataDir', type=str, default='/tmp/tensorflow/mnist/input_data',
                        help='MNIST data directory')
    parser.add_argument('--logDir', type=str, default='/tmp/tensorflow_logs/deepnet',
                        help='Training progress data directory')
    parser.add_argument('--batchSize', type=int, default=50,
                        help='Training batch size')
    parser.add_argument('--maxEpochs', type=int, default=10000,
                        help='Maximum training steps')
    parser.add_argument('--testStep', type=int, default=100,
                        help='Test model accuracy for every testStep iterations')
    FLAGS, unparsed = parser.parse_known_args()
    # Program entry
    tf.app.run(main=main, argv=[sys.argv[0]] + unparsed)
