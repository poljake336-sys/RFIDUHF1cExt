/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Native AddIn RFIDUHF — интерфейс методов/свойств для мобильной платформы 1С.
//
// Автор: Каюмов А.Р.
// Дата:  02.10.2026
//
// Подключение в 1С:
//   ПодключитьВнешнююКомпоненту(..., "RFID", ТипВнешнейКомпоненты.Native);
//   Компонента = Новый("AddIn.RFID.RFIDUHF");
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "AddInNative.h"
#include "ConversionWchar.h"
#include "wchar.h"
#include <string>
#include "../jni/jnienv.h"
#include "../include/IAndroidComponentHelper.h"

static const wchar_t* g_PropNames[] =
{
	L"DeviceInfo",
	L"Connected",
	L"InventoryRunning",
	L"Power",
	L"LastError"
};

static const wchar_t* g_PropNamesRu[] =
{
	L"ОписаниеУстройства",
	L"Подключено",
	L"ИнвентаризацияАктивна",
	L"Мощность",
	L"ПоследняяОшибка"
};

static const wchar_t* g_MethodNames[] =
{
	L"Initialize",
	L"Shutdown",
	L"StartInventory",
	L"StopInventory",
	L"InventorySingle",
	L"SetPower",
	L"SetFrequencyMode"
};

static const wchar_t* g_MethodNamesRu[] =
{
	L"Инициализировать",
	L"Завершить",
	L"НачатьИнвентаризацию",
	L"ОстановитьИнвентаризацию",
	L"СчитатьОднуМетку",
	L"УстановитьМощность",
	L"УстановитьЧастотныйРежим"
};

static const wchar_t g_ComponentNameAddIn[] = L"RFIDUHF";
static WcharWrapper s_ComponentClass(g_ComponentNameAddIn);
const long g_VersionAddIn = 2100;
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;

long GetClassObject(const WCHAR_T* /*wsName*/, IComponentBase** pInterface)
{
	if (!*pInterface)
	{
		*pInterface = new AddInNative();
		return (long)*pInterface;
	}
	return 0;
}

AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}

long DestroyObject(IComponentBase** pInterface)
{
	if (!*pInterface)
		return -1;
	delete *pInterface;
	*pInterface = 0;
	return 0;
}

const WCHAR_T* GetClassNames()
{
	return s_ComponentClass;
}

AddInNative::AddInNative() : m_iConnect(nullptr), m_iMemory(nullptr)
{
}

AddInNative::~AddInNative()
{
}

bool AddInNative::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBaseEx*)pConnection;
	if (!m_iConnect)
		return false;

	// Поднимаем Java-объект сразу при подключении компоненты.
	javaMainApp.Initialize(m_iConnect);
	return true;
}

bool AddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != nullptr;
}

long AddInNative::GetInfo()
{
	return g_VersionAddIn;
}

void AddInNative::Done()
{
	// Гасим UHF до отключения AddIn от платформы.
	javaMainApp.shutdownReader();
	m_iConnect = nullptr;
	m_iMemory = nullptr;
}

bool AddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	const wchar_t* wsExtension = g_ComponentNameAddIn;
	uint32_t iActualSize = static_cast<uint32_t>(::wcslen(wsExtension) + 1);
	if (m_iMemory && m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
	{
		convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}
	return false;
}

long AddInNative::GetNProps()
{
	return ePropLast;
}

long AddInNative::FindProp(const WCHAR_T* wsPropName)
{
	wchar_t* propName = 0;
	convFromShortWchar(&propName, wsPropName);
	long plPropNum = findName(g_PropNames, propName, ePropLast);
	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);
	delete[] propName;
	return plPropNum;
}

const WCHAR_T* AddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t* wsCurrentName = NULL;
	WCHAR_T* wsPropName = NULL;
	switch (lPropAlias)
	{
	case 0: wsCurrentName = (wchar_t*)g_PropNames[lPropNum]; break;
	case 1: wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum]; break;
	default: return 0;
	}

	uint32_t iActualSize = static_cast<uint32_t>(wcslen(wsCurrentName) + 1);
	if (m_iMemory && wsCurrentName
		&& m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
	{
		convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}
	return wsPropName;
}

bool AddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum)
	{
	case ePropDeviceInfo:
		ToV8String(javaMainApp.getDeviceInfo().c_str(), pvarPropVal);
		return true;
	case ePropConnected:
		SetBool(pvarPropVal, javaMainApp.isConnected());
		return true;
	case ePropInventoryRunning:
		SetBool(pvarPropVal, javaMainApp.isInventoryRunning());
		return true;
	case ePropPower:
		SetLong(pvarPropVal, javaMainApp.getPower());
		return true;
	case ePropLastError:
		ToV8String(javaMainApp.getLastError().c_str(), pvarPropVal);
		return true;
	default:
		return false;
	}
}

bool AddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal)
{
	if (lPropNum == ePropPower)
	{
		if (!isNumericParameter(varPropVal))
			return false;
		return javaMainApp.setPower(numericValue(varPropVal));
	}
	return false;
}

bool AddInNative::IsPropReadable(const long /*lPropNum*/)
{
	return true;
}

bool AddInNative::IsPropWritable(const long lPropNum)
{
	return lPropNum == ePropPower;
}

long AddInNative::GetNMethods()
{
	return eMethLast;
}

long AddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	wchar_t* name = 0;
	convFromShortWchar(&name, wsMethodName);
	long plMethodNum = findName(g_MethodNames, name, eMethLast);
	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);
	delete[] name;
	return plMethodNum;
}

const WCHAR_T* AddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t* wsCurrentName = NULL;
	WCHAR_T* wsMethodName = NULL;
	switch (lMethodAlias)
	{
	case 0: wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum]; break;
	case 1: wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum]; break;
	default: return 0;
	}

	uint32_t iActualSize = static_cast<uint32_t>(wcslen(wsCurrentName) + 1);
	if (m_iMemory && wsCurrentName
		&& m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
	{
		convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}
	return wsMethodName;
}

long AddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethSetPower:
	case eMethSetFrequencyMode:
		return 1;
	default:
		return 0;
	}
}

bool AddInNative::GetParamDefValue(const long /*lMethodNum*/, const long /*lParamNum*/, tVariant* /*pvarParamDefValue*/)
{
	return false;
}

bool AddInNative::HasRetVal(const long lMethodNum)
{
	return lMethodNum >= eMethInitialize && lMethodNum < eMethLast;
}

bool AddInNative::CallAsProc(const long /*lMethodNum*/, tVariant* /*paParams*/, const long /*lSizeArray*/)
{
	return false;
}

bool AddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long /*lSizeArray*/)
{
	switch (lMethodNum)
	{
	case eMethInitialize:
		SetBool(pvarRetValue, javaMainApp.initializeReader());
		return true;
	case eMethShutdown:
		SetBool(pvarRetValue, javaMainApp.shutdownReader());
		return true;
	case eMethStartInventory:
		SetBool(pvarRetValue, javaMainApp.startInventory());
		return true;
	case eMethStopInventory:
		SetBool(pvarRetValue, javaMainApp.stopInventory());
		return true;
	case eMethInventorySingle:
		SetBool(pvarRetValue, javaMainApp.inventorySingle());
		return true;
	case eMethSetPower:
		if (!paParams || !isNumericParameter(paParams))
			return false;
		SetBool(pvarRetValue, javaMainApp.setPower(numericValue(paParams)));
		return true;
	case eMethSetFrequencyMode:
		if (!paParams || !isNumericParameter(paParams))
			return false;
		SetBool(pvarRetValue, javaMainApp.setFrequencyMode(numericValue(paParams)));
		return true;
	default:
		return false;
	}
}

void AddInNative::SetLocale(const WCHAR_T* /*loc*/)
{
}

void AddInNative::addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code)
{
	if (!m_iConnect)
		return;
	WCHAR_T* err = 0;
	WCHAR_T* descr = 0;
	convToShortWchar(&err, source);
	convToShortWchar(&descr, descriptor);
	m_iConnect->AddError(wcode, err, descr, code);
	delete[] descr;
	delete[] err;
}

long AddInNative::findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const
{
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
			return (long)i;
	}
	return -1;
}

void AddInNative::ToV8String(const wchar_t* wstr, tVariant* par)
{
	if (!m_iMemory)
	{
		par->vt = VTYPE_EMPTY;
		return;
	}
	if (wstr)
	{
		int len = (int)wcslen(wstr);
		m_iMemory->AllocMemory((void**)&par->pwstrVal, (len + 1) * sizeof(WCHAR_T));
		convToShortWchar(&par->pwstrVal, wstr);
		par->vt = VTYPE_PWSTR;
		par->wstrLen = len;
	}
	else
		par->vt = VTYPE_EMPTY;
}

bool AddInNative::isNumericParameter(tVariant* par)
{
	return par && (par->vt == VTYPE_I4 || par->vt == VTYPE_UI4 || par->vt == VTYPE_R8);
}

long AddInNative::numericValue(tVariant* par)
{
	switch (par->vt)
	{
	case VTYPE_I4: return par->lVal;
	case VTYPE_UI4: return (long)par->ulVal;
	case VTYPE_R8: return (long)par->dblVal;
	default: return 0;
	}
}

void AddInNative::SetBool(tVariant* pvar, bool value)
{
	TV_VT(pvar) = VTYPE_BOOL;
	TV_BOOL(pvar) = value ? 1 : 0;
}

void AddInNative::SetLong(tVariant* pvar, long value)
{
	TV_VT(pvar) = VTYPE_I4;
	TV_I4(pvar) = value;
}
