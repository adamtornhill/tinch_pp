// Copyright (c) 2010, Adam Petersen <adam@adampetersen.se>. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//      conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//      of conditions and the following disclaimer in the documentation and/or other materials
//      provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY Adam Petersen ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Adam Petersen OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef BOOST_PP_IS_ITERATING
#if !defined(MAKE_ERL_TUPLE)
#define MAKE_ERL_TUPLE

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/fusion/tuple/tuple.hpp>
#include "erl_tuple.h"

namespace tinch_pp {
namespace erl {

inline e_tuple<boost::fusion::tuple<> > make_e_tuple()
{
  typedef boost::fusion::tuple<> Tuple;
  
  return erl::e_tuple<Tuple>(boost::fusion::tuple<>());
}

// A pointer to a tuple is typically used to store it in a list 
// as RPC argument.
inline object_ptr make_tuple_ptr()
{
  typedef boost::fusion::tuple<> Tuple;
  
  object_ptr created(new erl::e_tuple<Tuple>(boost::fusion::tuple<>()));

  return created;
}

#define BOOST_PP_FILENAME_1 "tinch_pp/make_erl_tuple.h"

/// If you need larger tuples, provide your own size through the pre-processor.
/// But please note that this library is implemented on top of Boost Fusion, which 
/// implies that you have to increase the maximum tuple size in Fusion too (see 
/// the Boost documentation).
#if !defined(MAX_ERL_TUPLE_SIZE)
  #define MAX_ERL_TUPLE_SIZE 10
#endif
#define BOOST_PP_ITERATION_LIMITS (1, MAX_ERL_TUPLE_SIZE)
#include BOOST_PP_ITERATE()

}}

#endif
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////

#define N BOOST_PP_ITERATION()

template <BOOST_PP_ENUM_PARAMS(N, typename T)>
inline e_tuple<boost::fusion::tuple<BOOST_PP_ENUM_PARAMS(N, T)> >
make_e_tuple(BOOST_PP_ENUM_BINARY_PARAMS(N, T, const& t))
{
  typedef boost::fusion::tuple<BOOST_PP_ENUM_PARAMS(N, T)> Tuple;
  
  return erl::e_tuple<Tuple>(boost::fusion::tuple<BOOST_PP_ENUM_PARAMS(N, T)>(
                             BOOST_PP_ENUM_PARAMS(N, t)));
}

template <BOOST_PP_ENUM_PARAMS(N, typename T)>
inline object_ptr make_tuple_ptr(BOOST_PP_ENUM_BINARY_PARAMS(N, T, const& t))
{
  typedef boost::fusion::tuple<BOOST_PP_ENUM_PARAMS(N, T)> Tuple;
  
  object_ptr created(new erl::e_tuple<Tuple>(boost::fusion::tuple<BOOST_PP_ENUM_PARAMS(N, T)>(
                                             BOOST_PP_ENUM_PARAMS(N, t))));
  return created;
}

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)

