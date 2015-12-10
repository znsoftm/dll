//=======================================================================
// Copyright (c) 2014-2015 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <deque>

#include "catch.hpp"

#define DLL_SVM_SUPPORT

#include "dll/conv_rbm.hpp"
#include "dll/dbn.hpp"
#include "dll/mp_layer.hpp"
#include "dll/avgp_layer.hpp"

#include "mnist/mnist_reader.hpp"
#include "mnist/mnist_utils.hpp"

TEST_CASE("conv_dbn/mnist_9", "max_pooling") {
    typedef dll::dbn_desc<
        dll::dbn_layers<
            dll::conv_rbm_desc<1, 28, 28, 40, 14, 12, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::conv_rbm_desc<40, 14, 12, 40, 8, 10, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::mp_layer_3d_desc<40, 8, 10, 2, 2, 1>::layer_t>>::dbn_t dbn_t;

    REQUIRE(dbn_t::output_size() == 800);

    auto dataset = mnist::read_dataset_direct<std::vector, etl::fast_dyn_matrix<double, 1, 28, 28>>(200);

    REQUIRE(!dataset.training_images.empty());

    mnist::binarize_dataset(dataset);

    auto dbn = std::make_unique<dbn_t>();

    dbn->pretrain(dataset.training_images, 20);

    auto output = dbn->activation_probabilities(dataset.training_images.front());

    REQUIRE(output.size() == 800);

    auto result = dbn->svm_train(dataset.training_images, dataset.training_labels);

    REQUIRE(result);

    auto test_error = dll::test_set(dbn, dataset.training_images, dataset.training_labels, dll::svm_predictor());
    std::cout << "test_error:" << test_error << std::endl;
    REQUIRE(test_error < 0.1);
}

TEST_CASE("conv_dbn/mnist_10", "max_pooling") {
    typedef dll::dbn_desc<
        dll::dbn_layers<
            dll::conv_rbm_desc<1, 28, 28, 40, 20, 21, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::mp_layer_3d_desc<40, 20, 21, 2, 2, 3>::layer_t,
            dll::conv_rbm_desc<20, 10, 7, 40, 8, 5, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::mp_layer_3d_desc<40, 8, 5, 2, 1, 1>::layer_t>>::dbn_t dbn_t;

    REQUIRE(dbn_t::output_size() == 800);

    auto dataset = mnist::read_dataset_direct<std::vector, etl::fast_dyn_matrix<double, 1, 28, 28>>(200);

    REQUIRE(!dataset.training_images.empty());

    mnist::binarize_dataset(dataset);

    auto dbn = std::make_unique<dbn_t>();

    dbn->pretrain(dataset.training_images, 20);

    auto output = dbn->activation_probabilities(dataset.training_images.front());

    REQUIRE(output.size() == 800);

    auto result = dbn->svm_train(dataset.training_images, dataset.training_labels);

    REQUIRE(result);

    auto test_error = dll::test_set(dbn, dataset.training_images, dataset.training_labels, dll::svm_predictor());
    std::cout << "test_error:" << test_error << std::endl;
    REQUIRE(test_error < 1.0);
}

TEST_CASE("conv_dbn/mnist_11", "avg_pooling") {
    typedef dll::dbn_desc<
        dll::dbn_layers<
            dll::conv_rbm_desc<1, 28, 28, 40, 14, 12, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::conv_rbm_desc<40, 14, 12, 40, 8, 10, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::avgp_layer_3d_desc<40, 8, 10, 2, 2, 1>::layer_t>>::dbn_t dbn_t;

    REQUIRE(dbn_t::output_size() == 800);

    auto dataset = mnist::read_dataset_direct<std::vector, etl::fast_dyn_matrix<double, 1, 28, 28>>(200);

    REQUIRE(!dataset.training_images.empty());

    mnist::binarize_dataset(dataset);

    auto dbn = std::make_unique<dbn_t>();

    dbn->pretrain(dataset.training_images, 20);

    auto output = dbn->activation_probabilities(dataset.training_images.front());

    REQUIRE(output.size() == 800);

    auto result = dbn->svm_train(dataset.training_images, dataset.training_labels);

    REQUIRE(result);

    auto test_error = dll::test_set(dbn, dataset.training_images, dataset.training_labels, dll::svm_predictor());
    std::cout << "test_error:" << test_error << std::endl;
    REQUIRE(test_error < 0.1);
}

TEST_CASE("conv_dbn/mnist_12", "avgp_pooling") {
    typedef dll::dbn_desc<
        dll::dbn_layers<
            dll::conv_rbm_desc<1, 28, 28, 40, 20, 21, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::avgp_layer_3d_desc<40, 20, 21, 2, 2, 3>::layer_t,
            dll::conv_rbm_desc<20, 10, 7, 40, 8, 5, dll::momentum, dll::batch_size<25>>::layer_t,
            dll::avgp_layer_3d_desc<40, 8, 5, 2, 1, 1>::layer_t>>::dbn_t dbn_t;

    REQUIRE(dbn_t::output_size() == 800);

    auto dataset = mnist::read_dataset_direct<std::vector, etl::fast_dyn_matrix<double, 1, 28, 28>>(200);

    REQUIRE(!dataset.training_images.empty());

    mnist::binarize_dataset(dataset);

    auto dbn = std::make_unique<dbn_t>();

    dbn->pretrain(dataset.training_images, 20);

    auto output = dbn->activation_probabilities(dataset.training_images.front());

    REQUIRE(output.size() == 800);

    auto result = dbn->svm_train(dataset.training_images, dataset.training_labels);

    REQUIRE(result);

    auto test_error = dll::test_set(dbn, dataset.training_images, dataset.training_labels, dll::svm_predictor());
    std::cout << "test_error:" << test_error << std::endl;
    REQUIRE(test_error < 1.0);
}

TEST_CASE("conv_dbn/mnist_13", "nop_layers") {
    typedef dll::dbn_desc<
        dll::dbn_layers<
            dll::conv_rbm_desc<1, 28, 28, 40, 14, 12, dll::momentum, dll::batch_size<25>>::layer_t, dll::conv_rbm_desc<40, 14, 12, 40, 8, 10, dll::momentum, dll::batch_size<25>>::layer_t, dll::mp_layer_3d_desc<40, 8, 10, 1, 1, 1>::layer_t, dll::avgp_layer_3d_desc<40, 8, 10, 1, 1, 1>::layer_t>>::dbn_t dbn_t;

    REQUIRE(dbn_t::output_size() == 3200);

    auto dataset = mnist::read_dataset_direct<std::vector, etl::fast_dyn_matrix<double, 1, 28, 28>>(200);

    REQUIRE(!dataset.training_images.empty());

    mnist::binarize_dataset(dataset);

    auto dbn = std::make_unique<dbn_t>();

    dbn->pretrain(dataset.training_images, 2);

    auto output = dbn->activation_probabilities(dataset.training_images.front());

    REQUIRE(output.size() == 3200);

    auto result = dbn->svm_train(dataset.training_images, dataset.training_labels);

    REQUIRE(result);

    auto test_error = dll::test_set(dbn, dataset.training_images, dataset.training_labels, dll::svm_predictor());
    std::cout << "test_error:" << test_error << std::endl;
    REQUIRE(test_error < 0.9);
}
