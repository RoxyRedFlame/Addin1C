
#ifndef _SMART_COMPONENT_BASE_H_
#define _SMART_COMPONENT_BASE_H_

#include <WTypes.h>
#include <string>
#include <vector>
#include <functional>

#include "Metadata.h"
#include "Variant.h"
#include "AddinManager.h"
#include "BaseNativeAPI.h"

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))
#define CALL_POINTER_MEMBER(pointer, member) CALL_MEMBER_FN(*(pointer), (member))

namespace Addin1C {

	template <class ConcreteAddin>
	class AddinObject : public AbstractAddinObject {
	public:
		typedef ClassMetadata<ConcreteAddin> Metadata;

	private:
		std::wstring mLastErrorDescription;
		BaseNativeAPI::IAddInDefBase* mConnect;
		BaseNativeAPI::IMemoryManager* mMemoryManager;

		static inline Metadata& getMetadata() {
			static Metadata md = ConcreteAddin::getMetadata();
			md.addFunction(L"ErrorDescription", L"��������������", 0, &ConcreteAddin::getErrorDescription);
			return md;
		}

	protected:
		static const int PROP_READABLE = 1;
		static const int PROP_WRITEABLE = 2;

		AddinObject();

	public:
		virtual ~AddinObject() {}

		virtual bool ADDIN_API Init(void*);
		virtual bool ADDIN_API setMemManager(void* mem);
		virtual long ADDIN_API GetInfo();
		virtual void ADDIN_API Done();
		virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T**);
		virtual long ADDIN_API GetNProps();
		virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
		virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
		virtual bool ADDIN_API GetPropVal(const long lPropNum, BaseNativeAPI::tVariant* pvarPropVal);
		virtual bool ADDIN_API SetPropVal(const long lPropNum, BaseNativeAPI::tVariant* varPropVal);
		virtual bool ADDIN_API IsPropReadable(const long lPropNum);
		virtual bool ADDIN_API IsPropWritable(const long lPropNum);
		virtual long ADDIN_API GetNMethods();
		virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
		virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias);
		virtual long ADDIN_API GetNParams(const long lMethodNum);
		virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, BaseNativeAPI::tVariant *pvarParamDefValue);   
		virtual bool ADDIN_API HasRetVal(const long lMethodNum);
		virtual bool ADDIN_API CallAsProc(const long lMethodNum, BaseNativeAPI::tVariant* paParams, const long lSizeArray);
		virtual bool ADDIN_API CallAsFunc(const long lMethodNum, BaseNativeAPI::tVariant* pvarRetValue, BaseNativeAPI::tVariant* paParams, const long lSizeArray);
		virtual void ADDIN_API SetLocale(const WCHAR_T* loc);

		Variant getErrorDescription(VariantParameters);

		AbstractAddinObject* clone() { return new ConcreteAddin; }

		void message(std::wstring msg, long code = 0);

		static inline Metadata& metadata() {
			static Metadata& md = getMetadata();
			return md;
		}
	};

	template <class ConcreteAddin>
	Variant AddinObject<ConcreteAddin>::getErrorDescription(VariantParameters) {
		return mLastErrorDescription;
	}

	template <class ConcreteAddin>
	AddinObject<ConcreteAddin>::AddinObject()
		:
		mMemoryManager(NULL),
		mConnect(NULL)
	{
		AddinManager::getSingleton().registerObject((ConcreteAddin*)this);
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::Init(void* pConnection) { 
		mConnect = (BaseNativeAPI::IAddInDefBase*)pConnection;
		return mConnect != NULL;
	}

	template <class ConcreteAddin>
	long AddinObject<ConcreteAddin>::GetInfo() { 
		return 2000; 
	}

	template <class ConcreteAddin>
	void AddinObject<ConcreteAddin>::Done() {

	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::RegisterExtensionAs(WCHAR_T** wsExtensionName) { 
		const std::wstring& name = metadata().name();
	
		if (!mMemoryManager) return false;
		if (!mMemoryManager->AllocMemory((void**)wsExtensionName, (name.size() + 1) * sizeof(WCHAR_T))) return false;

		lstrcpyW(*wsExtensionName, name.c_str());

		return true; 
	}

	template <class ConcreteAddin>
	long AddinObject<ConcreteAddin>::GetNProps() { 
		return metadata().properties().size();
	}

	template <class ConcreteAddin>
	long AddinObject<ConcreteAddin>::FindProp(const WCHAR_T* wsPropName) { 
		auto& props = metadata().properties();
	
		for (auto i = 0U; i < props.size(); i++) {
			if (_wcsicmp(props[i].englishName.c_str(), wsPropName) == 0 ||
				_wcsicmp(props[i].localName.c_str(), wsPropName) == 0) return i;
		}

		return -1;
	}

	template <class ConcreteAddin>
	const WCHAR_T* AddinObject<ConcreteAddin>::GetPropName(long lPropNum, long lPropAlias) { 
		auto& props = metadata().properties();
	
		if ((unsigned long)lPropNum >= props.size()) return NULL;

		const std::wstring* name;

		if (lPropAlias == 0)
			name = &props[lPropNum].englishName;
		else
			name = &props[lPropNum].localName;

		if (!mMemoryManager) return NULL;

		wchar_t* result;
		if (!mMemoryManager->AllocMemory((void**)&result, (name->size() + 1) * sizeof(WCHAR_T))) return NULL;

		lstrcpyW(result, name->c_str());

		return result;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::GetPropVal(const long lPropNum, BaseNativeAPI::tVariant* pvarPropVal) { 
		auto& props = metadata().properties();

		if ((unsigned long)lPropNum >= props.size()) return false;

		auto property = props[lPropNum];

		bool error = true;

		try {

			packVariant(CALL_POINTER_MEMBER((ConcreteAddin*)this, property.getter)(), pvarPropVal, mMemoryManager);

			error = false;

		} catch (std::wstring& errorDescription) {
			mLastErrorDescription = errorDescription;
		} catch (std::exception& e) {
			std::string what = e.what();
			mLastErrorDescription = std::wstring(what.begin(), what.end());
		} catch (...) {
			mLastErrorDescription = L"<unknown error>";
		}

		if (!error) mLastErrorDescription.clear();

		return !error;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::SetPropVal(const long lPropNum, BaseNativeAPI::tVariant *varPropVal) { 
		auto& props = metadata().properties();

		if ((unsigned long)lPropNum >= props.size()) return false;

		auto property = props[lPropNum];

		bool error = true;

		try {

			CALL_POINTER_MEMBER((ConcreteAddin*)this, property.setter)(extractVariant(varPropVal));

			error = false;

		} catch (std::wstring& errorDescription) {
			mLastErrorDescription = errorDescription;
		} catch (std::exception& e) {
			std::string what = e.what();
			mLastErrorDescription = std::wstring(what.begin(), what.end());
		} catch (...) {
			mLastErrorDescription = L"<unknown error>";
		}

		if (!error) mLastErrorDescription.clear();

		return !error;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::IsPropReadable(const long lPropNum) { 
		auto& props = metadata().properties();

		if ((unsigned long)lPropNum >= props.size()) return false;

		return (props[lPropNum].modes & PROP_READABLE) != 0;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::IsPropWritable(const long lPropNum) {
		auto& props = metadata().properties();

		if ((unsigned long)lPropNum >= props.size()) return false;

		return (props[lPropNum].modes & PROP_WRITEABLE) != 0;
	}

	template <class ConcreteAddin>
	long AddinObject<ConcreteAddin>::GetNMethods() { 
		return metadata().functions().size();
	}

	template <class ConcreteAddin>
	long AddinObject<ConcreteAddin>::FindMethod(const WCHAR_T* wsMethodName) { 
		auto& funcs = metadata().functions();

		for (auto i = 0U; i < funcs.size(); i++) {
			if (_wcsicmp(funcs[i].englishName.c_str(), wsMethodName) == 0 ||
				_wcsicmp(funcs[i].localName.c_str(), wsMethodName) == 0) return i;
		}

		return -1;
	}

	template <class ConcreteAddin>
	const WCHAR_T* AddinObject<ConcreteAddin>::GetMethodName(const long lMethodNum, const long lMethodAlias) { 
		auto& funcs = metadata().functions();

		if ((unsigned long)lMethodNum >= funcs.size()) return NULL;

		const std::wstring* name;

		if (lMethodAlias == 0)
			name = &funcs[lMethodNum].englishName;
		else
			name = &funcs[lMethodNum].localName;

		if (!mMemoryManager) return NULL;

		wchar_t* result;
		if (!mMemoryManager->AllocMemory((void**)&result, (name->size() + 1) * sizeof(WCHAR_T))) return NULL;

		lstrcpyW(result, name->c_str());

		return result;
	}

	template <class ConcreteAddin>
	long AddinObject<ConcreteAddin>::GetNParams(const long lMethodNum) { 
		auto& funcs = metadata().functions();

		if ((unsigned long)lMethodNum >= funcs.size()) return 0;

		return funcs[lMethodNum].parametersCount;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::GetParamDefValue(const long lMethodNum, const long lParamNum, BaseNativeAPI::tVariant *pvarParamDefValue) { 
		TV_VT(pvarParamDefValue)= BaseNativeAPI::VTYPE_EMPTY;
		return false;
	} 

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::HasRetVal(const long lMethodNum) { 
		if ((unsigned long)lMethodNum >= metadata().functions().size()) return false;

		return true;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::CallAsProc(const long lMethodNum, BaseNativeAPI::tVariant* paParams, const long lSizeArray) { 
		return false;
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::CallAsFunc(const long lMethodNum, BaseNativeAPI::tVariant* pvarRetValue, BaseNativeAPI::tVariant* paParams, const long lSizeArray) { 
		auto& funcs = metadata().functions();

		if ((unsigned long)lMethodNum >= funcs.size()) return false;
		if (lSizeArray != funcs[lMethodNum].parametersCount) return false;

		bool error = true;

		try {

			std::vector<Variant> smartParameters(lSizeArray);
			for (int i = 0; i < lSizeArray; i++) {
				smartParameters[i] = extractVariant(paParams + i);
			}

			Variant result = CALL_POINTER_MEMBER((ConcreteAddin*)this, funcs[lMethodNum].method)(smartParameters);
		
			for (int i = 0; i < lSizeArray; i++) {
				packVariant(smartParameters[i], paParams + i, mMemoryManager);
			}

			packVariant(result, pvarRetValue, mMemoryManager);

			error = false;

		} catch (std::wstring& errorDescription) {
			mLastErrorDescription = errorDescription;
		} catch (std::exception& e) {
			std::string what = e.what();
			mLastErrorDescription = std::wstring(what.begin(), what.end());
		} catch (...) {
			mLastErrorDescription = L"<unknown error>";
		}

		if (!error) mLastErrorDescription.clear();

		return !error;
	}

	template <class ConcreteAddin>
	void AddinObject<ConcreteAddin>::SetLocale(const WCHAR_T* loc) {
		_wsetlocale(LC_ALL, loc);
	}

	template <class ConcreteAddin>
	bool AddinObject<ConcreteAddin>::setMemManager(void* mem) {
		mMemoryManager = (BaseNativeAPI::IMemoryManager*)mem;
		return mMemoryManager != NULL;
	}

	template <class ConcreteAddin>
	void AddinObject<ConcreteAddin>::message(std::wstring msg, long code /*= 0*/) {
		if (!mConnect) return;
		mConnect->AddError(ADDIN_E_INFO, metadata().name().c_str(), msg.c_str(), code);
	}

}

#endif
