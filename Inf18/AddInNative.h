/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Native AddIn RFIDUHF — интерфейс методов/свойств для мобильной платформы 1С.
//
// Автор: Каюмов А.Р.
// Дата:  02.10.2026
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include <string>
#include "../include/ComponentBase.h"
#include "../include/AddInDefBase.h"
#include "../include/IMemoryManager.h"
#include "MainApp.h"

class AddInNative : public IComponentBase
{
public:
	enum Props
	{
		ePropDeviceInfo = 0,
		ePropConnected,
		ePropInventoryRunning,
		ePropPower,
		ePropLastError,
		ePropLast
	};

	enum Methods
	{
		eMethInitialize = 0,
		eMethShutdown,
		eMethStartInventory,
		eMethStopInventory,
		eMethInventorySingle,
		eMethSetPower,
		eMethSetFrequencyMode,
		eMethLast
	};

	AddInNative(void);
	virtual ~AddInNative();

	virtual bool ADDIN_API Init(void*);
	virtual bool ADDIN_API setMemManager(void* mem);
	virtual long ADDIN_API GetInfo();
	virtual void ADDIN_API Done();

	virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T**);
	virtual long ADDIN_API GetNProps();
	virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
	virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
	virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal);
	virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal);
	virtual bool ADDIN_API IsPropReadable(const long lPropNum);
	virtual bool ADDIN_API IsPropWritable(const long lPropNum);
	virtual long ADDIN_API GetNMethods();
	virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
	virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias);
	virtual long ADDIN_API GetNParams(const long lMethodNum);
	virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue);
	virtual bool ADDIN_API HasRetVal(const long lMethodNum);
	virtual bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray);
	virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc);

private:
	long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
	void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);
	bool isNumericParameter(tVariant*);
	long numericValue(tVariant*);
	void ToV8String(const wchar_t* wstr, tVariant*);
	void SetBool(tVariant* pvar, bool value);
	void SetLong(tVariant* pvar, long value);

	IAddInDefBaseEx* m_iConnect;
	IMemoryManager* m_iMemory;
	MainApp javaMainApp{};
};

#endif
