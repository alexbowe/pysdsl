#pragma once

#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <sdsl/vectors.hpp>
#include <sdsl/wavelet_trees.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "calc.hpp"
#include "docstrings.hpp"
#include "io.hpp"


namespace py = pybind11;



template <class T, bool /* enable */ = T::lex_ordered>
class add_lex_functor;

template <class T, bool /* enable */ = sdsl::has_node_type<T>::value>
class add_traversable_functor;


template <class T>
class add_lex_functor<T, false>
{
public:
    py::class_<T>& operator() (py::class_<T>& cls) { return cls; }
};


template <class T>
class add_lex_functor<T, true>
{
public:
    py::class_<T>& operator() (py::class_<T>& cls)
    {
        typedef typename T::size_type size_type;
        typedef typename T::value_type value_type;

        cls.def(
            "quantile_freq",
            [] (const T& self, typename T::size_type lb,
                typename T::size_type rb, typename T::size_type q) {
                return sdsl::quantile_freq(self, lb, rb, q); },
            py::arg("lb"), py::arg("rb"), py::arg("q"),
            "Returns the q-th smallest element and its frequency in wt[lb..rb]."
            "\n\tlb: Left array bound in T"
            "\n\trb: Right array bound in T"
            "\n\tq: q-th largest element ('quantile'), 0-based indexed.",
            py::call_guard<py::gil_scoped_release>());
        cls.def(
            "lex_count",
            [] (const T& self, size_t i, size_t j, typename T::value_type c) {
                if (j >= self.size()) {
                    throw std::invalid_argument("j should be less than size"); }
                if (i >= j) {
                    throw std::invalid_argument("i should be less than j"); }
                return self.lex_count(i, j, c); },
            py::arg("i"), py::arg("j"), py::arg("c"),
            "How many values are lexicographic smaller/greater than c in "
            "[i..j-1].\n\ti: Start index (inclusive) of the interval."
            "\n\tj: End index (exclusive) of the interval."
            "\n\tc: Value c.\nreturn A triple containing:\n\trank(i, c)"
            "\n\tnumber of values smaller than c in [i..j-1]"
            "\n\tnumber of values greater than c in [i..j-1]",
            py::call_guard<py::gil_scoped_release>());
        cls.def(
            "lex_smaller_count",
            [] (const T& self, size_t i, typename T::value_type c) {
                if (i >= self.size()) {
                    throw std::invalid_argument("i should be less than size"); }
                return self.lex_smaller_count(i, c); },
            py::arg("i"), py::arg("c"),
            "How many values are lexicographic smaller than c in [0..i-1]."
            "\n\ti: Exclusive right bound of the range."
            "\nreturn: A tuple containing:\n\trank(i, c)\n\tnumber of values "
            "smaller than c in [0..i-1]",
            py::call_guard<py::gil_scoped_release>());
        cls.def(
            "symbol_lte",
            [] (const T& self, typename T::value_type c) {
                auto result = sdsl::symbol_lte(self, c);
                if (!std::get<0>(result)) {
                    throw std::runtime_error("Symbol not found"); }
                return std::get<1>(result); },
            py::arg("c"),
            "Returns for a symbol c the previous smaller or equal symbol in "
            "the WT");
        cls.def(
            "symbol_gte",
            [] (const T& self, typename T::value_type c) {
                auto result = sdsl::symbol_gte(self, c);
                if (!std::get<0>(result)) {
                    throw std::runtime_error("Symbol not found"); }
                return std::get<1>(result); },
            py::arg("c"),
            "Returns for a symbol c the next larger or equal symbol in the WT");
        cls.def(
            "restricted_unique_range_values",
            [] (const T& self, size_type x_i, size_type x_j, value_type y_i,
                value_type y_j
            ) { return sdsl::restricted_unique_range_values(self, x_i, x_j,
                                                            y_i, y_j); },
            py::arg("x_i"), py::arg("x_j"), py::arg("y_i"), py::arg("y_j"),
            "For an x range [x_i, x_j] and a value range [y_i, y_j] "
            "return all unique y values occuring in [x_i, x_j] "
            "in ascending order.",
            py::call_guard<py::gil_scoped_release>());
        return cls;
    }
};


template <class T>
class add_traversable_functor<T, false>
{
public:
    py::class_<T>& operator() (py::module&, py::class_<T>& cls, std::string&&) {
        return cls; }
};


template <class T>
class add_traversable_functor<T, true>
{
public:
    py::class_<T>& operator() (py::module& m, py::class_<T>& cls,
                               std::string&& name)
    {
        typedef typename T::node_type t_node;
        typedef typename T::size_type t_size;

        try
        {
            py::class_<t_node> node_cls(m, name.c_str())
    //            .def_property_readonly("sym", &t_node::sym )
            ;
        }
        catch(std::runtime_error& /* ignore */) {}

        cls.def("root_node", &T::root);
        cls.def("node_is_leaf", &T::is_leaf);
        cls.def(
            "node_empty",
            [] (const T& self, const t_node& node)
            { return self.empty(node); });
        cls.def(
            "node_size",
            [] (const T& self, const t_node& node)
            { return self.size(node); });
        cls.def("node_sym", &T::sym);
        cls.def(
            "node_expand",
            [] (const T& self, const t_node& node)
            { return self.expand(node); });
        cls.def(
            "node_expand_ranges",
            [] (const T& self, const t_node& node,
                const sdsl::range_vec_type& ranges)
            {
                return self.expand(node, ranges);
            },
            py::arg("node"), py::arg("ranges"));
        cls.def(
            "node_bit_vec",
            [] (const T& self, const t_node& node) {
                auto bit_vec = self.bit_vec(node);
                return std::make_pair(
                    bit_vec.size(),
                    py::make_iterator(
                        detail::cbegin(bit_vec),
                        detail::cend(bit_vec))); });
        cls.def(
            "node_seq",
            [] (const T& self, const t_node& node) {
                auto seq = self.seq(node);
                sdsl::int_vector<> s(seq.size());
                std::copy(seq.begin(), seq.end(), s.begin());
                return s; } );

        cls.def(
            "intersect",
            [] (const T& self, std::vector<sdsl::range_type> ranges, size_t t) {
                return sdsl::intersect(self, ranges, t); },
            py::arg("ranges"), py::arg("t") = 0,
            "Intersection of elements in "
            "WT[s₀, e₀], WT[s₁, e₁], ...,WT[sₖ,eₖ]\n"
            "\tranges: The ranges.\n\tt: Threshold in how many distinct ranges "
            "the value has to be present. Default: t=ranges.size()\n"
            "Return a vector containing (value, frequency) - of value which "
            "are contained in t different ranges. Frequency = accumulated "
            "frequencies in all ranges. The tuples are ordered according "
            "to value, if wt is lex_ordered.");
        cls.def(
            "interval_symbols",
            [] (const T& self, t_size i, t_size j) {
                if (j > self.size()) {
                    throw std::invalid_argument("j should be less or equal "
                                                "than size"); }
                if (i > j) {
                    throw std::invalid_argument("i should be less or equal "
                                                "than j"); }
                t_size k;
                std::vector<typename T::value_type> cs(self.sigma);
                std::vector<t_size> rank_c_i(self.sigma);
                std::vector<t_size> rank_c_j(self.sigma);

                sdsl::interval_symbols(self, i, j, k, cs, rank_c_i, rank_c_j);

                return std::make_tuple(k, cs, rank_c_i, rank_c_j); },
            py::arg("i"), py::arg("j"),
            "For each symbol c in wt[i..j - 1] get rank(i, c) and rank(j, c).");
        return cls;
    }
};


template <class T>
auto add_wavelet_specific(py::class_<T>& cls) { return cls; }


template <class... T>
auto add_wavelet_specific(py::class_<sdsl::wt_int<T...>>& cls)
{
    typedef sdsl::wt_int<T...> base_cls;
    typedef typename base_cls::size_type size_type;
    typedef typename base_cls::value_type value_type;

    cls.def_property_readonly(
        "tree",
        [] (const base_cls& self) { return self.tree; },
        "A concatenation of all bit vectors of the wavelet tree.");
    cls.def(
        "get_tree",
        [] (const base_cls& self) { return self.tree; },
        "A concatenation of all bit vectors of the wavelet tree.");
    cls.def_property_readonly(
        "max_level",
        [] (const base_cls& self) { return self.max_level; },
        "Maximal level of the wavelet tree.");
    cls.def(
        "get_max_level",
        [] (const base_cls& self) { return self.max_level; },
        "Maximal level of the wavelet tree.");
    cls.def(
        "range_search_2d",
        [] (const base_cls& self, size_type lb, size_type rb,
            value_type vlb, value_type vrb, bool report=true)
        {
            return self.range_search_2d(lb, rb, vlb, vrb, report);
        },
        py::arg("lb"), py::arg("rb"), py::arg("vlb"), py::arg("vrb"),
        py::arg("report"),
        "searches points in the index interval [lb..rb] and "
        "value interval [vlb..vrb].\n"
        "\tlb: Left bound of index interval (inclusive)\n"
        "\trb: Right bound of index interval (inclusive)\n"
        "\tvlb: Left bound of value interval (inclusive)\n"
        "\tvrb: Right bound of value interval (inclusive)\n"
        "\treport: Should the matching points be returned?\n"
        "returns pair (number of found points, vector of points), "
        "the vector is empty when report = false.",
        py::call_guard<py::gil_scoped_release>());

    return cls;
}


template <class T>
inline auto add_wavelet_class(py::module& m, const std::string&& name,
                              const char* doc= nullptr)
{
    auto cls = py::class_<T>(m, name.c_str())
        .def_property_readonly(
            "sigma",
            [] (const T& self) { return self.sigma; },
            "Effective alphabet size of the wavelet tree")
        .def(
            "get_sigma",
            [] (const T& self) { return self.sigma; },
            "Effective alphabet size of the wavelet tree")
        .def_static(
            "from_bytes",
            [] (const py::bytes& bytes)
            {
                T wt;
                sdsl::construct_im(wt, std::string(bytes),
                                   sizeof(typename T::value_type));
                return wt;
            },
            py::arg("s"),
            "Construct from a build sequence",
            py::call_guard<py::gil_scoped_release>())
        .def_static(
            "from_binary_file",
            [] (const std::string& file_name) {
                T wt;
                sdsl::construct(wt, file_name, sizeof(typename T::value_type));
                return wt; },
            py::arg("file_name"),
            py::call_guard<py::gil_scoped_release>())
        .def_static(
            "parse_string",
            [] (const std::string& s)
            {
                T wt;
                sdsl::construct_im(wt, s, 'd');
                return wt;
            },
            py::arg("s"),
            "Construct from space-separated human-readable string")
        .def(
            "rank",
            [] (const T& self, typename T::size_type i,
                typename T::value_type c)
            {
                if (i >= self.size()) {
                    throw std::out_of_range(std::to_string(i)); }
                return self.rank(i, c);
            },
            "Calculates how many values c are in the prefix [0..i-1] of the "
            "supported vector (i in [0..size]).\nTime complexity: "
            "Order(log(|Sigma|))",
            py::arg("i"), py::arg("c"),
            py::call_guard<py::gil_scoped_release>())
        .def(
            "inverse_select",
            [] (const T& self, typename T::size_type i) {
                if (i >= self.size()) {
                    throw std::out_of_range(std::to_string(i)); }
                return self.inverse_select(i); },
            py::arg("i"),
            "Calculates how many occurrences of value wt[i] are in the prefix"
            "[0..i-1] of the original sequence, returns pair "
            "(rank(wt[i], i), wt[i])",
            py::call_guard<py::gil_scoped_release>())
        .def(
            "select",
            [] (const T& self, typename T::size_type i,
                typename T::value_type c)
            {
                if (i < 1 || i >= self.size()) {
                    throw std::out_of_range(std::to_string(i)); }
                if (i > self.rank(self.size(), c)) {
                    throw std::invalid_argument(
                        std::to_string(i) + " is greater than rank(" +
                        std::to_string(i) + ", " + std::to_string(c) + ")"); }
                return self.select(i, c); },
            py::arg("i"), py::arg("c"),
            "Calculates the i-th occurrence of the value c in the supported "
            "vector.\nTime complexity: Order(log(|Sigma|))",
            py::call_guard<py::gil_scoped_release>());

    add_wavelet_specific(cls);

    add_lex_functor<T>()(cls);
    add_traversable_functor<T>()(m, cls, "_" + name + "Node");

    add_sizes(cls);
    add_description(cls);
    add_serialization(cls);
    add_to_string(cls);

    add_read_access(cls);
    add_std_algo(cls);

    if (doc) cls.doc() = doc;

    m.attr("all_wavelet_trees").attr("append")(cls);

    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_int(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_int<bit_vector>>(
        m, ("WaveletTreeInt" + base_name).c_str(), doc_wtint);
    m.attr("wavelet_tree_int").attr("__setitem__")(base_name, cls);

    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wm_int(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wm_int<bit_vector>>(
        m, ("WaveletMatrixInt" + base_name).c_str(), doc_wm_int);
    m.attr("wavelet_matrix_int").attr("__setitem__")(base_name, cls);

    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_huff(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_huff<bit_vector>>(
        m, ("WaveletTreeHuffman" + base_name).c_str(), doc_wt_huff);
    m.attr("wavelet_tree_huffman").attr("__setitem__")(base_name, cls);

    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_huff_int(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_huff_int<bit_vector>>(
        m, ("WaveletTreeHuffmanInt" + base_name).c_str(), doc_wt_huff);
    m.attr("wavelet_tree_huffman_int").attr("__setitem__")(base_name, cls);

    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_hutu(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_hutu<bit_vector>>(
        m, ("WaveletTreeHuTucker" + base_name).c_str(), doc_wt_hutu);
    m.attr("wavelet_tree_hu_tucker").attr("__setitem__")(base_name, cls);

    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_hutu_int(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_hutu_int<bit_vector>>(
        m, ("WaveletTreeHuTuckerInt" + base_name).c_str(), doc_wt_hutu);
    m.attr("wavelet_tree_hu_tucker_int").attr("__setitem__")(base_name, cls);

    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_blcd(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_blcd<bit_vector>>(
        m, ("WaveletTreeBalanced" + base_name).c_str(), doc_wt_blcd);
    m.attr("wavelet_tree_balanced").attr("__setitem__")(base_name, cls);

    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_blcd_int(py::module& m, std::string&& base_name)
{
    auto cls = add_wavelet_class<sdsl::wt_blcd_int<bit_vector>>(
        m, ("WaveletTreeBalancedInt" + base_name).c_str(), doc_wt_blcd);
    m.attr("wavelet_tree_balanced_int").attr("__setitem__")(base_name, cls);

    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_int(py::module& m, const std::string& base_name)
{ return add_wt_int<bit_vector>(m, std::string(base_name)); }


template <class bit_vector=sdsl::bit_vector>
inline auto add_wm_int(py::module& m, const std::string& base_name)
{ return add_wm_int<bit_vector>(m, std::string(base_name)); }


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_huff(py::module& m, const std::string& base_name)
{ return add_wt_huff<bit_vector>(m, std::string(base_name)); }

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_huff_int(py::module& m, const std::string& base_name)
{ return add_wt_huff_int<bit_vector>(m, std::string(base_name)); }

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_hutu(py::module& m, const std::string& base_name)
{ return add_wt_hutu<bit_vector>(m, std::string(base_name)); }

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_hutu_int(py::module& m, const std::string& base_name)
{ return add_wt_hutu_int<bit_vector>(m, std::string(base_name)); }

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_blcd(py::module& m, const std::string& base_name)
{ return add_wt_blcd<bit_vector>(m, std::string(base_name)); }

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_blcd_int(py::module& m, const std::string& base_name)
{ return add_wt_blcd_int<bit_vector>(m, std::string(base_name)); }


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_int(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_int<bit_vector>(m, base_name);
    m.attr("wavelet_tree_int_by_base").attr("__setitem__")(base, cls);
    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wm_int(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wm_int<bit_vector>(m, base_name);
    m.attr("wavelet_matrix_int_by_base").attr("__setitem__")(base, cls);
    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_huff(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_huff<bit_vector>(m, base_name);
    m.attr("wavelet_tree_huffman_by_base").attr("__setitem__")(base, cls);
    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_huff_int(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_huff_int<bit_vector>(m, base_name);
    m.attr("wavelet_tree_huffman_int_by_base").attr("__setitem__")(base, cls);
    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_hutu(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_hutu<bit_vector>(m, base_name);
    m.attr("wavelet_tree_hu_tucker_by_base").attr("__setitem__")(base, cls);
    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_hutu_int(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_hutu_int<bit_vector>(m, base_name);
    m.attr("wavelet_tree_hu_tucker_int_by_base").attr("__setitem__")(base, cls);
    return cls;
}


template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_blcd(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_blcd<bit_vector>(m, base_name);
    m.attr("wavelet_tree_balanced_by_base").attr("__setitem__")(base, cls);
    return cls;
}

template <class bit_vector=sdsl::bit_vector>
inline auto add_wt_blcd_int(py::module& m, const py::class_<bit_vector>& base)
{
    auto base_name = py::cast<std::string>(base.attr("__name__"));
    auto cls = add_wt_blcd_int<bit_vector>(m, base_name);
    m.attr("wavelet_tree_balanced_int_by_base").attr("__setitem__")(base, cls);
    return cls;
}


template <class... T>
inline auto add_wavelet(py::module& m,
                        const std::tuple<py::class_<T>...> t)
{
    m.attr("all_wavelet_trees") = py::list();
    m.attr("wavelet_tree_int") = py::dict();
    m.attr("wavelet_tree_int_by_base") = py::dict();
    m.attr("wavelet_matrix_int") = py::dict();
    m.attr("wavelet_matrix_int_by_base") = py::dict();
    m.attr("wavelet_tree_huffman") = py::dict();
    m.attr("wavelet_tree_huffman_by_base") = py::dict();
    m.attr("wavelet_tree_huffman_int") = py::dict();
    m.attr("wavelet_tree_huffman_int_by_base") = py::dict();
    m.attr("wavelet_tree_hu_tucker") = py::dict();
    m.attr("wavelet_tree_hu_tucker_by_base") = py::dict();
    m.attr("wavelet_tree_hu_tucker_int") = py::dict();
    m.attr("wavelet_tree_hu_tucker_int_by_base") = py::dict();
    m.attr("wavelet_tree_balanced") = py::dict();
    m.attr("wavelet_tree_balanced_by_base") = py::dict();
    m.attr("wavelet_tree_balanced_int") = py::dict();
    m.attr("wavelet_tree_balanced_int_by_base") = py::dict();

    return std::make_tuple(
        add_wt_int<>(m, ""),
        add_wt_int(m, std::get<0>(t)),
        add_wt_int(m, std::get<1>(t)),
        add_wt_int(m, std::get<2>(t)),

        add_wm_int<>(m, ""),
        add_wm_int(m, std::get<0>(t)),
        add_wm_int(m, std::get<1>(t)),
        add_wm_int(m, std::get<2>(t)),

        add_wt_huff<>(m, ""),
        add_wt_huff(m, std::get<0>(t)),
        add_wt_huff(m, std::get<1>(t)),
        add_wt_huff(m, std::get<2>(t)),

        add_wt_huff_int<>(m, ""),
        add_wt_huff_int(m, std::get<0>(t)),
        add_wt_huff_int(m, std::get<1>(t)),
        add_wt_huff_int(m, std::get<2>(t)),

        add_wt_hutu<>(m, ""),
        add_wt_hutu(m, std::get<0>(t)),
        add_wt_hutu(m, std::get<1>(t)),
        add_wt_hutu(m, std::get<2>(t)),

        add_wt_hutu_int<>(m, ""),
        add_wt_hutu_int(m, std::get<0>(t)),
        add_wt_hutu_int(m, std::get<1>(t)),
        add_wt_hutu_int(m, std::get<2>(t)),

        add_wt_blcd<>(m, ""),
        add_wt_blcd(m, std::get<0>(t)),
        add_wt_blcd(m, std::get<1>(t)),
        add_wt_blcd(m, std::get<2>(t)),

        add_wt_blcd_int<>(m, ""),
        add_wt_blcd_int(m, std::get<0>(t)),
        add_wt_blcd_int(m, std::get<1>(t)),
        add_wt_blcd_int(m, std::get<2>(t)),

        add_wavelet_class<sdsl::wt_gmr_rs<>>(m, "WaveletTreeGMRrankselect",
                                             doc_wt_gmr_rs),
        add_wavelet_class<sdsl::wt_gmr_rs<sdsl::enc_vector<>>>(
            m, "WaveletTreeGMRrankselectEnc", doc_wt_gmr_rs),
        add_wavelet_class<sdsl::wt_gmr<>>(m, "WaveletTreeGolynskiMunroRao",
                                          doc_wt_gmr),
        add_wavelet_class<sdsl::wt_gmr<sdsl::enc_vector<>>>(
            m, "WaveletTreeGolynskiMunroRaoEnc", doc_wt_gmr),

        add_wavelet_class<sdsl::wt_ap<>>(m, "WaveletTreeAP", doc_wt_ap));
}
