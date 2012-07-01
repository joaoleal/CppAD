/* $Id$ */
/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-12 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the 
                    Common Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

/*
$begin eigen_array.cpp$$
$spell
	Eigen
$$

$section Using Eigen Arrays: Example and Test$$

$index array, eigen example$$
$index eigen, array example$$
$index example, eigen array$$
$index test, eigen array$$

$code
$verbatim%example/eigen_array.cpp%0%// BEGIN C++%// END C++%1%$$
$$

$end
*/
// BEGIN C++
# include <cppad/example/cppad_eigen.hpp>
# include <cppad/speed/det_by_minor.hpp>
# include <Eigen/Dense>

bool eigen_array(void)
{	bool ok = true;
	using CppAD::AD;
	using CppAD::NearEqual;
	using Eigen::Array;
	using Eigen::Dynamic;
	//
	typedef Array< double     , Dynamic, 1 > array;
	typedef Array< AD<double> , Dynamic, 1 > a_array;
	//
	typedef CPPAD_TEST_VECTOR< double >       vector;
	typedef CPPAD_TEST_VECTOR< AD<double> >   a_vector;
	// some temporary indices
	size_t i, j;

	// domain and range space vectors
	size_t n  = 10, m = n;
	a_vector a_x(n), a_y(m);

	// set and declare independent variables and start tape recording
	for(j = 0; j < n; j++)
		a_x[j] = double(1 + j);
	CppAD::Independent(a_x);

	// copy independent variable vector to a array
	a_array a_X(n);
	array X(n);
	for(j = 0; j < n; j++)
		a_X(j) = a_x[j];

	// evaluate a component wise function
	a_array a_Y = a_X + sin(a_X); 
	
	// copy independent variable to a simple vector
	for(i = 0; i < m; i++)
		a_y[i] = a_Y(i); 

	// create f: x -> y and stop tape recording
	CppAD::ADFun<double> f(a_x, a_y); 

	// compute the derivative of y w.r.t x using CppAD
	CPPAD_TEST_VECTOR<double> x(n);
	for(j = 0; j < n; j++)
		x[j] = double(j) + 1.0 / double(j+1);
	CPPAD_TEST_VECTOR<double> jac = f.Jacobian(x);

	// check Jacobian
	double eps = 100. * CppAD::numeric_limits<double>::epsilon();
	for(i = 0; i < m; i++)
	{	for(j = 0; j < n; j++)
		{	double check = 1.0 + cos(x[i]);
			if( i != j )
				check = 0.0;
			ok &= NearEqual(jac[i * n + j], check, eps, eps);
		}
	}

	return ok;
}

// END C++
