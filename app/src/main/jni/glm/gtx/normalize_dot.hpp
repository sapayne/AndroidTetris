///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref gtx_normalize_dot
/// @file glm/gtx/normalize_dot.hpp
/// @date 2007-09-28 / 2011-06-07
/// @author Christophe Riccio
///
/// @see core (dependence)
/// @see gtx_fast_square_root (dependence)
///
/// @defgroup gtx_normalize_dot GLM_GTX_normalize_dot
/// @ingroup gtx
/// 
/// @brief Dot product of vectors that need to be normalize with a single square root.
/// 
/// <glm/gtx/normalized_dot.hpp> need to be included to use these functionalities.
///////////////////////////////////////////////////////////////////////////////////

#ifndef GLM_GTX_normalize_dot
#define GLM_GTX_normalize_dot

// Dependency:
#include "../glm.hpp"
#include "fast_square_root.hpp"

#if(defined(GLM_MESSAGES) && !defined(GLM_EXT_INCLUDED))
#	pragma message("GLM: GLM_GTX_normalize_dot extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_normalize_dot
	/// @{

	//! Normalize parameters and returns the dot product of x and y.
	//! It's faster that dot(normalize(x), normalize(y)).
	//! From GLM_GTX_normalize_dot extension.
	template <typename genType> 
	GLM_FUNC_DECL typename genType::value_type normalizeDot(
		genType const & x, 
		genType const & y);

	//! Normalize parameters and returns the dot product of x and y.
	//! Faster that dot(fastNormalize(x), fastNormalize(y)).
	//! From GLM_GTX_normalize_dot extension.
	template <typename genType> 
	GLM_FUNC_DECL typename genType::value_type fastNormalizeDot(
		genType const & x, 
		genType const & y);

	/// @}
}//namespace glm

#include "normalize_dot.inl"

#endif//GLM_GTX_normalize_dot
