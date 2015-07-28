//=======================================================================
// Copyright (c) 2014-2015 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef DLL_STANDARD_CONV_RBM_HPP
#define DLL_STANDARD_CONV_RBM_HPP

#include "base_conf.hpp"          //The configuration helpers
#include "rbm_base.hpp"           //The base class
#include "layer_traits.hpp"
#include "checks.hpp"

namespace dll {

/*!
 * \brief Standard version of Convolutional Restricted Boltzmann Machine
 *
 * This follows the definition of a CRBM by Honglak Lee.
 */
template<typename Parent, typename Desc>
struct standard_conv_rbm : public rbm_base<Parent, Desc> {
    using desc = Desc;
    using parent_t = Parent;
    using this_type = standard_conv_rbm<parent_t, desc>;
    using base_type = rbm_base<parent_t, Desc>;
    using weight = typename desc::weight;

    static constexpr const unit_type visible_unit = desc::visible_unit;
    static constexpr const unit_type hidden_unit = desc::hidden_unit;

    static_assert(visible_unit == unit_type::BINARY || visible_unit == unit_type::GAUSSIAN,
        "Only binary and linear visible units are supported");
    static_assert(hidden_unit == unit_type::BINARY || is_relu(hidden_unit),
        "Only binary hidden units are supported");

    double std_gaussian = 0.2;
    double c_sigm = 1.0;

    //Constructors

    standard_conv_rbm(){
        //Note: Convolutional RBM needs lower learning rate than standard RBM

        //Better initialization of learning rate
        base_type::learning_rate =
                visible_unit == unit_type::GAUSSIAN  ?             1e-5
            :   is_relu(hidden_unit)                 ?             1e-4
            :   /* Only Gaussian Units needs lower rate */         1e-3;
    }

    //Utility functions

    template<typename Sample>
    void reconstruct(const Sample& items){
        reconstruct(items, *static_cast<parent_t*>(this));
    }

    void display_visible_unit_activations() const {
        display_visible_unit_activations(*static_cast<parent_t*>(this));
    }

    void display_visible_unit_samples() const {
        display_visible_unit_samples(*static_cast<parent_t*>(this));
    }

    void display_hidden_unit_activations() const {
        display_hidden_unit_samples(*static_cast<parent_t*>(this));
    }

    void display_hidden_unit_samples() const {
        display_hidden_unit_samples(*static_cast<parent_t*>(this));
    }

protected:

    template<typename W>
    static void deep_fflip(W&& w_f){
        //flip all the kernels horizontally and vertically

        for(std::size_t channel = 0; channel < etl::dim<0>(w_f); ++channel){
            for(size_t k = 0; k < etl::dim<1>(w_f); ++k){
                w_f(channel)(k).fflip_inplace();
            }
        }
    }

    template<typename L, typename V1, typename VCV, typename W>
    static void compute_vcv(const V1& v_a, VCV&& v_cv, W&& w){
        static constexpr const auto NC = L::NC;

        auto w_f = etl::force_temporary(w);

        deep_fflip(w_f);

        v_cv(1) = 0;

        for(std::size_t channel = 0; channel < NC; ++channel){
            etl::conv_2d_valid_multi(v_a(channel), w_f(channel), v_cv(0));

            v_cv(1) += v_cv(0);
        }

        nan_check_deep(v_cv);
    }

    template<typename L, typename H2, typename HCV, typename W, typename Functor>
    static void compute_hcv(const H2& h_s, HCV&& h_cv, W&& w, Functor activate){
        static constexpr const auto K = L::K;
        static constexpr const auto NC = L::NC;

        for(std::size_t channel = 0; channel < NC; ++channel){
            h_cv(1) = 0.0;

            for(std::size_t k = 0; k < K; ++k){
                h_cv(0) = etl::fast_conv_2d_full(h_s(k), w(channel)(k));
                h_cv(1) += h_cv(0);
            }

            activate(channel);
        }
    }

#ifdef ETL_MKL_MODE

    template<typename F1, typename F2>
    static void deep_pad(const F1& in, F2& out){
        for(std::size_t outer1 = 0; outer1 < in.template dim<0>(); ++outer1){
            for(std::size_t outer2 = 0; outer2 < in.template dim<1>(); ++outer2){
                auto* direct = out(outer1)(outer2).memory_start();
                for(std::size_t i = 0; i < in.template dim<2>(); ++i){
                    for(std::size_t j = 0; j < in.template dim<3>(); ++j){
                        direct[i * out.template dim<3>() + j] = in(outer1,outer2,i,j);
                    }
                }
            }
        }
    }

    static void inplace_fft2(std::complex<double>* memory, std::size_t m1, std::size_t m2){
        etl::impl::blas::detail::inplace_zfft2_kernel(memory, m1, m2);
    }

    static void inplace_fft2(std::complex<float>* memory, std::size_t m1, std::size_t m2){
        etl::impl::blas::detail::inplace_cfft2_kernel(memory, m1, m2);
    }

    static void inplace_ifft2(std::complex<double>* memory, std::size_t m1, std::size_t m2){
        etl::impl::blas::detail::inplace_zifft2_kernel(memory, m1, m2);
    }

    static void inplace_ifft2(std::complex<float>* memory, std::size_t m1, std::size_t m2){
        etl::impl::blas::detail::inplace_cifft2_kernel(memory, m1, m2);
    }

    template<typename L, typename TP, typename H2, typename HCV, typename W, typename Functor>
    static void batch_compute_hcv(TP& pool, const H2& h_s, HCV&& h_cv, W&& w, Functor activate){
        static constexpr const auto Batch = layer_traits<L>::batch_size();

        static constexpr const auto K = L::K;
        static constexpr const auto NC = L::NC;
        static constexpr const auto NV1 = L::NV1;
        static constexpr const auto NV2 = L::NV2;

        etl::fast_dyn_matrix<std::complex<weight>, Batch, K, NV1, NV2> h_s_padded;
        etl::fast_dyn_matrix<std::complex<weight>, NC, K, NV1, NV2> w_padded;
        etl::fast_dyn_matrix<std::complex<weight>, Batch, NV1, NV2> tmp_result;

        deep_pad(h_s, h_s_padded);
        deep_pad(w, w_padded);

        for(std::size_t batch = 0; batch < Batch; ++batch){
            for(std::size_t k = 0; k < K; ++k){
                inplace_fft2(h_s_padded(batch)(k).memory_start(), NV1, NV2);
            }
        }

        for(std::size_t channel = 0; channel < NC; ++channel){
            for(std::size_t k = 0; k < K; ++k){
                inplace_fft2(w_padded(channel)(k).memory_start(), NV1, NV2);
            }
        }

        maybe_parallel_foreach_n(pool, 0, Batch, [&](std::size_t batch){
            for(std::size_t channel = 0; channel < NC; ++channel){
                h_cv(batch)(1) = 0.0;

                for(std::size_t k = 0; k < K; ++k){
                    tmp_result(batch) = h_s_padded(batch)(k) >> w_padded(channel)(k);

                    inplace_ifft2(tmp_result(batch).memory_start(), NV1, NV2);

                    for(std::size_t i = 0; i < etl::size(tmp_result(batch)); ++i){
                        h_cv(batch)(1)[i] += tmp_result(batch)[i].real();
                    }
                }

                activate(batch, channel);
            }
        });
    }

#else

    template<typename L, typename TP, typename H2, typename HCV, typename W, typename Functor>
    static void batch_compute_hcv(TP& pool, const H2& h_s, HCV&& h_cv, W&& w, Functor activate){
        static constexpr const auto Batch = layer_traits<L>::batch_size();

        static constexpr const auto K = L::K;
        static constexpr const auto NC = L::NC;

        maybe_parallel_foreach_n(pool, 0, Batch, [&](std::size_t batch){
            for(std::size_t channel = 0; channel < NC; ++channel){
                h_cv(batch)(1) = 0.0;

                for(std::size_t k = 0; k < K; ++k){
                    h_cv(batch)(0) = etl::fast_conv_2d_full(h_s(batch)(k), w(channel)(k));
                    h_cv(batch)(1) += h_cv(batch)(0);
                }

                activate(batch, channel);
            }
        });
    }

#endif

    template<typename L, typename TP, typename V1, typename VCV, typename W, typename Functor>
    static void batch_compute_vcv(TP& pool, const V1& v_a, VCV&& v_cv, W&& w, Functor activate){
        static constexpr const auto Batch = layer_traits<L>::batch_size();

        static constexpr const auto NC = L::NC;

        auto w_f = etl::force_temporary(w);

        deep_fflip(w_f);

        maybe_parallel_foreach_n(pool, 0, Batch, [&](std::size_t batch){
            v_cv(batch)(1) = 0;

            for(std::size_t channel = 0; channel < NC; ++channel){
                etl::conv_2d_valid_multi(v_a(batch)(channel), w_f(channel), v_cv(batch)(0));

                v_cv(batch)(1) += v_cv(batch)(0);
            }

            activate(batch);
        });
    }

private:

    //Since the sub classes do not have the same fields, it is not possible
    //to put the fields in standard_rbm, therefore, it is necessary to use template
    //functions to implement the details

    template<typename Sample>
    static void reconstruct(const Sample& items, parent_t& rbm){
        cpp_assert(items.size() == parent_t::input_size(), "The size of the training sample must match visible units");

        cpp::stop_watch<> watch;

        //Set the state of the visible units
        rbm.v1 = items;

        rbm.activate_hidden(rbm.h1_a, rbm.h1_s, rbm.v1, rbm.v1);

        rbm.activate_visible(rbm.h1_a, rbm.h1_s, rbm.v2_a, rbm.v2_s);
        rbm.activate_hidden(rbm.h2_a, rbm.h2_s, rbm.v2_a, rbm.v2_s);

        std::cout << "Reconstruction took " << watch.elapsed() << "ms" << std::endl;
    }

    static void display_visible_unit_activations(const parent_t& rbm){
        for(std::size_t channel = 0; channel < parent_t::NC; ++channel){
            std::cout << "Channel " << channel << std::endl;

            for(size_t i = 0; i < parent_t::NV; ++i){
                for(size_t j = 0; j < parent_t::NV; ++j){
                    std::cout << rbm.v2_a(channel, i, j) << " ";
                }
                std::cout << std::endl;
            }
        }
    }

    static void display_visible_unit_samples(const parent_t& rbm){
        for(std::size_t channel = 0; channel < parent_t::NC; ++channel){
            std::cout << "Channel " << channel << std::endl;

            for(size_t i = 0; i < parent_t::NV; ++i){
                for(size_t j = 0; j < parent_t::NV; ++j){
                    std::cout << rbm.v2_s(channel, i, j) << " ";
                }
                std::cout << std::endl;
            }
        }
    }

    static void display_hidden_unit_activations(const parent_t& rbm){
        for(size_t k = 0; k < parent_t::K; ++k){
            for(size_t i = 0; i < parent_t::NV; ++i){
                for(size_t j = 0; j < parent_t::NV; ++j){
                    std::cout << rbm.h2_a(k)(i, j) << " ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl << std::endl;
        }
    }

    static void display_hidden_unit_samples(const parent_t& rbm){
        for(size_t k = 0; k < parent_t::K; ++k){
            for(size_t i = 0; i < parent_t::NV; ++i){
                for(size_t j = 0; j < parent_t::NV; ++j){
                    std::cout << rbm.h2_s(k)(i, j) << " ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl << std::endl;
        }
    }
};

} //end of dll namespace

#endif
