template<typename Real>
Real SplineInterp<Real>::operator()(Real x)
{/*{{{*/
  // don't extrapolate at all
  if( x < X[0] )
    return 0;
     
  if( x > X[this->n-1] )
    return 0;
  
  // find the index that is just to the right of the x
  int i = 1;
  while( i < this->n-1 && X[i] < x )
    i++;

  // See the wikipedia page on "Spline interpolation" (https://en.wikipedia.org/wiki/Spline_interpolation)
  // for a derivation this interpolation.
  Real t = ( x - X[i-1] ) / ( X[i] - X[i-1] );
  Real q = ( 1 - t ) * Y[i-1] + t * Y[i] + t*(1-t)*(a[i-1]*(1-t)+b[i-1]*t);
  
  return q;
} /*}}}*/

template<typename Real>
Real SplineInterp<Real>::derivative(Real x)
{/*{{{*/
    //No extrapolation
    if( x < X[0] )
        return 0;

    if( x > X[this->n-1] )
        return 0;

    // find the index that is just to the right of x
    int i = 1;
    while( i < this->n-1 && X[i] < x )
        i++;

    //this should be the same t as in the regular interpolation case
    Real t = ( x - X[i-1] ) / ( X[i] - X[i-1] );

    Real qprime = ( Y[i] - Y[i-1] )/( X[i]-X[i-1] ) + ( 1 - 2*t )*( a[i-1]*(1-t) + b[i-1]*t )/( X[i] - X[i-1])
                    + t*(1-t)*(b[i-1]-a[i-1])/(X[i]-X[i-1]) ;

    return qprime;
}/*}}}*/

template<typename Real>
Real SplineInterp<Real>::integral(Real _a, Real _b)
{/*{{{*/
    // allow b to be less than a
    Real sign = 1;
    if( _a > _b )
    {
      std::swap( a, b );
      sign = -1;
    }
    //No extrapolation
    _a = std::max( _a, X[0] );
    _b = std::min( _b, X[this->n-1] );

    // find the indexes that is just to the right of a and b
    int ai = 1;
    while( ai < this->n-1 && X[ai] < _a )
        ai++;
    int bi = 1;
    while( bi < this->n-1 && X[bi] < _b )
        bi++;

    /**
     *
     * We can integrate the function directly using its cubic spline representation.
     *
     * from wikipedia:
     *
     * q(x) = ( 1 - t )*y_1 + t*y_2 + t*( 1 - t )( a*(1 - t) + b*t )
     * 
     * t = (x - x_1) / (x_2 - x_1)
     *
     * I = \int_a^b q(x) dx = \int_a^b ( 1 - t )*y_1 + t*y_2 + t*( 1 - t )( a*(1 - t) + b*t ) dx
     *
     * variable substitution: x -> t
     *
     * dt = dx / (x_2 - x_1)
     * t_a = (a - x_1) / (x_2 - x_1)
     * t_b = (b - x_1) / (x_2 - x_1)
     *
     * I = (x_2 - x_1) \int_t_a^t_b ( 1 - t )*y_1 + t*y_2 + t*( 1 - t )( a*(1 - t) + b*t ) dt
     *
     *   = (x_2 - x_1) [ ( t - t^2/2 )*y_1 + t^2/2*y_2 + a*(t^2 - 2*t^3/3 + t^4/4) + b*(t^3/3 - t^4/4) ] |_t_a^t_b
     *
     * if we integrate over the entire element, i.e. x -> [x1,x2], then we will have
     * t_a = 0, t_b = 1. This gives
     *
     * I = (x_2 - x_1) [ ( 1 - 1/2 )*y_1 + 1/2*y_2 + a*(1 - 2/3 + 1/4) + b*(1/3 - 1/4) ]
     *
     */

    Real x_1, x_2, t;
    Real y_1, y_2;
    Real sum = 0;
    for( int i = ai; i < bi-1; i++)
    {
      // x_1 -> X(i)
      // x_2 -> X(i+1)
      // y_1 -> Y(i)
      // y_2 -> Y(i+1)
      x_1 = X[i];
      x_2 = X[i+1];
      y_1 = Y[i];
      y_2 = Y[i+1];
      // X(ai) is to the RIGHT of _a
      // X(bi) is to the RIGHT of _b, but i only goes up to bi-2 and
      // X(bi-1) is to the LEFT of _b
      // therefore, we are just handling interior elements in this loop.
      sum += (x_2 - x_1)*( 0.5*(y_1 + y_2) + (1./12)*(a[i] + b[i]) );
    }


    // now we need to handle the area between [_a,X(ai)] and [X(bi-1),_b]


    // [X(0),_b]
    // x_1 -> X(bi-1)
    // x_2 -> X(bi)
    // y_1 -> Y(bi-1)
    // y_2 -> Y(bi)
    x_1 = X[bi-1];
    x_2 = X[bi];
    y_1 = Y[bi-1];
    y_2 = Y[bi];
    t   = (_b - x_1)/(x_2 - x_1);

    // adding area between x_1 and _b
    sum += (x_2 - x_1) * ( ( t - pow(t,2)/2 )*y_1 + pow(t,2)/2.*y_2 + a[bi-1]*(pow(t,2) - 2.*pow(t,3)/3. + pow(t,4)/4.) + b[bi-1]*(pow(t,3)/3. - pow(t,4)/4.) );

    //
    // [_a,X(0)]
    // x_1 -> X(ai-1)
    // x_2 -> X(ai)
    // y_1 -> Y(ai-1)
    // y_2 -> Y(ai)
    x_1 = X[ai-1];
    x_2 = X[ai];
    y_1 = Y[ai-1];
    y_2 = Y[ai];
    t   = (_a - x_1)/(x_2 - x_1);

    // subtracting area from x_1 to _a
    sum -= (x_2 - x_1) * ( ( t - pow(t,2)/2 )*y_1 + pow(t,2)/2.*y_2 + a[ai-1]*(pow(t,2) - 2.*pow(t,3)/3. + pow(t,4)/4.) + b[ai-1]*(pow(t,3)/3. - pow(t,4)/4.) );

    if( ai != bi ) // _a and _b are not in the in the same element, need to add area of element containing _a
      sum += (x_2 - x_1)*( 0.5*(y_1 + y_2) + (1./12)*(a[ai-1] + b[ai-1]) );

    return sign*sum;
}/*}}}*/

template<typename Real>
Real SplineInterp<Real>::integral()
{/*{{{*/
    return this->integral( X[0], X[this->n-1] );
}/*}}}*/

template<typename Real>
void SplineInterp<Real>::setData( size_t _n, Real *_x, Real *_y )
{/*{{{*/
    this->n = _n;

    this->X.resize(this->n);
    this->Y.resize(this->n);

    for (int i = 0; i < this->n; ++i)
    {
       this->X[i] = _x[i];
       this->Y[i] = _y[i];
    }

    this->initCoefficients();
}/*}}}*/

template<typename Real>
void SplineInterp<Real>::setData( std::vector<Real> &x, std::vector<Real> &y )
{/*{{{*/
    this->setData( x.size(), x.data(), y.data() );
}/*}}}*/

template<typename Real>
void SplineInterp<Real>::initCoefficients()
{/*{{{*/
  // these are the vectors we want to setup
  this->a.resize(this->n - 1);
  this->b.resize(this->n - 1);


  // we need to solve A x = b, where A is a matrix and b is a vector...
  // isn't that what we are always doing?
  
#ifdef USE_EIGEN
  typedef Eigen::SparseMatrix<Real>              MatrixType;
  typedef Eigen::Matrix< Real,Eigen::Dynamic,1 > VectorType;
  MatrixType A(this->n,this->n);
  VectorType b(this->n);
  Eigen::SparseLU<MatrixType> solver;

  std::vector< Eigen::Triplet<Real> > initA;
  
  for(int i = 0; i < this->n; ++i)
  {
    Real coeff;

    // first row
    if( i == 0 )
    {
      coeff = 1/(X[i+1] - X[i]);
      initA.push_back( Eigen::Triplet<Real>( i, i+1, coeff ) );
      coeff *= 2;
      initA.push_back( Eigen::Triplet<Real>( i, i, coeff ) );
    }
    // last row
    else if (i == n-1)
    {
      coeff = 1 / (X[i] - X[i-1]);
      initA.push_back( Eigen::Triplet<Real>( i, i-1, coeff ) );
      coeff *= 2;
      initA.push_back( Eigen::Triplet<Real>( i, i, coeff ) );
    }
    // middle rows
    else
    {
      // sub-diag
      coeff = 1 / (X[i] - X[i-1]);
      initA.push_back( Eigen::Triplet<Real>( i, i-1, coeff ) );
      // sup-diag
      coeff = 1/(X[i+1] - X[i]);
      initA.push_back( Eigen::Triplet<Real>( i, i+1, coeff ) );
      //diag
      coeff = 2 / (X[i]-X[i-1]) + 2 / (X[i+1] - X[i]);
      initA.push_back( Eigen::Triplet<Real>( i, i, coeff ) );
    }

  }

  A.setFromTriplets( initA.begin(), initA.end() );
  A.makeCompressed();

  for(int i = 0; i < n; ++i)
  {   
      if(i == 0)
      {   
        b(i) = 3 * ( Y[i+1] - Y[i] )/pow(X[i+1]-X[i],2);
      }
      else if( i == n-1 )
      {   
        b(i) = 3 * (Y[i] - Y[i-1])/pow(X[i] - X[i-1],2);
      }
      else
      { 
        b(i) = 3 * ( (Y[i] - Y[i-1])/(pow(X[i]-X[i-1],2)) + (Y[i+1] - Y[i])/pow(X[i+1] - X[i],2));     
      }
  }


  solver.compute(A);
  VectorType x = solver.solve(b);

  for (int i = 0; i < this->n - 1; ++i)
  {
      this->a[i] = x(i) * (X[i+1]-X[i]) - (Y[i+1] - Y[i]);
      this->b[i] = -x(i+1) * (X[i+1] - X[i]) + (Y[i+1] - Y[i]);
  }

#else

  /*
   * Solves Ax=B using the Thomas algorithm, because the matrix A will be tridiagonal and diagonally dominant.
   *
   * The method is outlined on the Wikipedia page for Tridiagonal Matrix Algorithm
   */

  //init the matrices that get solved
  std::vector<Real> Aa(this->n), Ab(this->n), Ac(this->n);  // three three diagonals of the tridiagonal matrix A
  std::vector<Real> b(this->n); // RHS vector


  //Ac is a vector of the upper diagonals of matrix A
  //
  //Since there is no upper diagonal on the last row, the last value must be zero.
  for (size_t i = 0; i < this->n-1; ++i)
  {
      Ac[i] = 1/(X[i+1] - X[i]);
  }
  Ac[this->n] = 0.0;

  //Ab is a vector of the diagnoals of matrix A
  Ab[0] = 2/(X[1] - X[0]);
  for (size_t i = 1; i < this->n-1; ++i)
  {
      Ab[i] = 2 / (X[i]-X[i-1]) + 2 / (X[i+1] - X[i]);
  }
  Ab[this->n-1] = 2/(X[this->n-1] - X[this->n-1-1]);


  //Aa is a vector of the lower diagonals of matrix A
  //
  //Since there is no upper diagonal on the first row, the first value must be zero.
  Aa[0] = 0.0;
  for (size_t i = 1; i < this->n; ++i)
  {
      Aa[i] = 1 / (X[i] - X[i-1]);
  }



  // setup RHS vector
  for(int i = 0; i < n; ++i)
  {   
      if(i == 0)
      {   
        b[i] = 3 * ( Y[i+1] - Y[i] )/pow(X[i+1]-X[i],2);
      }
      else if( i == n-1 )
      {   
        b[i] = 3 * (Y[i] - Y[i-1])/pow(X[i] - X[i-1],2);
      }
      else
      { 
        b[i] = 3 * ( (Y[i] - Y[i-1])/(pow(X[i]-X[i-1],2)) + (Y[i+1] - Y[i])/pow(X[i+1] - X[i],2));     
      }
  }









  std::vector<Real> c_star;
  c_star.resize( Ac.size() );
  c_star[0] = Ac[0]/Ab[0];
  for (size_t i = 1; i < c_star.size(); ++i)
  {
     c_star[i] = Ac[i] / (Ab[i]-Aa[i]*c_star[i-1]); 
  }

  std::vector<Real> d_star;
  d_star.resize(this->n);
  d_star[0] = b[0]/Ab[0];

  for (size_t i = 1; i < d_star.size(); ++i)
  {
      d_star[i] = (b[i] - Aa[i]*d_star[i-1])/(Ab[i]-Aa[i]*c_star[i-1]);
  }

  std::vector<Real> x;
  x.resize( this->n );
  x[ x.size() - 1 ] = d_star[ d_star.size() - 1 ];

  for (size_t i = x.size() - 1; i-- > 0;)
  {
      x[i] = d_star[i] - c_star[i]*x[i+1];
  }

  for (int i = 0; i < this->n - 1; ++i)
  {
      this->a[i] = x[i] * (X[i+1]-X[i]) - (Y[i+1] - Y[i]);
      this->b[i] = -x[i+1] * (X[i+1] - X[i]) + (Y[i+1] - Y[i]);
  }

#endif

}/*}}}*/
