/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JNI-обёртка Java-класса org.rfiduhf.addin.MainApp для Native AddIn 1С.
//
// Автор: Каюмов А.Р.
// Дата:  02.10.2026
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../include/AddInDefBase.h"
#include "../include/IAndroidComponentHelper.h"
#include "../jni/jnienv.h"
#include "../include/IMemoryManager.h"
#include <string>

class MainApp
{
private:
	jclass cc;
	jobject obj;

	bool callBool(const char* name, const char* sig) const;
	bool callBoolInt(const char* name, const char* sig, jint arg) const;
	long callInt(const char* name, const char* sig) const;
	std::wstring callString(const char* name) const;

public:
	MainApp();
	~MainApp();

	void setCC(jclass _cc);
	void setOBJ(jobject _obj);

	// Создаёт Java MainApp и грузит native-lib (как в шаблоне androidUtils).
	void Initialize(IAddInDefBaseEx* cnn);

	bool initializeReader() const;
	bool shutdownReader() const;
	bool startInventory() const;
	bool stopInventory() const;
	bool inventorySingle() const;
	bool setPower(long dbm) const;
	bool setFrequencyMode(long mode) const;
	long getPower() const;
	bool isConnected() const;
	bool isInventoryRunning() const;
	std::wstring getLastError() const;
	std::wstring getDeviceInfo() const;
};
