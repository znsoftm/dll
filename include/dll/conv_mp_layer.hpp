//=======================================================================
// Copyright (c) 2014 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef DBN_CONV_MP_LAYER_HPP
#define DBN_CONV_MP_LAYER_HPP

#include "base_conf.hpp"
#include "contrastive_divergence.hpp"
#include "tmp.hpp"

namespace dll {

template<std::size_t NV_T, std::size_t NH_T, std::size_t K_T, std::size_t C_T, typename... Parameters>
struct conv_mp_layer {
    static constexpr const std::size_t NV = NV_T;
    static constexpr const std::size_t NH = NH_T;
    static constexpr const std::size_t K = K_T;
    static constexpr const std::size_t C = C_T;

    static_assert(NV > 0, "A matrix of at least 1x1 is necessary for the visible units");
    static_assert(NH > 0, "A matrix of at least 1x1 is necessary for the hidden units");
    static_assert(K > 0, "At least one base is necessary");
    static_assert(C > 0, "At least one pooling group is necessary");

    //Make sure only valid types are passed to the configuration list
    static_assert(
        is_valid<tmp_list<
                momentum, batch_size_id, visible_unit_id, hidden_unit_id, pooling_unit_id,
                weight_decay_id>
            , Parameters...>::value,
        "Invalid parameters type");

    static constexpr const bool Momentum = is_present<momentum, Parameters...>::value;
    static constexpr const std::size_t BatchSize = get_value<batch_size<1>, Parameters...>::value;
    static constexpr const Type VisibleUnit = get_value<visible_unit<Type::BINARY>, Parameters...>::value;
    static constexpr const Type HiddenUnit = get_value<hidden_unit<Type::BINARY>, Parameters...>::value;
    static constexpr const Type PoolingUnit = get_value<pooling_unit<Type::BINARY>, Parameters...>::value;
    static constexpr const DecayType Decay = get_value<weight_decay<DecayType::NONE>, Parameters...>::value;

    template <typename RBM>
    using trainer_t = typename get_template_type<trainer<cd1_trainer_t>, Parameters...>::template type<RBM>;

    static_assert(BatchSize > 0, "Batch size must be at least 1");
};

} //end of dbn namespace

#endif