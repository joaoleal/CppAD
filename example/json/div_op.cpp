/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-19 Bradley M. Bell

CppAD is distributed under the terms of the
             Eclipse Public License Version 2.0.

This Source Code may also be made available under the following
Secondary License when the conditions for such availability set forth
in the Eclipse Public License, Version 2.0 are satisfied:
      GNU General Public License, Version 2.0 or later.
---------------------------------------------------------------------------- */
/*
$begin json_div_op.cpp$$
$spell
    Json
$$

$section Using The Json Division Operator: Example and Test$$

$head Source Code$$
$srcfile%example/json/div_op.cpp%0%// BEGIN C++%// END C++%1%$$

$end
*/
// BEGIN C++
# include <cppad/cppad.hpp>

bool div_op(void)
{   bool ok = true;
    using CppAD::vector;
    using CppAD::AD;
    using CppAD::NearEqual;
    double eps99 = 99.0 * std::numeric_limits<double>::epsilon();
    //
    // AD graph example
    // node_1 : p[0]
    // node_2 : p[1]
    // node_3 : x[0]
    // node_4 : p[0] / p[1]
    // node_5 : x[0] * p[1] / p[0]
    // y[0]   = p[0] / p[1]
    // y[1]   = x[0] * p[1] / p[0]
    // use single quote to avoid having to escape double quote
    std::string graph =
        "{\n"
        "   'function_name'  : 'div example',\n"
        "   'op_define_vec'  : [ 1, [\n"
        "       { 'op_code':1, 'name':'div', 'n_arg':2 } ]\n"
        "   ],\n"
        "   'n_dynamic_ind'  : 2,\n"
        "   'n_independent'  : 1,\n"
        "   'string_vec'     : 0, [ ],\n"
        "   'constant_vec'   : 0, [ ],\n"
        "   'op_usage_vec'   : 2, [\n"
        "       [ 1, 1, 2 ] ,\n" // p[0] / p[1]
        "       [ 1, 3, 4 ] ]\n" // x[0] * p[1] / p[0]
        "   ,\n"
        "   'dependent_vec' : 2, [4, 5]\n"
        "}\n";
    // Convert the single quote to double quote
    for(size_t i = 0; i < graph.size(); ++i)
        if( graph[i] == '\'' ) graph[i] = '"';
    //
    // f(x, p) = [ p_0 / p_1 , x_0 * p_1 / p_0 ]
    CppAD::ADFun<double> f;
    f.from_json(graph);
    ok &= f.Domain() == 1;
    ok &= f.Range() == 2;
    ok &= f.size_dyn_ind() == 2;
    //
    // set independent variables and parameters
    vector<double> p(2), x(1);
    p[0] = 2.0;
    p[1] = 3.0;
    x[0] = 4.0;
    //
    // compute y = f(x, p)
    f.new_dynamic(p);
    vector<double> y = f.Forward(0, x);
    //
    // check result
    ok &= NearEqual(y[0] , p[0] / p[1] , eps99, eps99 );
    ok &= NearEqual(y[1] , x[0] * p[1] / p[0] , eps99, eps99 );
    //
    return ok;
}
// END C++
