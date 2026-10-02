/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JNI-обёртка Java-класса org.rfiduhf.addin.MainApp для Native AddIn 1С.
//
// Автор: Каюмов А.Р.
// Дата:  02.10.2026
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include <wchar.h>
#include "MainApp.h"
#include "ConversionWchar.h"

MainApp::MainApp() : cc(nullptr), obj(nullptr)
{
}

MainApp::~MainApp()
{
	if (obj)
	{
		shutdownReader();
		JNIEnv* env = getJniEnv();
		if (env)
		{
			env->DeleteGlobalRef(obj);
			if (cc)
				env->DeleteGlobalRef(cc);
		}
		obj = nullptr;
		cc = nullptr;
	}
}

void MainApp::Initialize(IAddInDefBaseEx* cnn)
{
	if (obj || !cnn)
		return;

	IAndroidComponentHelper* helper =
		(IAndroidComponentHelper*)cnn->GetInterface(eIAndroidComponentHelper);
	if (!helper)
		return;

	WCHAR_T* className = nullptr;
	convToShortWchar(&className, L"org.rfiduhf.addin.MainApp");
	jclass ccloc = helper->FindClass(className);
	delete[] className;

	if (!ccloc)
		return;

	JNIEnv* env = getJniEnv();
	if (!env)
		return;

	cc = static_cast<jclass>(env->NewGlobalRef(ccloc));
	env->DeleteLocalRef(ccloc);

	jobject activity = helper->GetActivity();
	jmethodID ctor = env->GetMethodID(cc, "<init>", "(Landroid/app/Activity;J)V");
	if (!ctor)
	{
		env->DeleteLocalRef(activity);
		return;
	}

	// В конструктор передаём указатель на IAddInDefBaseEx — его же вернём в OnTagRead.
	jobject objloc = env->NewObject(cc, ctor, activity, (jlong)cnn);
	obj = static_cast<jobject>(env->NewGlobalRef(objloc));
	env->DeleteLocalRef(objloc);

	jmethodID show = env->GetMethodID(cc, "show", "()V");
	if (show)
		env->CallVoidMethod(obj, show);

	env->DeleteLocalRef(activity);
}

bool MainApp::callBool(const char* name, const char* sig) const
{
	if (!obj || !cc)
		return false;
	JNIEnv* env = getJniEnv();
	if (!env)
		return false;
	jmethodID mid = env->GetMethodID(cc, name, sig);
	if (!mid)
		return false;
	return env->CallBooleanMethod(obj, mid) == JNI_TRUE;
}

bool MainApp::callBoolInt(const char* name, const char* sig, jint arg) const
{
	if (!obj || !cc)
		return false;
	JNIEnv* env = getJniEnv();
	if (!env)
		return false;
	jmethodID mid = env->GetMethodID(cc, name, sig);
	if (!mid)
		return false;
	return env->CallBooleanMethod(obj, mid, arg) == JNI_TRUE;
}

long MainApp::callInt(const char* name, const char* sig) const
{
	if (!obj || !cc)
		return -1;
	JNIEnv* env = getJniEnv();
	if (!env)
		return -1;
	jmethodID mid = env->GetMethodID(cc, name, sig);
	if (!mid)
		return -1;
	return (long)env->CallIntMethod(obj, mid);
}

std::wstring MainApp::callString(const char* name) const
{
	std::wstring result;
	if (!obj || !cc)
		return result;
	JNIEnv* env = getJniEnv();
	if (!env)
		return result;
	jmethodID mid = env->GetMethodID(cc, name, "()Ljava/lang/String;");
	if (!mid)
		return result;
	jstring jstr = (jstring)env->CallObjectMethod(obj, mid);
	if (!jstr)
		return result;

	const jchar* chars = env->GetStringChars(jstr, nullptr);
	jsize len = env->GetStringLength(jstr);
	result.assign(chars, chars + len);
	env->ReleaseStringChars(jstr, chars);
	env->DeleteLocalRef(jstr);
	return result;
}

bool MainApp::initializeReader() const { return callBool("initialize", "()Z"); }
bool MainApp::shutdownReader() const { return callBool("shutdown", "()Z"); }
bool MainApp::startInventory() const { return callBool("startInventory", "()Z"); }
bool MainApp::stopInventory() const { return callBool("stopInventory", "()Z"); }
bool MainApp::inventorySingle() const { return callBool("inventorySingle", "()Z"); }
bool MainApp::setPower(long dbm) const { return callBoolInt("setPower", "(I)Z", (jint)dbm); }
bool MainApp::setFrequencyMode(long mode) const { return callBoolInt("setFrequencyMode", "(I)Z", (jint)mode); }
long MainApp::getPower() const { return callInt("getPower", "()I"); }
bool MainApp::isConnected() const { return callBool("isConnected", "()Z"); }
bool MainApp::isInventoryRunning() const { return callBool("isInventoryRunning", "()Z"); }
std::wstring MainApp::getLastError() const { return callString("getLastError"); }
std::wstring MainApp::getDeviceInfo() const { return callString("getDeviceInfo"); }

void MainApp::setCC(jclass _cc) { cc = _cc; }
void MainApp::setOBJ(jobject _obj) { obj = _obj; }

static const wchar_t g_EventSource[] = L"org.rfiduhf.addin";
static const wchar_t g_EventTag[] = L"TagRead";
static const wchar_t g_EventError[] = L"Error";
static WcharWrapper s_EventSource(g_EventSource);
static WcharWrapper s_EventTag(g_EventTag);
static WcharWrapper s_EventError(g_EventError);

// Java: static native void onTagRead(long, String)
extern "C" JNIEXPORT void JNICALL
Java_org_rfiduhf_addin_MainApp_onTagRead(JNIEnv* env, jclass /*cls*/, jlong pObject, jstring jJson)
{
	IAddInDefBaseEx* pAddIn = (IAddInDefBaseEx*)pObject;
	if (!pAddIn)
		return;

	WCHAR_T* data = nullptr;
	if (jJson)
	{
		const jchar* chars = env->GetStringChars(jJson, nullptr);
		jsize len = env->GetStringLength(jJson);
		data = new WCHAR_T[len + 1];
		for (jsize i = 0; i < len; ++i)
			data[i] = (WCHAR_T)chars[i];
		data[len] = 0;
		env->ReleaseStringChars(jJson, chars);
	}

	// Платформа копирует строки синхронно внутри ExternalEvent.
	pAddIn->ExternalEvent(s_EventSource, s_EventTag, data);
	delete[] data;
}

// Java: static native void onError(long, String)
extern "C" JNIEXPORT void JNICALL
Java_org_rfiduhf_addin_MainApp_onError(JNIEnv* env, jclass /*cls*/, jlong pObject, jstring jMsg)
{
	IAddInDefBaseEx* pAddIn = (IAddInDefBaseEx*)pObject;
	if (!pAddIn)
		return;

	WCHAR_T* data = nullptr;
	if (jMsg)
	{
		const jchar* chars = env->GetStringChars(jMsg, nullptr);
		jsize len = env->GetStringLength(jMsg);
		data = new WCHAR_T[len + 1];
		for (jsize i = 0; i < len; ++i)
			data[i] = (WCHAR_T)chars[i];
		data[len] = 0;
		env->ReleaseStringChars(jMsg, chars);
	}

	pAddIn->ExternalEvent(s_EventSource, s_EventError, data);
	delete[] data;
}
