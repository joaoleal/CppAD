/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-08 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the 
                    Common Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */
/*
$begin ipopt_cppad_ode.cpp$$
$spell
	CppAD
	Ipopt
$$


$section Fitting an Ode Using the CppAD Interface to Ipopt$$

$head Purpose$$
The CppAD interface to Ipopt 
$cref/ipopt_cppad_nlp/$$
enables one to represent an optimization problem
with a large number of variables and constraints in terms of functions
with a few domain and range variables.
This is a demonstration of how to use this representation.

$head General Problem$$
This example solves a problem of the form
$latex \[
{\rm minimize} \; 
	\sum_{j=1}^N H_j [ j ,  x( t_j , a ) , a ] \;
		{\rm with \; respect \; to} \; a \in \R^p
\] $$
where $latex x(t)$$ is the solution of the ODE
$latex \[
\begin{array}{rcl}
	x(0, a)  & = &  F(a)                 \\
	x'(t, a) & = & G[ t , x(t, a) , a ]
\end{array}
\] $$

$head Specific Case$$
Most of the code below is for the general problem above but some of it
for a specific case where
$latex \[
\begin{array}{rcl}
	x_0 (t, a) & = & a_0 \exp( - a_1 t )
	\\
	x_1 (t, a) & = & a_0 a_1 [\exp(- a_2 t) - \exp(- a_1 t)] / (a_1 - a_2)
\end{array}
\] $$ 



$end
------------------------------------------------------------------------------
*/

# include "ipopt_cppad_nlp.hpp"

// include a definition for Number.
typedef Ipopt::Number Number;

// ---------------------------------------------------------------------------
namespace { // Begin empty namespace 

size_t nx = 2;    // dimension of x(t, a) for this case
size_t na = 3;    // dimension of a for this case 
size_t ng = 1;    // number of grid intervals with in each data interval

// function used to simulate data
Number x_one(Number t)
{	Number a0 = 1.; 
	Number a1 = 1.; 
	Number a2 = 1.; 
	Number x_1 =  a0 + a1 * t + a2 * t * t / 2;
	return x_1;
}

// time points were we have data
double td[] = {        1.,         2.,         3.  }; 
// This data is for the case 
double yd[] = {  x_one(1.),  x_one(2.),  x_one(3.) };
// number of data values
size_t nd   = sizeof(yd) / sizeof(yd[0]);

// F(a) = x(0, a)
template <class Vector>
Vector eval_F(Vector a)
{	// this particual F is for the case where nx == 1 and na == 2	
	Vector F(nx);
	F[0] = .0;    // x_0 (t) = t
	F[1] = a[0];  // x_1 (t) = a0 + a1 * t + a2 * t * t / 2
	return F;
}
// G(x, a) =  x'(t, a)
template <class Vector>
Vector eval_G(Vector x , Vector a)
{	// this particular G is for the case where nx == 1 and na == 2
	Vector G(nx);
	G[0] = 1.;                 // x_0 (t) = t
	G[1] = a[1] + a[2] * x[0]; // x_1 (t) = a0 + a1 * t + a2 * t * t / 2
	return G;
} 
// H(k, x, a) = contribution to objective at k-th data point
template <class Scalar, class Vector>
Scalar eval_H(size_t k, Vector x, Vector a)
{	Scalar diff = yd[k] - x[1];
 	return diff * diff;
}

/* Index plan:
Time grid:
For k = 0,...,nd - 1, dtg[k] = (td[k] - td[k-1]) / ng where td[-1] is 
interpreted as zero. For k = 0 , ... , nd-1 and ell = 0 , ... , ng-1,
        tg[k*ng+ell] = td[k-1] + dtg[k] * ell

xa
The value of x(t) at t = tg[0] is given by the first nx components of
the vector a (which is store in the last nx components of the vector xa).
For J > 0, the value of x(t) at t = tg[J] is given by
	xJ = ( xa[(J-1)*nx] , ... , xa[J*nx -1] ) 
We use the following difference approximation to solution of ODE
	0 = x_{J+1} - x_J - [G(x_{J+1}, a) + G(x_J , a)] * dtg[k]
where J > 0 and k is chosen so that td[k-1] <= tg[J] < td[k].

k:
0 < k < nd is for two sided difference equations.  Note that there is one less 
such equation for k = 0.  k = nd is for the initial difference equation.
k = nd + 1 , ... , nd + nd is for the objective function.
*/
class my_FG_info : public ipopt_cppad_fg_info
{
private:
	bool retape_;
public:
	// derived class part of constructor
	my_FG_info(bool retape)
	: retape_ (retape)
	{ }
	size_t number_functions(void)
	{	return nd + 1 + nd; }
	ADVector eval_r(size_t k, const ADVector &u)
	{	size_t j;
		// objective function case
		if( k > nd )
		{	// We use a differnent k for each data interval
			// (there may be more efficient ways to do this).
			ADVector r(1);
			size_t j;
			// u is [x(t) , a] where t = td[ell]
			ADVector xJ(nx), a(na);
			for(j = 0; j < nx; j++)
				xJ[j] = u[j];
			for(j = 0; j < na; j++)
				a[j] = u[nx + j];
			r[0] = eval_H<ADNumber>(k - nd - 1, xJ, a);
			return r;
		}
		ADVector xJ(nx), xJ1(nx), a(na);
		Number dtg;
		if( k == nd )
		{	// u = [xJ1 , a] where xJ1 is x(t) at t = tg[1]
			for(j = 0; j < nx; j++)
				xJ1[j] = u[j];
			for(j = 0; j < na; j++)
				a[j] = u[nx + j];
			// xJ is value of x(t) at t = tg[0]
			xJ = eval_F(a);
			// size of subinterval
			dtg = (td[0] - 0.) / Number(ng);
		}
		else
		{	// u = [xJ, xJ1, a] where xJ is x(t) at 
			// t = tg[ k * ng + ell + 1 ] for k = 0 case and
			// t = tg[ k * ng + ell ] for 0 < k < nd cases
			for(j = 0; j < nx; j++)
			{	xJ[j]  = u[j];
				xJ1[j] = u[nx + j];
			}
			for(j = 0; j < na; j++)
				a[j] = u[2 * nx + j];
			if( k == 0 )
				dtg = (td[0] - 0.) / Number(ng);
			else	dtg = (td[k] - td[k-1]) / Number(ng);
		}
		ADVector GJ   = eval_G(xJ,  a);
		ADVector GJ1  = eval_G(xJ1, a);
		ADVector r(nx);
		for(j = 0; j < nx; j++)
			r[j] = xJ1[j] - xJ[j] - (GJ1[j] + GJ[j]) * dtg / 2.;
		return r;
	}
	// Operation sequence does not depend on u so retape = false should
	// work and be faster. Allow for both for testing.
	bool retape(size_t k)
	{	return retape_; }
	// size of the vector u in eval_r
	size_t domain_size(size_t k)
	{	if( k > nd )
			return nx + na;   // objective function
		if( k == nd )
			return nx + na;  // initial difference equation
		return 2 * nx + na;      // other difference equations
	}
	// size of the vector r in eval_r
	size_t range_size(size_t k)
	{	if( k > nd )
			return 1;
		return nx; 
	}
	size_t number_terms(size_t k)
	{
		if( k > nd )
			return 1;        // objective function terms per k
		if( k == nd )
			return 1;        // one initial difference equation
		if( k == 0 )
			return ng - 1;   // ng - 1 first data interval special
		return ng;               // number difference equations
	}
	// k is index of time interal between data values
	// ell is index of grid point within data interval
	void index(size_t k, size_t ell, SizeVector& I, SizeVector& J)
	{	size_t i, j;
		// number of constraints, one for each subinterval interval
		// times the number of components in x(t)
		size_t m = nd * ng * nx;
		// special range index for objective fucntion
		if( k > nd )
		{	// the first component of fg	
			I[0] = 0;
			// The first nx components of u is x(t) at t = td[j]; 
			// i.e. at time tg[(j+1)*ng]. Note that the xa starts 
			// with x(t) at time tg[1] and x(t). Also Note that 
			// there are ng subintervals between each data point.
			for(j = 0; j < nx; j++)
				J[j] = ( (k - nd) * ng - 1) * nx + j;
			// all of a (which is last na components of xa)
			for(j = 0; j < na; j++)
				J[nx + j] = m + j; 
			return;
		}
		// special grid equaiton for initial value
		if( k == nd )
		{	// first nx components of g
			for(i = 0; i < nx; i++)
				I[i] = 1 + i;
			// variables are first j components of xa
			for(j = 0; j < nx; j++)
				J[j] = j;
			// and all of a (which is last na components of xa)
			for(j = 0; j < na; j++)
				J[nx + j] = m + j;
			return;
		}
		size_t grid_point; 
		if( k == 0 )
			grid_point = k * ng + ell + 1;
		else	grid_point = k * ng + ell;
		for(j = 0; j < nx; j++)
		{	J[j]          = (grid_point - 1) * nx  + j; // xJ
			J[nx + j]     = J[j] + nx;                  // xJ1
		}
		for(j = 0; j < na; j++)
			J[2 * nx + j] = m + j;                      // a
		// There are nx difference equations for each grid point.
		// (add one for the objective function index).
		for(i = 0; i < nx; i++)
			I[i] = grid_point * nx + 1 + i;
	} 
};

} // End empty namespace
// ---------------------------------------------------------------------------

bool ipopt_cppad_ode(void)
{	bool ok = true;
	size_t j, I, J;

	// number of constraints (range dimension of g)
	size_t m = nd * ng * nx;
	// number of independent variables (domain dimension for f and g)
	size_t n = m + na;
	// the independent variable vector is x at every t value followed by a
	NumberVector xa_i(n), xa_l(n), xa_u(n);
	for(J = 0; J < m; J++)
	{	xa_i[J] = 0.;       // initial x for optimization
		xa_l[J] = -1.0e19;  // no lower limit
		xa_u[J] = +1.0e19;  // no upper limit
	}
	for(j = 0; j < na; j++)
	{	xa_i[m + j ] = .5;       // initiali a for optimization
		xa_l[m + j ] =  -1.e19;  // no lower limit
		xa_u[m + j ] =  +1.e19;  // no upper
	}
	// all of the difference equations are contrianed to the value zero
	NumberVector g_l(m), g_u(m);
	for(I = 0; I < m; I++)
	{	g_l[I] = 0.;
		g_u[I] = 0.;
	}
	// derived class object
	
	for(size_t icase = 0; icase <= 1; icase++)
	{
		bool retape = bool(icase);

		// object defining the objective f(x) and constraints g(x)
		my_FG_info fg_info(retape);

		// create the CppAD Ipopt interface
		ipopt_cppad_solution solution;
		Ipopt::SmartPtr<Ipopt::TNLP> cppad_nlp = new ipopt_cppad_nlp(
			n, m, xa_i, xa_l, xa_u, g_l, g_u, &fg_info, &solution
		);

		// Create an Ipopt application
		using Ipopt::IpoptApplication;
		Ipopt::SmartPtr<IpoptApplication> app = new IpoptApplication();

		// turn off any printing
		app->Options()->SetIntegerValue("print_level", -2);

		// maximum number of iterations
		app->Options()->SetIntegerValue("max_iter", 20);

		// approximate accuracy in first order necessary conditions;
		// see Mathematical Programming, Volume 106, Number 1, 
		// Pages 25-57, Equation (6)
		app->Options()->SetNumericValue("tol", 1e-9);

		// derivative testing (this is very slow for large problems)
		app->Options()-> SetStringValue(
			"derivative_test", "second-order"
		);

		// Initialize the application and process the options
		Ipopt::ApplicationReturnStatus status = app->Initialize();
		ok    &= status == Ipopt::Solve_Succeeded;

		// Run the application
		status = app->OptimizeTNLP(cppad_nlp);
		ok    &= status == Ipopt::Solve_Succeeded;

		// Check some of the return values
		Number rel_tol = 1e-6;
		Number abs_tol = 1e-6;
		double check_a[] = {1., 1., 1.}; // see the x_one function
		for(j = 0; j < na; j++)
		{
			ok &= CppAD::NearEqual( 
				check_a[j], solution.x[m+j], rel_tol, abs_tol
			);
		}

		// split out return values
		NumberVector a(na), x0(nx), x1(nx), x2(nx);
		for(j = 0; j < na; j++)
			a[j] = solution.x[m+j];
		x0 = eval_F(a);
		for(j = 0; j < nx; j++)
		{	x1[j] = solution.x[j];
			x2[j] = solution.x[nx + j];
		} 
		//
		// check the initial difference equation
		NumberVector G0 = eval_G(x0, a);
		NumberVector G1 = eval_G(x1, a);
		Number dtg = td[0] / Number(ng);
		for(j = 0; j < nx; j++)
		{	Number check = x1[j] - x0[j]-(G1[j]+G0[j])*dtg/2;
			ok &= CppAD::NearEqual( check, 0., rel_tol, abs_tol);
		}
		//
		// check the second difference equation
		NumberVector G2 = eval_G(x2, a);
		if( ng == 1 )
			dtg = (td[1] - td[0]) / Number(ng);
		for(j = 0; j < nx; j++)
		{	Number check = x2[j] - x1[j]-(G2[j]+G1[j])*dtg/2;
			ok &= CppAD::NearEqual( check, 0., rel_tol, abs_tol);
		}
		//
		// check the objective function (specialized to this case)
		Number check = 0.;
		NumberVector xk(nx);
		for(size_t k = 0; k < nd; k++)
		{	for(j = 0; j < nx; j++)
			{	size_t grid_point = (k + 1) * ng;
				xk[j] =  solution.x[(grid_point-1) * nx + j];
			}
			check += eval_H<Number>(k, xk, a);
		}
		Number obj_value = solution.obj_value;
		ok &= CppAD::NearEqual(check, obj_value, rel_tol, abs_tol);
	}
	return ok;
}
