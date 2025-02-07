/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef ENGINES_HQPS_ENGINE_OPERATOR_GROUP_H_
#define ENGINES_HQPS_ENGINE_OPERATOR_GROUP_H_

#include <tuple>
#include <unordered_map>
#include <vector>

#include "flex/engines/hqps_db/core/context.h"
#include "flex/engines/hqps_db/core/params.h"
#include "flex/engines/hqps_db/core/utils/keyed.h"
#include "flex/engines/hqps_db/structures/collection.h"

namespace gs {

// For each aggreator, return the type of applying aggregate on the desired col.
// with possible aggregate func.

template <typename CTX_T, typename GROUP_KEY>
struct CommonBuilderT;

template <typename CTX_T, int col_id>
struct CommonBuilderT<CTX_T, GroupKey<col_id, grape::EmptyType>> {
  using set_t = std::remove_const_t<std::remove_reference_t<decltype(
      std::declval<CTX_T>().template GetNode<col_id>())>>;
  using builder_t = typename set_t::builder_t;
  using result_t = typename builder_t::result_t;
};

template <typename CTX_T, typename GROUP_KEY>
struct GroupKeyResT;

template <typename CTX_T, int col_id, typename T>
struct GroupKeyResT<CTX_T, GroupKey<col_id, T>> {
  using set_t = std::remove_const_t<std::remove_reference_t<decltype(
      std::declval<CTX_T>().template GetNode<col_id>())>>;
  using result_t = typename KeyedT<set_t, PropertySelector<T>>::keyed_set_t;
};

template <typename CTX_T, typename AGG_T>
struct GroupValueResT;

template <typename SET_T, AggFunc agg_func, typename SELECTOR_TUPLE>
struct GroupValueResTImpl;

template <typename CTX_T, AggFunc agg_func, typename... SELECTOR, int Is>
struct GroupValueResT<CTX_T, AggregateProp<agg_func, std::tuple<SELECTOR...>,
                                           std::integer_sequence<int, Is>>> {
  using old_set_t = std::remove_const_t<std::remove_reference_t<decltype(
      std::declval<CTX_T>().template GetNode<Is>())>>;
  using result_t =
      typename GroupValueResTImpl<old_set_t, agg_func,
                                  std::tuple<SELECTOR...>>::result_t;
};

// specialization for count for single tag
// TODO: count for pairs.
template <typename SET_T>
struct GroupValueResTImpl<SET_T, AggFunc::COUNT,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  using result_t = Collection<size_t>;
};

template <typename SET_T>
struct GroupValueResTImpl<SET_T, AggFunc::COUNT_DISTINCT,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  using result_t = Collection<size_t>;
};

template <typename T>
struct GroupValueResTImpl<Collection<T>, AggFunc::SUM,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  using result_t = Collection<T>;
};

// specialization for to_set
// TODO: to set for pairs.
template <typename T>
struct GroupValueResTImpl<Collection<T>, AggFunc::TO_SET,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  using result_t = Collection<std::vector<T>>;
};

template <typename LabelT, typename VID_T, typename... SET_T, typename PropT>
struct GroupValueResTImpl<RowVertexSet<LabelT, VID_T, SET_T...>,
                          AggFunc::TO_SET,
                          std::tuple<PropertySelector<PropT>>> {
  using result_t = Collection<std::vector<PropT>>;
};

// specialization for to_list
// TODO: to set for pairs.
template <typename T>
struct GroupValueResTImpl<Collection<T>, AggFunc::TO_LIST,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  using result_t = Collection<std::vector<T>>;
};

// get the vertex's certain properties as list
template <typename LabelT, typename VID_T, typename... SET_T, typename PropT>
struct GroupValueResTImpl<RowVertexSet<LabelT, VID_T, SET_T...>,
                          AggFunc::TO_LIST,
                          std::tuple<PropertySelector<PropT>>> {
  // using old_set_t = std::remove_const_t<std::remove_reference_t<decltype(
  //     std::declval<CTX_T>().template GetNode<Is>())>>;
  using result_t = Collection<std::vector<PropT>>;
};

// get min value
template <typename T>
struct GroupValueResTImpl<Collection<T>, AggFunc::MIN,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  using result_t = Collection<T>;
};

// support get max of vertexset's id
template <typename LabelT, typename VID_T, typename... SET_T, typename T>
struct GroupValueResTImpl<RowVertexSet<LabelT, VID_T, SET_T...>, AggFunc::MAX,
                          std::tuple<PropertySelector<T>>> {
  using result_t = Collection<T>;
};

// support get first from vertexset
template <typename LabelT, typename VID_T, typename... SET_T, typename T>
struct GroupValueResTImpl<RowVertexSet<LabelT, VID_T, SET_T...>, AggFunc::FIRST,
                          std::tuple<PropertySelector<T>>> {
  // the old_set_t is vertex_set or collection
  using result_t =
      typename AggFirst<RowVertexSet<LabelT, VID_T, SET_T...>>::result_t;
};

// support get first from two label vertex set
template <typename VID_T, typename LabelT, typename... SET_T>
struct GroupValueResTImpl<TwoLabelVertexSet<VID_T, LabelT, SET_T...>,
                          AggFunc::FIRST,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  // the old_set_t is vertex_set or collection
  using result_t =
      typename AggFirst<TwoLabelVertexSet<VID_T, LabelT, SET_T...>>::result_t;
};

// get first from collection
template <typename T>
struct GroupValueResTImpl<Collection<T>, AggFunc::FIRST,
                          std::tuple<PropertySelector<grape::EmptyType>>> {
  // the old_set_t is vertex_set or collection
  using result_t = typename AggFirst<Collection<T>>::result_t;
};

template <typename Head, int new_head_tag, int base_tag, typename PREV>
struct UnWrapTuple;

template <typename Head, int new_head_tag, int base_tag, typename... T>
struct UnWrapTuple<Head, new_head_tag, base_tag, std::tuple<T...>> {
  using context_t = Context<Head, new_head_tag, base_tag, T...>;
};

template <int new_head_tag, int base_tag, typename... Nodes>
struct Rearrange {
  using head_t =
      std::tuple_element_t<sizeof...(Nodes) - 1, std::tuple<Nodes...>>;
  using prev_t =
      typename first_n<sizeof...(Nodes) - 1, std::tuple<Nodes...>>::type;
  using context_t =
      typename UnWrapTuple<head_t, new_head_tag, base_tag, prev_t>::context_t;
};

// only two nodees
// template <int new_head_tag, int base_tag, typename First, typename Node>
// struct Rearrange<new_head_tag, base_tag, First, Node> {
//   using context_t = Context<Node, new_head_tag, base_tag, First>;
// };

// only one nodes
template <int new_head_tag, int base_tag, typename First>
struct Rearrange<new_head_tag, base_tag, First> {
  using context_t = Context<First, new_head_tag, base_tag, grape::EmptyType>;
};

template <typename CTX_T, typename GROUP_KEYs, typename AGG_FUNCs>
struct GroupResT;

// We will return a brand new context.

// after groupby, we will get a brand new context, and the tag_ids will start
// from 0.
template <typename CTX_T, typename GROUP_KEY, typename... AGG_T>
struct GroupResT<CTX_T, std::tuple<GROUP_KEY>, std::tuple<AGG_T...>> {
  static constexpr int new_cur_alias = +sizeof...(AGG_T);
  // result ctx type
  using result_t = typename Rearrange<
      new_cur_alias, 0, typename GroupKeyResT<CTX_T, GROUP_KEY>::result_t,
      typename GroupValueResT<CTX_T, AGG_T>::result_t...>::context_t;
};

// keyed by two sets
template <typename CTX_T, typename... GROUP_KEY, typename... AGG_T>
struct GroupResT<CTX_T, std::tuple<GROUP_KEY...>, std::tuple<AGG_T...>> {
  static constexpr int new_cur_alias =
      sizeof...(GROUP_KEY) + sizeof...(AGG_T) - 1;
  // result ctx type
  using result_t = typename Rearrange<
      new_cur_alias, 0, typename CommonBuilderT<CTX_T, GROUP_KEY>::result_t...,
      typename GroupValueResT<CTX_T, AGG_T>::result_t...>::context_t;
};

template <typename CTX_T, typename GROUP_OPT>
struct FoldResT;

// We will return a brand new context.
template <typename CTX_T, typename... AGG_T>
struct FoldResT<CTX_T, std::tuple<AGG_T...>> {
  // take the largest alias in current context as base_tag.
  static constexpr int base_tag = CTX_T::max_tag_id + 1;
  static constexpr int new_head_tag = base_tag + sizeof...(AGG_T) - 1;

  // result ctx type
  using result_t = typename Rearrange<
      new_head_tag, base_tag,
      typename GroupValueResT<CTX_T, AGG_T>::result_t...>::context_t;
};

template <typename GRAPH_INTERFACE>
class GroupByOp {
  using label_id_t = typename GRAPH_INTERFACE::label_id_t;
  using vertex_id_t = typename GRAPH_INTERFACE::vertex_id_t;
  using vertex_set_t = DefaultRowVertexSet<label_id_t, vertex_id_t>;

 public:
  template <typename CTX_HEAD_T, int cur_alias, int base_tag,
            typename... CTX_PREV, typename FOLD_OPT,
            typename RES_T = typename FoldResT<
                Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>,
                std::tuple<FOLD_OPT>>::result_t>
  static RES_T GroupByWithoutKeyImpl(
      const GRAPH_INTERFACE& graph,
      Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>&& ctx,
      std::tuple<FOLD_OPT>&& group_opt) {
    VLOG(10) << "new result_t, base tag: " << RES_T::base_tag_id;
    // Currently we only support to to_count;
    using agg_tuple_t = std::tuple<FOLD_OPT>;
    using CTX_T = Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>;
    static constexpr size_t agg_num = std::tuple_size_v<agg_tuple_t>;
    static constexpr size_t grouped_value_num = std::tuple_size_v<agg_tuple_t>;
    // the result context must be one-to-one mapping.

    if (ctx.get_sub_task_start_tag() == INVALID_TAG) {
      LOG(FATAL) << "Not implemented now";
    }

    int start_tag = ctx.get_sub_task_start_tag();
    VLOG(10) << "start tag: " << start_tag;
    auto& agg_tuple = group_opt;

    auto value_set_builder_tuple = create_keyed_value_set_builder_tuple(
        graph, ctx.GetPrevCols(), ctx.GetHead(), agg_tuple,
        std::make_index_sequence<grouped_value_num>());
    VLOG(10) << "Create value set builders";

    for (auto iter : ctx) {
      auto ele_tuple = iter.GetAllIndexElement();
      auto data_tuple = iter.GetAllData();
      auto start_tag_ind = iter.GetTagOffset(start_tag);
      // indicate at which index the start_tag element is in.
      auto key = start_tag_ind;

      insert_to_value_set_builder(value_set_builder_tuple, ele_tuple,
                                  data_tuple, start_tag_ind);
    }
    auto value_set_built =
        build_value_set_tuple(std::move(value_set_builder_tuple),
                              std::make_index_sequence<grouped_value_num>());
    return RES_T(std::move(std::get<0>(value_set_built)),
                 ctx.get_sub_task_start_tag());

    // // create offset array with one-one mapping.
    // if (grouped_value_num == 1) {
    // } else {
    //   auto offset_vec = make_offset_vector(
    //       grouped_value_num - 1, std::get<0>(value_set_built).size() + 1);
    //   VLOG(10) << "after group by, the set size: " << keyed_set_built.Size();
    //   VLOG(10) << "offset vec: " << offset_vec.size();
    //   VLOG(10) << "," << offset_vec[0].size();

    //   RES_T res(std::move(std::get<grouped_value_num - 1>(value_set_built)),
    //             std::move(gs::tuple_slice<0, grouped_value_num - 1>(
    //                 std::move(value_set_built))),
    //             std::move(offset_vec));
    //   return res;
    // }
  }

  // group by only one key_alias
  template <typename CTX_HEAD_T, int cur_alias, int base_tag,
            typename... CTX_PREV, typename GROUP_KEY, typename... AGG_T,
            typename RES_T = typename GroupResT<
                Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>,
                std::tuple<GROUP_KEY>, std::tuple<AGG_T...>>::result_t>
  static RES_T GroupByImpl(
      const GRAPH_INTERFACE& graph,
      Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>&& ctx,
      std::tuple<GROUP_KEY>&& group_keys, std::tuple<AGG_T...>&& agg_tuple) {
    VLOG(10) << "new result_t, base tag: " << RES_T::base_tag_id;
    // Currently we only support to to_count;
    using agg_tuple_t = std::tuple<AGG_T...>;
    using key_alias_t = typename GROUP_KEY::selector_t;
    using CTX_T = Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>;
    static constexpr size_t grouped_value_num = std::tuple_size_v<agg_tuple_t>;
    static constexpr int keyed_tag_id = GROUP_KEY::col_id;
    // the result context must be one-to-one mapping.

    auto& old_key_set = gs::Get<keyed_tag_id>(ctx);
    using old_key_set_t = typename std::remove_const_t<
        std::remove_reference_t<decltype(old_key_set)>>;
    using keyed_set_builder_t =
        typename KeyedT<old_key_set_t, key_alias_t>::builder_t;
    auto keyed_set_size = old_key_set.Size();

    // create a keyed set from the old key set.
    keyed_set_builder_t keyed_set_builder(old_key_set);
    // VLOG(10) << "Create keyed set builder";
    auto value_set_builder_tuple = create_keyed_value_set_builder_tuple(
        graph, ctx.GetPrevCols(), ctx.GetHead(), agg_tuple,
        std::make_index_sequence<grouped_value_num>());

    // if group_key use property, we need property getter
    // else we just insert into key_set
    if constexpr (group_key_on_property<key_alias_t>::value) {
      auto named_property = create_prop_desc_from_selector<GROUP_KEY::col_id>(
          std::get<0>(group_keys).selector_);
      auto prop_getter =
          create_prop_getter_from_prop_desc(graph, ctx, named_property);
      for (auto iter : ctx) {
        auto ele_tuple = iter.GetAllIndexElement();
        auto data_tuple = iter.GetAllData();

        auto key_ele = gs::get_from_tuple<GROUP_KEY::col_id>(ele_tuple);
        auto data_ele = gs::get_from_tuple<GROUP_KEY::col_id>(data_tuple);
        size_t ind = insert_to_keyed_set_with_prop_getter(keyed_set_builder,
                                                          prop_getter, key_ele);

        insert_to_value_set_builder(value_set_builder_tuple, ele_tuple,
                                    data_tuple, ind);
      }
    } else {
      for (auto iter : ctx) {
        auto ele_tuple = iter.GetAllIndexElement();
        auto data_tuple = iter.GetAllData();

        auto key_ele = gs::get_from_tuple<key_alias_t::tag_id>(ele_tuple);
        auto data_ele = gs::get_from_tuple<key_alias_t::tag_id>(data_tuple);
        size_t ind = insert_to_keyed_set(keyed_set_builder, key_ele, data_ele);
        insert_to_value_set_builder(value_set_builder_tuple, ele_tuple,
                                    data_tuple, ind);
      }
    }

    auto keyed_set_built = keyed_set_builder.Build();

    auto value_set_built =
        build_value_set_tuple(std::move(value_set_builder_tuple),
                              std::make_index_sequence<grouped_value_num>());

    // create offset array with one-one mapping.
    auto offset_vec =
        make_offset_vector(grouped_value_num, keyed_set_built.Size());

    auto new_tuple = std::tuple_cat(std::move(std::make_tuple(keyed_set_built)),
                                    std::move(value_set_built));

    RES_T res(
        std::move(std::get<grouped_value_num>(new_tuple)),
        std::move(gs::tuple_slice<0, grouped_value_num>(std::move(new_tuple))),
        std::move(offset_vec));

    return res;
  }

  // group by two key_alias,
  template <
      typename CTX_HEAD_T, int cur_alias, int base_tag, typename... CTX_PREV,
      typename KEY_ALIAS0, typename KEY_ALIAS1, typename... AGG,
      typename RES_T = typename GroupResT<
          Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>,
          std::tuple<KEY_ALIAS0, KEY_ALIAS1>, std::tuple<AGG...>>::result_t>
  static RES_T GroupByImpl(
      const GRAPH_INTERFACE& graph,
      Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>&& ctx,
      std::tuple<KEY_ALIAS0, KEY_ALIAS1> group_keys,
      std::tuple<AGG...>&& aggs) {
    VLOG(10) << "new result_t, base tag: " << RES_T::base_tag_id;
    // Currently we only support to to_count;
    using agg_tuple_t = std::tuple<AGG...>;
    using key_alias0_t = KEY_ALIAS0;
    using key_alias1_t = KEY_ALIAS1;
    // we assume key_alias's tag are sequential.

    using CTX_T = Context<CTX_HEAD_T, cur_alias, base_tag, CTX_PREV...>;
    static constexpr size_t grouped_value_num = std::tuple_size_v<agg_tuple_t>;
    static constexpr int keyed_tag_id0 = key_alias0_t::col_id;
    static constexpr int keyed_tag_id1 = key_alias1_t::col_id;

    // the result context must be one-to-one mapping.

    auto& old_key_set0 = gs::Get<keyed_tag_id0>(ctx);
    auto& old_key_set1 = gs::Get<keyed_tag_id1>(ctx);
    using old_key_set_t0 = typename std::remove_const_t<
        std::remove_reference_t<decltype(old_key_set0)>>;
    using old_key_set_t1 = typename std::remove_const_t<
        std::remove_reference_t<decltype(old_key_set1)>>;
    using old_key_set_iter_t0 = typename old_key_set_t0::iterator;
    using old_key_set_iter_t1 = typename old_key_set_t1::iterator;
    using old_key_set_ele0_t = std::remove_reference_t<decltype(
        std::declval<old_key_set_iter_t0>().GetIndexElement())>;
    using old_key_set_ele1_t = std::remove_reference_t<decltype(
        std::declval<old_key_set_iter_t1>().GetIndexElement())>;

    // when grouping key is two key_alias, we use just set builder, not keyed
    // builder.

    auto& key_alias_opt0 = std::get<0>(group_keys);
    auto& key_alias_opt1 = std::get<1>(group_keys);
    // create a keyed set from the old key set.
    // VLOG(10) << "Create keyed set builder";
    auto value_set_builder_tuple = create_keyed_value_set_builder_tuple(
        graph, ctx.GetPrevCols(), ctx.GetHead(), aggs,
        std::make_index_sequence<grouped_value_num>());
    VLOG(10) << "Create value set builders";

    if constexpr (!group_key_on_property<key_alias0_t>::value &&
                  !group_key_on_property<key_alias1_t>::value) {
      // NOTE: here when we create keyed set builder, we don't require it as a
      // keyed builder,i.e. it doesn't need to deduplicate.
      auto keyed_set_builder0 = old_key_set0.CreateBuilder();
      auto keyed_set_builder1 = old_key_set1.CreateBuilder();
      using con_key_ele_t = std::pair<old_key_set_ele0_t, old_key_set_ele1_t>;
      std::unordered_map<con_key_ele_t, int, boost::hash<con_key_ele_t>>
          key_tuple_set;
      size_t cur_ind = 0;
      for (auto iter : ctx) {
        auto ele_tuple = iter.GetAllIndexElement();
        auto data_tuple = iter.GetAllData();

        auto key_ele0 = gs::get_from_tuple<keyed_tag_id0>(ele_tuple);
        auto key_ele1 = gs::get_from_tuple<keyed_tag_id1>(ele_tuple);
        auto tmp_ele = std::make_pair(key_ele0, key_ele1);

        auto data_ele0 = gs::get_from_tuple<keyed_tag_id0>(data_tuple);
        auto data_ele1 = gs::get_from_tuple<keyed_tag_id1>(data_tuple);
        size_t ind = 0;
        if (key_tuple_set.find(tmp_ele) != key_tuple_set.end()) {
          // already exist
          auto ind = key_tuple_set[tmp_ele];
        } else {
          // not exist
          ind = cur_ind++;
          insert_into_builder_v2_impl(keyed_set_builder0, key_ele0, data_ele0);
          insert_into_builder_v2_impl(keyed_set_builder1, key_ele1, data_ele1);
          key_tuple_set[tmp_ele] = ind;
        }
        // CHECK insert key.
        insert_to_value_set_builder(value_set_builder_tuple, ele_tuple,
                                    data_tuple, ind);
      }

      auto keyed_set_built0 = keyed_set_builder0.Build();
      auto keyed_set_built1 = keyed_set_builder1.Build();
      CHECK(keyed_set_built0.Size() == keyed_set_built1.Size())
          << "size ueq: " << keyed_set_built0.Size() << " "
          << keyed_set_built1.Size();

      auto value_set_built =
          build_value_set_tuple(std::move(value_set_builder_tuple),
                                std::make_index_sequence<grouped_value_num>());
      // create offset array with one-one mapping.
      auto offset_vec =
          make_offset_vector(grouped_value_num + 1, keyed_set_built0.Size());

      auto new_tuple =
          std::tuple_cat(std::move(std::make_tuple(keyed_set_built0)),
                         std::move(std::make_tuple(keyed_set_built1)),
                         std::move(value_set_built));
      RES_T res(std::move(std::get<grouped_value_num + 1>(new_tuple)),
                std::move(gs::tuple_slice<0, grouped_value_num + 1>(
                    std::move(new_tuple))),
                std::move(offset_vec));

      return res;
    } else {
      static_assert("Not implemented");
    }
  }

  // ind is the index of the key in the key set
  template <size_t Is = 0, typename ele_tuple_t, typename data_tuple_t,
            typename... SET_T>
  static void insert_to_value_set_builder(
      std::tuple<SET_T...>& value_set_builder, const ele_tuple_t& ele_tuple,
      const data_tuple_t& data_tuple, size_t ind) {
    std::get<Is>(value_set_builder).insert(ind, ele_tuple, data_tuple);
    if constexpr (Is + 1 < sizeof...(SET_T)) {
      insert_to_value_set_builder<Is + 1>(value_set_builder, ele_tuple,
                                          data_tuple, ind);
    }
  }

  template <typename... BUILDER_T, size_t... Is>
  static auto build_value_set_tuple(std::tuple<BUILDER_T...>&& builder_tuple,
                                    std::index_sequence<Is...>) {
    return std::make_tuple(std::get<Is>(builder_tuple).Build()...);
  }

  template <typename... SET_T, typename HEAD_T, typename... AGG_T, size_t... Is>
  static auto create_keyed_value_set_builder_tuple(
      const GRAPH_INTERFACE& graph, const std::tuple<SET_T...>& prev,
      const HEAD_T& head, std::tuple<AGG_T...>& agg_tuple,
      std::index_sequence<Is...>) {
    return std::make_tuple(create_keyed_value_set_builder(
        graph, prev, head, std::get<Is>(agg_tuple))...);
  }

  template <typename... SET_T, typename HEAD_T, AggFunc _agg_func, typename T,
            int tag_id>
  static auto create_keyed_value_set_builder(
      const GRAPH_INTERFACE& graph, const std::tuple<SET_T...>& tuple,
      const HEAD_T& head,
      AggregateProp<_agg_func, std::tuple<PropertySelector<T>>,
                    std::integer_sequence<int32_t, tag_id>>& agg) {
    if constexpr (tag_id < sizeof...(SET_T)) {
      auto old_set = gs::get_from_tuple<tag_id>(tuple);
      using old_set_t = typename std::remove_const_t<
          std::remove_reference_t<decltype(old_set)>>;

      return KeyedAggT<GRAPH_INTERFACE, old_set_t, _agg_func, std::tuple<T>,
                       std::integer_sequence<int32_t, tag_id>>::
          create_agg_builder(old_set, graph, agg.selectors_);
    } else {
      return KeyedAggT<GRAPH_INTERFACE, HEAD_T, _agg_func, std::tuple<T>,
                       std::integer_sequence<int32_t, tag_id>>::
          create_agg_builder(head, graph, agg.selectors_);
    }
  }

  // insert_to_key_set with respect to property type
  template <typename BuilderT, typename PROP_GETTER, typename ELE>
  static inline auto insert_to_keyed_set_with_prop_getter(
      BuilderT& builder, const PROP_GETTER& prop_getter, const ELE& ele) {
    return builder.insert(prop_getter.get_view(ele));
  }

  // insert_into_bulder_v2_impl
  template <typename BuilderT, typename ELE, typename DATA,
            typename std::enable_if<std::is_same_v<
                DATA, std::tuple<grape::EmptyType>>>::type* = nullptr>
  static inline auto insert_to_keyed_set(BuilderT& builder, const ELE& ele,
                                         const DATA& data) {
    return builder.insert(ele);
  }

  // insert_into_bulder_v2_impl
  template <typename BuilderT, typename ELE, typename DATA,
            typename std::enable_if<!std::is_same_v<
                DATA, std::tuple<grape::EmptyType>>>::type* = nullptr>
  static inline auto insert_to_keyed_set(BuilderT& builder, const ELE& ele,
                                         const DATA& data) {
    return builder.insert(ele, data);
  }
};
}  // namespace gs

#endif  // ENGINES_HQPS_ENGINE_OPERATOR_GROUP_H_
