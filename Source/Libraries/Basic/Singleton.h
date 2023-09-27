/*!
	@file include/CorsacBase/Singleton.h
	@author Electronic Arts Inc.
	@date 16/05/2019
	@license BSD3
	@see https://github.com/electronicarts/EAStdC/blob/master/include/EAStdC/EASingleton.h
*/
/*
	Copyright (C) 2017 Electronic Arts Inc.  All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1.  Redistributions of source code must retain the above copyright
		notice, this list of conditions and the following disclaimer.
	2.  Redistributions in binary form must reproduce the above copyright
		notice, this list of conditions and the following disclaimer in the
		documentation and/or other materials provided with the distribution.
	3.  Neither the name of Electronic Arts, Inc. ("EA") nor the names of
		its contributors may be used to endorse or promote products derived
		from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY ELECTRONIC ARTS AND ITS CONTRIBUTORS "AS IS" AND ANY
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS OR ITS CONTRIBUTORS BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef EASTDC_EASINGLETON_H
#define EASTDC_EASINGLETON_H

#include <assert.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127) // conditional expression is constant
#endif

#ifdef CORSAC_NS
namespace CORSAC_NS
{
#endif
	/// \class Singleton
	///
	/// Singleton adds singleton semantics to derived classes.  It provides
	/// singleton-style instance accessors and will assert if more than one
	/// instance of the derived class is constructed.
	///
	/// \code
	/// class UniqueWidget : public CorsacBase::Singleton<UniqueWidget> { };
	/// UniqueWidget *pWidget = UniqueWidget::GetInstance();
	/// \endcode
	///
	/// \param  T           The classname from which to create the singleton.
	/// \param  kId         Multiple unique singleton instances of class \a T
	///                     can be created if they are given unique \a kId
	///                     numbers.
	///
	template <typename T, unsigned int kId = 0>
	class Singleton
	{
	public:
		typedef T value_type;
		typedef Singleton<T, kId> this_type;

		/// Return the pointer to the singleton instance.
		static T* GetInstance()
		{
			return static_cast<T*>(spInstance);
		}

	protected:
		/// Constructor
		/// The singleton instance is assigned at construction time.
		Singleton()
		{
			assert(!spInstance); // "Singleton instance (%p) has already been created", static_cast<T*>(spInstance)));
			spInstance = this;
		}

		/// Destructor
		/// This destructor is intentionally not marked 'virtual'.  We don't
		/// want to force virtual-ness on our derived class.
		~Singleton()
		{
			spInstance = nullptr;
		}

	private:
		/// Private (disabled) copy constructor
		Singleton(const this_type&) = delete;

		/// Private (disabled) assignment operator
		Singleton& operator=(const this_type&) = delete;

		/// Static pointer to this singleton's instance.
		static this_type* spInstance;
	};


	/// \class  SingletonAdapter
	///
	/// SingletonAdapter adds singleton semantics to an existing class by
	/// extending its public interface.  This is useful for creating
	/// singletons from existing (and potentially externally maintained)
	/// classes without modifiying the original code directly.
	///
	/// To use this class, derive a new class with a unique name from the base
	/// class (since the static instance pointer will be unique only for this
	/// class name).
	///
	/// \code
	/// class WidgetSingleton : public CorsacBase::SingletonAdapter<Widget> { };
	/// \endcode
	///
	/// If implicit creation was requested, the singleton's instance will be
	/// created the first time it is accessed.  Otherwise, CreateInstance() or
	/// SetInstance() must be called.
	///
	/// The singleton's instance can be destructed and freed using Destroy().
	///
	/// SingletonAdapter<> is not thread-safe.
	///
	/// If you're going to be using \a T exclusively through the
	/// SingletonAdapter interface, you should consider making its constructor
	/// and destructor protected members.  That will ensure that instances of
	/// \a T can only be created by the SingletonAdapter layer.
	///
	/// \param  T           The classname from which to create the singleton.
	/// \param  bImplicitCreation   If implicit creation is requested, the
	///                     singleton instance will be created on the first
	///                     attempt to access it.
	/// \param  kId         Multiple unique singleton instances of class \a T
	///                     can be created if they are given unique \a kId
	///                     numbers.
	///
	template <typename T, bool bImplicitCreation = false, unsigned int kId = 0>
	class SingletonAdapter : public T
	{
	public:
		typedef T value_type;
		typedef SingletonAdapter<T, bImplicitCreation, kId> this_type;

		/// Return the pointer to the singleton instance.
		/// If \a bImplicitCreation was requested, an instance will be created
		/// if one does not already exist.
		static T* GetInstance()
		{
			if (bImplicitCreation && !spInstance)
				return CreateInstance();

			return static_cast<T*>(spInstance);
		}

		/// This allows you to manually set the instance, which is useful
		/// if you want to allocate the memory for it yourself. 
		/// \return A pointer to the previous instance.
		static T* SetInstance(_In_ T* pInstance)
		{
			T* const pPrevious = spInstance;
			spInstance = pInstance;
			return pPrevious;
		}

		/// Create this singleton's instance.  If the instance has already
		/// been created, a pointer to it will simply be returned.
		/// \return A pointer to the singleton's instance.
		static T* CreateInstance()
		{
			if (!spInstance)
				spInstance = new SingletonAdapter();

			return spInstance;
		}

		/// Destroy this singleton's instance.
		static void DestroyInstance()
		{
			if (spInstance)
			{
				delete spInstance;
				spInstance = NULL;
			}
		}

	protected:
		/// Constructor
		SingletonAdapter() { }

		/// Destructor
		~SingletonAdapter() { }

	private:
		/// Private (disabled) copy constructor
		SingletonAdapter(const this_type&) = delete;

		/// Private (disabled) assignment operator
		SingletonAdapter& operator=(const this_type&) = delete;

		/// Static pointer to this singleton's instance.
		static this_type* spInstance;
	};

#ifdef CORSAC_NS
} // namespace CorsacBase
#endif


// Initialize the singletons' static instance pointers to NULL.

#ifdef CORSAC_NS
template <typename T, unsigned int kId> CORSAC_NS::Singleton<T, kId>* CORSAC_NS::Singleton<T, kId>::spInstance = nullptr;
template <typename T, bool bImplicitCreation, unsigned int kId> CORSAC_NS::SingletonAdapter<T, bImplicitCreation, kId>* CORSAC_NS::SingletonAdapter<T, bImplicitCreation, kId>::spInstance = nullptr;
#else
template <typename T, unsigned int kId> Singleton<T, kId>* Singleton<T, kId>::spInstance = nullptr;
template <typename T, bool bImplicitCreation, unsigned int kId> SingletonAdapter<T, bImplicitCreation, kId>* SingletonAdapter<T, bImplicitCreation, kId>::spInstance = nullptr;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // Header include guard
