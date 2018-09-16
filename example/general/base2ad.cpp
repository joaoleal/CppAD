/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

/*
$begin mul_level_ode.cpp$$
$spell
	Taylor
	Cpp
	const
	std
	Adolc
	adouble
$$

$section Taylor's Ode Solver: A Multi-Level AD Example and Test$$

$head Purpose$$
This is a realistic example using
two levels of AD; see $cref mul_level$$.
The first level uses $code AD<double>$$ to tape the solution of an
ordinary differential equation.
This solution is then differentiated with respect to a parameter vector.
The second level uses $code AD< AD<double> >$$
to take derivatives during the solution of the differential equation.
These derivatives are used in the application
of Taylor's method to the solution of the ODE.
The example $cref mul_level_adolc_ode.cpp$$ computes the same values using
Adolc's type $code adouble$$ and CppAD's type $code AD<adouble>$$.
The example $cref ode_taylor.cpp$$ is a simpler applications
of Taylor's method for solving an ODE.

$head ODE$$
For this example the ODE's are defined by the function
$latex h : \B{R}^n \times \B{R}^n \rightarrow \B{R}^n$$ where
$latex \[
	h[ x, y(t, x) ] =
	\left( \begin{array}{c}
			x_0                     \\
			x_1 y_0 (t, x)          \\
			\vdots                  \\
			x_{n-1} y_{n-2} (t, x)
	\end{array} \right)
	=
	\left( \begin{array}{c}
			\partial_t y_0 (t , x)      \\
			\partial_t y_1 (t , x)      \\
			\vdots                      \\
			\partial_t y_{n-1} (t , x)
	\end{array} \right)
\] $$
and the initial condition $latex y(0, x) = 0$$.
The value of $latex x$$ is fixed during the solution of the ODE
and the function $latex g : \B{R}^n \rightarrow \B{R}^n$$ is used to
define the ODE where
$latex \[
	g(y) =
	\left( \begin{array}{c}
			x_0     \\
			x_1 y_0 \\
			\vdots  \\
			x_{n-1} y_{n-2}
	\end{array} \right)
\] $$

$head ODE Solution$$
The solution for this example can be calculated by
starting with the first row and then using the solution
for the first row to solve the second and so on.
Doing this we obtain
$latex \[
	y(t, x ) =
	\left( \begin{array}{c}
		x_0 t                  \\
		x_1 x_0 t^2 / 2        \\
		\vdots                 \\
		x_{n-1} x_{n-2} \ldots x_0 t^n / n !
	\end{array} \right)
\] $$

$head Derivative of ODE Solution$$
Differentiating the solution above,
with respect to the parameter vector $latex x$$,
we notice that
$latex \[
\partial_x y(t, x ) =
\left( \begin{array}{cccc}
y_0 (t,x) / x_0      & 0                   & \cdots & 0      \\
y_1 (t,x) / x_0      & y_1 (t,x) / x_1     & 0      & \vdots \\
\vdots               & \vdots              & \ddots & 0      \\
y_{n-1} (t,x) / x_0  & y_{n-1} (t,x) / x_1 & \cdots & y_{n-1} (t,x) / x_{n-1}
\end{array} \right)
\] $$

$head Taylor's Method Using AD$$
We define the function $latex z(t, x)$$ by the equation
$latex \[
	z ( t , x ) = g[ y ( t , x ) ] = h [ x , y( t , x ) ]
\] $$
see $cref taylor_ode$$ for the method used to compute the
Taylor coefficients w.r.t $latex t$$ of $latex y(t, x)$$.

$head Source$$
$code
$srcfile%example/general/base2ad.cpp%0%// BEGIN C++%// END C++%1%$$
$$

$end
--------------------------------------------------------------------------
*/
// BEGIN C++

# include <cppad/cppad.hpp>

// =========================================================================
namespace { // BEGIN empty namespace

typedef CppAD::AD<double>          a_double;

typedef CPPAD_TESTVECTOR(double)   d_vector;
typedef CPPAD_TESTVECTOR(a_double) a_vector;

typedef CppAD::ADFun<double>       fun_double;
typedef CppAD::ADFun<a_double>     afun_double;

// -------------------------------------------------------------------------
// class definition for C++ function object that defines ODE
class Ode {
private:
	// copy of x that is set by constructor and used by g(y)
	a_vector x_;
public:
	// constructor
	Ode(const a_vector& x) : x_(x)
	{ }
	// the function g(y) given the parameter vector x
	a_vector operator() (const a_vector& y) const
	{	size_t n = y.size();
		a_vector g(n);
		g[0] = x_[0];
		for(size_t i = 1; i < n; i++)
			g[i] = x_[i] * y[i-1];
		//
		return g;
	}
};

// -------------------------------------------------------------------------
// Routine that uses Taylor's method to solve ordinary differential equaitons
a_vector taylor_ode(
	afun_double&     fun_g   ,  // function that defines the ODE
	size_t           order   ,  // order of Taylor's method used
	size_t           nstep   ,  // number of steps to take
	const a_double&  dt      ,  // Delta t for each step
	const a_vector&  y_ini)     // y(t) at the initial time
{
	// number of variables in the ODE
	size_t n = y_ini.size();

	// initialize y
	a_vector y = y_ini;

	// loop with respect to each step of Taylors method
	for(size_t s = 0; s < nstep; s++)
	{
		// initialize
		a_vector y_k   = y;
		a_double dt_k  = a_double(1.0);
		a_vector next  = y;

		for(size_t k = 0; k < order; k++)
		{
			// evaluate k-th order Taylor coefficient z^{(k)} (t)
			a_vector z_k = fun_g.Forward(k, y_k);

			// dt^{k+1}
			dt_k *= dt;

			// y^{(k+1)}
			for(size_t i = 0; i < n; i++)
			{	// y^{(k+1)}
				y_k[i] = z_k[i] / a_double(k + 1);

				// add term for k+1 Taylor coefficient
				// to solution for next y
				next[i] += y_k[i] * dt_k;
			}
		}

		// take step
		y = next;
	}
	return y;
}
} // END empty namespace

// ==========================================================================
// Routine that tests alogirhtmic differentiation of solutions computed
// by the routine taylor_ode.
bool base2ad(void)
{	bool ok = true;
	double eps = 100. * std::numeric_limits<double>::epsilon();

	// number of components in differential equation
	size_t n = 4;

	// record the function g(y, x) with x as dynamic parameters
	a_vector  ay(n), ax(n);
	for(size_t i = 0; i < n; i++)
		ay[i] = ax[i] = double(i + 1);


	// declare the parameters as the independent variable
	size_t abort_op_index = 0;
	bool   record_compare = true;
	CppAD::Independent(ay, abort_op_index, record_compare, ax);

	// fun_g
	Ode G(ax);
	a_vector ag = G(ay);
	fun_double fun_g(ay, ag);


	// arguments to taylor_ode
	afun_double afun_g;
	afun_g = fun_g.base2ad(); // differential equation
	size_t   order = n;       // order of Taylor's method used
	size_t   nstep = 2;       // number of steps to take
	a_double adt   = 1.;      // Delta t for each step
	a_vector ay_ini(n);       // initial value of y
	for(size_t i = 0; i < n; i++)
		ay_ini[i] = 0.;

	// declare the independent variables
	CppAD::Independent(ax);

	// set value of x in afun_g
	afun_g.new_dynamic(ax);

	// integrate the differential equation
	a_vector ay_final;
	ay_final = taylor_ode(afun_g, order, nstep, adt, ay_ini);

	// define differentiable fucntion object f(x) = y_final(x)
	// that computes its derivatives in double
	CppAD::ADFun<double> fun_f(ax, ay_final);

	// double version of ax
	d_vector x(n);
	for(size_t i = 0; i < n; i++)
		x[i] = Value( ax[i] );

	// check function values
	double check = 1.;
	double t     = double(nstep) * Value(adt);
	for(size_t i = 0; i < n; i++)
	{	check *= x[i] * t / double(i + 1);
		ok &= CppAD::NearEqual(Value(ay_final[i]), check, eps, eps);
	}

	// There appears to be a bug in g++ version 4.4.2 because it generates
	// a warning for the equivalent form
	// d_vector jac = fun_f.Jacobian(x);
	d_vector jac ( fun_f.Jacobian(x) );

	// check Jacobian
	for(size_t i = 0; i < n; i++)
	{	for(size_t j = 0; j < n; j++)
		{	double jac_ij = jac[i * n + j];
			if( i < j )
				check = 0.;
			else	check = Value( ay_final[i] ) / x[j];
			ok &= CppAD::NearEqual(jac_ij, check, eps, eps);
		}
	}
	return ok;
}

// END C++